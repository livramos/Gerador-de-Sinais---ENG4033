#include <Arduino.h>   // necessario em .cpp (o .ino adiciona isso automaticamente)
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>   // driver SPI do display (Wokwi)
#include <SPI.h>
#include <RotaryEncoder.h>
#include <GFButton.h>
#include <math.h>
#include <EEPROM.h>
#include <LinkedList.h>
#include <MCUFRIEND_kbv.h>

// verificar salvamento da EPROM -> livia 
#define TFT_CS  10
#define TFT_DC   9
#define TFT_RST  8
#define COR_GRADE 0x2945
// isso aq é pro simulador 
#define COR_SEN   ILI9341_GREEN
#define COR_COS   ILI9341_MAGENTA
#define COR_TRI   ILI9341_CYAN     // cor do novo sinal triangular
#define COR_DENTE ILI9341_ORANGE   // cor do novo sinal dente de serra
#define COR_INFO  ILI9341_YELLOW
#define COR_PRETO        ILI9341_BLACK
#define COR_AZUL         ILI9341_BLUE
#define COR_BRANCO        ILI9341_WHITE
#define COR_CINZA_CLARO  ILI9341_LIGHTGREY
#define COR_VERMELHO    ILI9341_RED
/*
#define COR_SEN   0x07E0   // verde
#define COR_COS   0xF81F   // magenta
#define COR_TRI   0x07FF   // ciano
#define COR_DENTE 0xFD20   // laranja
#define COR_INFO  0xFFE0   // amarelo
#define COR_PRETO        0x0000
#define COR_AZUL         0x001F
#define COR_BRANCO       0xFFFF
#define COR_CINZA_CLARO  0xC618
#define COR_VERMELHO     0xF800

*/
#define encY_CLK  21 
#define encY_DT   20
#define encY_SW   A12  
#define encX_CLK  18
#define encX_DT   19
#define G_X0   35
#define G_X1  315
#define G_Y0   25
#define G_Y1  210
#define G_CY  117


struct Sinal {
    int valores[100];   
    int qtdPontos;
};

struct Opcao {
    char nome[100];
    uint16_t cor;
    float amp;
    float freq;        
    Sinal sinal;       
};

LinkedList<Opcao> *opcoes = new LinkedList<Opcao>();
const int OPCOES_VISIVEIS = 3; 
GFButton botao_enc(encY_SW);  
Adafruit_ILI9341 lcd(TFT_CS, TFT_DC, TFT_RST); // so funciona no woki
//MCUFRIEND_kbv lcd(TFT_CS, TFT_DC, TFT_RST);
RotaryEncoder encY(encY_CLK, encY_DT, RotaryEncoder::LatchMode::TWO03);
RotaryEncoder encX(encX_CLK,encX_DT);
int  telaAtual         = 0;
int  opcao_selecionada = 0;
int  menu_offset       = 0;  
int posMenu_ant = 0;
long posAmp_ant = 0;
long posFreq_ant = 0;
unsigned long ultimoMovimentoMenu = 0;
const unsigned long intervaloMenu = 100;
const unsigned long bloqueioCliqueAposGiro = 200;
int curvaY[G_X1 + 1];
int ultimoEndereco = 4;
int qtdSinais = 0;

int posicaoAnterior_enc1 = 0;
int posicaoAnterior_enc2 = 0;
int indiceAtual = 0;
unsigned long ultimoPonto = 0;
Sinal sinalAtual;
bool ativo = false;
float frequenciaAtual = 1.0;
float amplitudeAtual = 1.0;
float listaValores[100];
int qtdValores = 0;
float minimo;
float maximo;
float indice = 0;
String nome = "";
String funcao = "";
bool state = false;
int t = 0;
int value = 0;
unsigned long instante_anterior = 0;
unsigned long intervalo = 100;
int dir = 1;
int saw = 0;


void desenhaTelaSelecao();
void desenhaTelaGrafico();
void atualizaOpcoes();
void desenhaOpcao(int idx, const char *nome, uint16_t cor, int y);
void desenhaEixosEGrade();
void redesenhaCurva(uint16_t cor);
void atualizaInfoEncoders();
void muda_tela(GFButton& btn);
// func do lorenzo 
void salvarConfiguracao(Sinal s);
void LimparEprom();
void carregarConfiguracao();
void printobj();
void calculaAmplitude(Sinal &s);
void calculaFrequencia();

// isso aq deve sumir 
/*
float amostra(Opcao& op, float idx)
{
    const Sinal* s = op.sinal;
    int i = (int)idx % s->qtdPontos;     
    if (i < 0) i += s->qtdPontos;
    int iProx = (i + 1) % s->qtdPontos;    
    float frac = idx - floor(idx);         
    float v = s->valores[i] + (s->valores[iProx] - s->valores[i]) * frac;
    return v / 100.0f;                     
}
*/



void desenhaTelaSelecao()
{
     lcd.setRotation(1);
    lcd.fillScreen(ILI9341_BLACK);
    lcd.setTextColor(ILI9341_CYAN);
    lcd.setTextSize(2);
    lcd.setCursor(70, 8);                       // centralizado p/ largura 320
    lcd.print("Escolha o Sinal");
    lcd.drawLine(10, 38, 310, 38,COR_BRANCO); 
    atualizaOpcoes();
    lcd.fillRect(195, 60, 115, 30,COR_AZUL );
    lcd.drawRect(195, 60, 115, 30,  COR_BRANCO );
    lcd.setTextColor( COR_BRANCO );
    lcd.setTextSize(1);
    lcd.setCursor(205, 72);
    lcd.print("BTN: Confirmar");
    lcd.setTextColor(COR_CINZA_CLARO);
    lcd.setCursor(200, 110);
    lcd.print("SCROLL:");
    lcd.setCursor(200, 122);
    lcd.print("trocar sinal");
    Serial.println("tela selecao");
}

// desenha a lista a partir do array opcoes[], com viewport rolavel
void atualizaOpcoes()
{
    lcd.fillRect(10, 45, 175, OPCOES_VISIVEIS * 60, COR_PRETO);

    for (int i = 0; i < OPCOES_VISIVEIS; i++) {
        int idx = menu_offset + i;

        if (idx >= opcoes->size()) break;

        Opcao op = opcoes->get(idx);

        desenhaOpcao(
            idx,
            op.nome,
            op.cor,
            45 + i * 60
        );
    }
}

void desenhaOpcao(int idx, const char *nome, uint16_t cor, int y)
{
    bool sel = (opcao_selecionada == idx);
    lcd.setTextColor(sel ? cor : COR_BRANCO);
    lcd.setTextSize(2);
    lcd.setCursor(25, y + 17);
    lcd.print(sel ? "> " : "  ");
    lcd.print(nome);
}
// parte do grafico 

void desenhaTelaGrafico()
{
    lcd.setRotation(1);
    lcd.fillScreen(COR_PRETO);
    Opcao op = opcoes->get(opcao_selecionada);
    lcd.setTextColor(COR_AZUL );
    lcd.setTextSize(1);
    lcd.setCursor(5, 5);
    lcd.print(op.nome); 
    lcd.fillRect(235, 1, 80, 16, COR_VERMELHO) ;
    lcd.setTextColor(COR_BRANCO);
    lcd.setCursor(243, 5);
    lcd.print("BTN: Voltar");          
    desenhaEixosEGrade();
    redesenhaCurva(op.cor);
    atualizaInfoEncoders();

    Serial.println("tela grafico");
}
void redesenhaCurva(uint16_t cor)
{
    Opcao op = opcoes->get(opcao_selecionada);
    for (int px = G_X0 + 1; px <= G_X1; px++)
    {
        // apaga o pixel antigo restaurando o fundo (grade/preto)
        int pyVelho = curvaY[px];
        lcd.drawPixel(px, pyVelho, corFundo(px, pyVelho));
        // usa o map prafazer a relacao pixel -> pontos 
        float posicao = (float)(px - (G_X0 + 1)) / (float)(G_X1 - (G_X0 + 1));
        int i = (int)(posicao * op.sinal.qtdPontos * op.freq) % op.sinal.qtdPontos;
        float yVal = op.sinal.valores[i] /100.0f;     
        int py = G_CY - (int)(yVal * op.amp);
        py = constrain(py, G_Y0, G_Y1 - 1);
        lcd.drawPixel(px, py, cor);
        curvaY[px] = py;                   // atualiza o buffer
    }
}
// devolve a cor de fundo correta de um pixel (grade ou preto)
// usado para "apagar" a curva antiga sem limpar a tela toda
uint16_t corFundo(int px, int py)
{
    if (py == G_CY) return COR_BRANCO;                 // eixo X
    if ((px - G_X0) % 40 == 0 && px > G_X0) return COR_GRADE; // linha vertical da grade
    if ((py - G_Y0) % 30 == 0) return COR_GRADE;          // linha horizontal da grade
    return COR_PRETO;
}

void desenhaEixosEGrade()
{
    for (int x = G_X0 + 40; x < G_X1; x += 40)
        lcd.drawLine(x, G_Y0, x, G_Y1, COR_GRADE);
    for (int y = G_Y0 + 30; y < G_Y1; y += 30)
        lcd.drawLine(G_X0, y, G_X1, y, COR_GRADE);
    lcd.drawLine(G_X0, G_CY, G_X1, G_CY, COR_BRANCO);
    lcd.drawLine(G_X0, G_Y0, G_X0, G_Y1, COR_BRANCO);
    lcd.setTextColor(ILI9341_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(G_X1 - 5, G_CY + 4); lcd.print("x");
    lcd.setCursor(G_X0 + 3, G_Y0 + 2); lcd.print("y");
}
// encoder 

void atualizaInfoEncoders()
{
    Opcao op = opcoes->get(opcao_selecionada);

    lcd.fillRect(0, G_Y1 + 1, G_X1, 15, COR_PRETO  );
    lcd.setTextColor(COR_INFO);
    lcd.setTextSize(1);
    lcd.setCursor(5, G_Y1 + 3);
    lcd.print("amplitude: ");
    lcd.print((int)op.amp);
    lcd.setCursor(G_X1 - 90, G_Y1 + 4);
    lcd.print("frequencia: ");
    lcd.print((int)op.freq);
}


// redesenho rapido: apaga so a curva antiga (coluna a coluna) e desenha a nova

void muda_tela(GFButton& btn)
{
     Serial.println(">>> MUDA_TELA FOI CHAMADA");
    if (millis() - ultimoMovimentoMenu < bloqueioCliqueAposGiro)
    {
        return;
    }

    if (telaAtual == 1) {
        telaAtual = 0;
        encY.setPosition(0);
        posMenu_ant = 0; posAmp_ant = 0;   // zera os dois "ant" pra nao herdar lixo da outra tela
        desenhaTelaSelecao();
    } else {
        telaAtual = 1;
        encY.setPosition(0);
        posMenu_ant = 0; posAmp_ant = 0;   // idem ao entrar no grafico
        desenhaTelaGrafico();
    }
}

void salvarConfiguracao( Opcao op) {
  //Serial.println("ENTREI EM salvarConfiguracao");
  int endereco = ultimoEndereco;
  byte tamNome = strlen(op.nome);
  EEPROM.put(endereco, tamNome);
  
  endereco += sizeof(byte);
  for (int i = 0; i < tamNome; i++) {
    EEPROM.put(endereco, op.nome[i]);
    endereco += sizeof(char);
  }
  EEPROM.put(endereco, op.sinal.qtdPontos);
  endereco += sizeof(int);
  for (int i = 0; i < op.sinal.qtdPontos; i++) {
    EEPROM.put(endereco, op.sinal.valores[i]);
    endereco += sizeof(float);
  }
  ultimoEndereco = endereco;
  EEPROM.put(2, ultimoEndereco);
  qtdSinais++;
  EEPROM.put(0, qtdSinais);
  Serial.println("Configuracao salva!");
}

void carregarConfiguracao() {

    int endereco = 4;

    for (int i = 0; i < qtdSinais; i++) {
        Serial.println("ENTREI EM carregarConfiguracao");
        Opcao temp;
        byte tamNome;
        EEPROM.get(endereco, tamNome);
        endereco += sizeof(byte);
        for (int j = 0; j < tamNome; j++) {
            EEPROM.get(endereco, temp.nome[j]);
            endereco += sizeof(char);
        }
        temp.nome[tamNome] = '\0';
        EEPROM.get(endereco, temp.sinal.qtdPontos);
        endereco += sizeof(int);
        for (int j = 0; j < temp.sinal.qtdPontos; j++) {
            EEPROM.get(endereco, temp.sinal.valores[j]);
            endereco += sizeof(float);
        }
        opcoes->add(temp);
        Serial.println("Nome:");
        Serial.println(temp.nome);
    }

    Serial.println("Lista carregada!");
}
// lixo do codigo do lorenzo, acho q n usa pra nada 
void calculaAmplitude(Sinal &s) {

  maximo = s.valores[0];
  minimo = s.valores[0];

  for (int i = 1; i < qtdValores; i++) {
    if (s.valores[i] > maximo) maximo = s.valores[i];
    if (s.valores[i] < minimo) minimo = s.valores[i];
  }

  amplitudeAtual = (maximo - minimo) / 2;
}

void LimparEprom(){
  qtdSinais = 0;
  EEPROM.put(0, qtdSinais);
  ultimoEndereco = 4;
  EEPROM.put(2, ultimoEndereco);
  Serial.println("EEPROM apagada!");
}

void printobj() {
  if (opcoes->size() == 0) {
    Serial.println("Lista vazia");
    Serial.println("Qtd de Sinais:");
    Serial.println(qtdSinais);
    return;
  }
  Opcao myObject = opcoes->get(opcoes->size() - 1);
  Serial.print("Nome: ");
  Serial.println(myObject.nome);
  Serial.print("Amplitude: ");
  Serial.println(myObject.amp);
  Serial.print("Frequencia: ");
  Serial.println(myObject.freq);
  Serial.print("Qtd pontos: ");
  Serial.println(myObject.sinal.qtdPontos);
  Serial.print("Valores: ");
  for (int i = 0; i < myObject.sinal.qtdPontos; i++) {
    Serial.print(myObject.sinal.valores[i]);
    if (i < myObject.sinal.qtdPontos - 1) {
      Serial.print(", ");
    }
  }
  Serial.println();
  Serial.print("Tamanho da lista opcoes: ");
  Serial.println(opcoes->size());
}
void calculaFrequencia() {

  int ciclos = 0;
  float ref = listaValores[0];

  for (int i = 1; i < qtdValores; i++) {
    if (listaValores[i] == ref) {
      ciclos++;
    }
  }
  frequenciaAtual = ciclos;
}

void reproduzirTabela(Sinal &s)
{

    unsigned long intervalo = 1000.0 / (frequenciaAtual);

    if(millis() - ultimoPonto >= intervalo)
    {
        ultimoPonto = millis();

        Serial.println(s.valores[indiceAtual]);

        indiceAtual++;

        if(indiceAtual >= s.qtdPontos)
            indiceAtual = 0;
    }
}

void setup()
{
    Serial.begin(9600);
    lcd.begin();                     // ILI9341 nao usa readID()
    pinMode(encY_SW, INPUT_PULLUP);  // pullup pro botao no Wokwi
    encY.setPosition(0);
    encX.setPosition(0);
    botao_enc.setPressHandler(muda_tela);
    carregarConfiguracao();
    desenhaTelaSelecao();
    EEPROM.get(0, qtdSinais);
    EEPROM.get(2, ultimoEndereco);
  if (ultimoEndereco == 0){
    ultimoEndereco = 4;
  }
  


}


void loop()
{   
    encY.tick();
    encX.tick();   // leitura do encoder por polling (substitui a interrupcao)
    botao_enc.process();
    long posY = encY.getPosition();
    long posX=encX.getPosition();
     if (Serial.available() > 0) {

    String texto = Serial.readStringUntil('\n');
    texto.trim();

    if (texto.startsWith("sinal ")) {

      texto = texto.substring(6);

      int p1 = texto.indexOf(";");
      int p2 = texto.indexOf(";", p1 + 1);

      nome = texto.substring(0, p1);
      funcao = texto.substring(p1 + 1, p2);
      String valores = texto.substring(p2 + 1);

      qtdValores = 0;
      while (valores.length() > 0) {
        int virgula = valores.indexOf(',');
        if (virgula == -1) {
          listaValores[qtdValores++] = valores.toFloat();
          break;
        }
        listaValores[qtdValores++] =
          valores.substring(0, virgula).toFloat();

        valores = valores.substring(virgula + 1);
      }
      Opcao op; 
      Sinal s;
      nome.toCharArray(op.nome, sizeof(op.nome));
      s.qtdPontos = qtdValores;
      for (int i = 0; i < qtdValores; i++) {
        valor = (int)(listaValores[i]*100.0f);
        valor= constrain(valor,-100,100);
        op.sinal.valores[i]=valor;
      }
      op.sinal.qtdPontos=s.qtdPontos;
      op.cor=COR_COS;
      op.freq=1.0f;
      op.amp=100.0f;111
      opcoes->add(op);
      salvarConfiguracao(op);
      opcao_selecionada = opcoes->size() - 1;
      atualizaOpcoes();
    }
    if (texto.startsWith("printar")) {
      printobj();
      
    }
    if (texto.startsWith("limpar")){
      LimparEprom();
    }
    if (texto.startsWith("plotar")) {
    for (int i = 0; i < opcoes->size(); i++) {
    String nome_curva = texto.substring(7);
    nome_curva.trim();

    Opcao obj = opcoes->get(i);

    if (nome_curva.equals(obj.nome)) {
      sinalAtual = obj.sinal;
      opcao_selecionada = i;
      indiceAtual = 0;
      ativo = true;
      break;
    }
    }
    }
    }
    if (telaAtual == 0)
    {
        int posMenu = posY / 4;

        if (posMenu != posMenu_ant && millis() - ultimoMovimentoMenu > intervaloMenu)
        {
            int delta = (posMenu > posMenu_ant) ? 1 : -1;

            // selecao circular: passa do fim volta pro inicio e vice-versa
            int total = opcoes->size();

            if (total == 0) return;

            opcao_selecionada = (opcao_selecionada + delta + total) % total;

            posMenu_ant = posMenu;

          if (opcao_selecionada < menu_offset)
            menu_offset = opcao_selecionada;

          if (opcao_selecionada >= menu_offset + OPCOES_VISIVEIS)
            menu_offset = opcao_selecionada - OPCOES_VISIVEIS + 1;

          if (opcao_selecionada == 0)
            menu_offset = 0;

          if (opcao_selecionada == total - 1)
            menu_offset = max(0, total - OPCOES_VISIVEIS);

            atualizaOpcoes();

        ultimoMovimentoMenu = millis();
        posAmp_ant = posY;
        }
    }
    else
    {
        if (posY != posAmp_ant)
        {
           Opcao op_amp= opcoes->get(opcao_selecionada);
            op_amp.amp += (posY - posAmp_ant) * 2.0f;
             op_amp.amp = constrain(
                 op_amp.amp,
                10.0f,
                120.0f
            );
            posAmp_ant = posY;
            // redesenho rapido: so a curva, sem limpar a area toda
            opcoes->set(opcao_selecionada, op_amp);
            redesenhaCurva( op_amp.cor);
            atualizaInfoEncoders();
        }
        if(posX!=posFreq_ant){
          Opcao op_freq= opcoes->get(opcao_selecionada);
          if(posX>posFreq_ant){
            op_freq.freq *= 1.1f; 
          }
          else{
             op_freq.freq *=0.9f;
          }
          op_freq.freq = constrain(
            op_freq.freq,
            0.1f,
            100.0f
          );
          opcoes->set(opcao_selecionada, op_freq);
          posFreq_ant = posX;
          redesenhaCurva(op_freq.cor);
          atualizaInfoEncoders();
        }
    }
     
    
}
