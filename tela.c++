#include <Arduino.h>   // necessario em .cpp (o .ino adiciona isso automaticamente)
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>   // driver SPI do display (Wokwi)
#include <SPI.h>
#include <RotaryEncoder.h>
#include <GFButton.h>

// --- pinos do display SPI ---
#define TFT_CS  10
#define TFT_DC   9
#define TFT_RST  8

#define COR_GRADE 0x2945
#define COR_SEN   ILI9341_GREEN
#define COR_COS   ILI9341_MAGENTA
#define COR_TRI   ILI9341_CYAN     // cor do novo sinal triangular
#define COR_DENTE ILI9341_ORANGE   // cor do novo sinal dente de serra
#define COR_INFO  ILI9341_YELLOW

#define encY_CLK  21 
#define encY_DT   20
#define encY_SW   A12  

#define G_X0   35
#define G_X1  315
#define G_Y0   25
#define G_Y1  210
#define G_CY  117


typedef float (*FuncOnda)(float);

float funcSeno(float x)    { return sin(x); }
float funcCosseno(float x) { return cos(x); }
float funcQuadrado(float x) {
    if (sin(x) >= 0) {
        return 1.0f;
    } else {
        return -1.0f;
    }
}
// onda triangular: sobe e desce linearmente entre -1 e 1
float funcTriangular(float x) {
    float f = x / (2.0f * PI);          // fase normalizada
    f = f - floor(f);                   // mantem entre 0 e 1
    return 4.0f * fabs(f - 0.5f) - 1.0f;
}
// dente de serra: rampa de -1 a 1 e cai de volta
float funcDenteSerra(float x) {
    float f = x / (2.0f * PI);
    f = f - floor(f);
    return 2.0f * f - 1.0f;
}

struct Opcao {
    const char* nome;
    uint16_t    cor;
    float       amp;
    float       freq;
    FuncOnda    func;
};

// lista de sinais: para adicionar um novo, basta criar a funcao acima
// e acrescentar uma linha aqui. todo o resto se ajusta sozinho.
Opcao opcoes[] = {
    { "Seno",     COR_SEN,   80.0f, 0.02f, funcSeno       },
    { "Cosseno",  COR_COS,   80.0f, 0.02f, funcCosseno    },
    { "Quadrado", COR_SEN,   80.0f, 0.02f, funcQuadrado   },
    { "Triangular", COR_TRI, 80.0f, 0.02f, funcTriangular },
    { "Dente",    COR_DENTE, 80.0f, 0.02f, funcDenteSerra }
};
const int NUM_OPCOES      = sizeof(opcoes) / sizeof(opcoes[0]);
const int OPCOES_VISIVEIS = 3; 

GFButton botao_enc(encY_SW);  

Adafruit_ILI9341 lcd(TFT_CS, TFT_DC, TFT_RST);  // objeto do display SPI

RotaryEncoder encY(encY_CLK, encY_DT, RotaryEncoder::LatchMode::TWO03);

int  telaAtual         = 0;
int  opcao_selecionada = 0;
int  menu_offset       = 0;  
int  opcao_ant_desenhada = -1;   // controla redesenho parcial do menu
long pos_ant           = 0;
int posMenu_ant = 0;
long posAmp_ant = 0;
unsigned long ultimoMovimentoMenu = 0;
const unsigned long intervaloMenu = 100;
const unsigned long bloqueioCliqueAposGiro = 200;

// buffer da curva desenhada: guarda o Y de cada coluna X para apagar
// apenas a curva antiga em vez de limpar a tela inteira (mais rapido, sem flicker)
int curvaY[G_X1 + 1];
bool curvaValida = false;

void desenhaTelaSelecao();
void desenhaTelaGrafico();
void atualizaOpcoes();
void desenhaOpcao(int idx, const char *nome, uint16_t cor, int y);
void desenhaEixosEGrade();
void plotaFuncao(uint16_t cor);
void redesenhaCurva(uint16_t cor);
void atualizaInfoEncoders();
void muda_tela(GFButton& btn);

void desenhaTelaSelecao()
{
    lcd.setRotation(0);
    lcd.fillScreen(ILI9341_BLACK);
    lcd.setTextColor(ILI9341_CYAN);
    lcd.setTextSize(2);
    lcd.setCursor(30, 8);
    lcd.print("Escolha o Sinal");
    lcd.drawLine(10, 38, 230, 38, ILI9341_WHITE);
    opcao_ant_desenhada = -1;   // forca desenho completo da lista
    atualizaOpcoes();
    lcd.fillRect(40, 250, 160, 30, ILI9341_BLUE);
    lcd.drawRect(40, 250, 160, 30, ILI9341_WHITE);
    lcd.setTextColor(ILI9341_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(60, 262);
    lcd.print("BTN: Confirmar");     
    lcd.setTextColor(ILI9341_LIGHTGREY);
    lcd.setCursor(45, 295);
    lcd.print("SCROLL: trocar sinal"); 
    Serial.println("tela selecao");
}

// desenha a lista a partir do array opcoes[], com viewport rolavel
void atualizaOpcoes()
{
    lcd.fillRect(10, 50, 220, OPCOES_VISIVEIS * 70, ILI9341_BLACK);
    for (int i = 0; i < OPCOES_VISIVEIS; i++) {
        int idx = menu_offset + i;
        if (idx >= NUM_OPCOES) break;
        desenhaOpcao(idx, opcoes[idx].nome, opcoes[idx].cor, 50 + i * 70);
    }
    opcao_ant_desenhada = opcao_selecionada;
}

void desenhaOpcao(int idx, const char *nome, uint16_t cor, int y)
{
    bool sel = (opcao_selecionada == idx);
    lcd.setTextColor(sel ? cor : ILI9341_WHITE);
    lcd.setTextSize(2);
    lcd.setCursor(25, y + 17);
    lcd.print(sel ? "> " : "  ");
    lcd.print(nome);
}

void desenhaTelaGrafico()
{
    lcd.setRotation(1);
    lcd.fillScreen(ILI9341_BLACK);
    lcd.setTextColor(ILI9341_CYAN);
    lcd.setTextSize(1);
    lcd.setCursor(5, 5);
    lcd.print(opcoes[opcao_selecionada].nome); 
    lcd.fillRect(235, 1, 80, 16, ILI9341_RED);
    lcd.setTextColor(ILI9341_WHITE);
    lcd.setCursor(243, 5);
    lcd.print("BTN: Voltar");          
    desenhaEixosEGrade();
    curvaValida = false;             // primeira plotagem desenha do zero
    plotaFuncao(opcoes[opcao_selecionada].cor);
    atualizaInfoEncoders();
    Serial.println("tela grafico");
}

void desenhaEixosEGrade()
{
    for (int x = G_X0 + 40; x < G_X1; x += 40)
        lcd.drawLine(x, G_Y0, x, G_Y1, COR_GRADE);
    for (int y = G_Y0 + 30; y < G_Y1; y += 30)
        lcd.drawLine(G_X0, y, G_X1, y, COR_GRADE);
    lcd.drawLine(G_X0, G_CY, G_X1, G_CY, ILI9341_WHITE);
    lcd.drawLine(G_X0, G_Y0, G_X0, G_Y1, ILI9341_WHITE);
    lcd.setTextColor(ILI9341_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(G_X1 - 5, G_CY + 4); lcd.print("x");
    lcd.setCursor(G_X0 + 3, G_Y0 + 2); lcd.print("y");
}

void atualizaInfoEncoders()
{
    lcd.fillRect(0, G_Y1 + 1, G_X1, 15, ILI9341_BLACK);
    lcd.setTextColor(COR_INFO);
    lcd.setTextSize(1);
    lcd.setCursor(5, G_Y1 + 3);
    lcd.print("zoom: ");
    lcd.print((int)opcoes[opcao_selecionada].amp);
}

// devolve a cor de fundo correta de um pixel (grade ou preto)
// usado para "apagar" a curva antiga sem limpar a tela toda
uint16_t corFundo(int px, int py)
{
    if (py == G_CY) return ILI9341_WHITE;                 // eixo X
    if ((px - G_X0) % 40 == 0 && px > G_X0) return COR_GRADE; // linha vertical da grade
    if ((py - G_Y0) % 30 == 0) return COR_GRADE;          // linha horizontal da grade
    return ILI9341_BLACK;
}

// primeira plotagem completa: calcula e guarda a curva no buffer
void plotaFuncao(uint16_t cor)
{
    Opcao& op = opcoes[opcao_selecionada]; 
    int yAnterior = -1;
    for (int px = G_X0 + 1; px <= G_X1; px++)
    {
        float xRad = (px - G_X0) * op.freq;
        float yVal = op.func(xRad);
        int py = G_CY - (int)(yVal * op.amp);
        py = constrain(py, G_Y0, G_Y1 - 1);
        curvaY[px] = py;                       // guarda para o redesenho parcial
        if (yAnterior >= 0)
            lcd.drawLine(px - 1, yAnterior, px, py, cor);
        yAnterior = py;
    }
    curvaValida = true;
}

// redesenho rapido: apaga so a curva antiga (coluna a coluna) e desenha a nova
void redesenhaCurva(uint16_t cor)
{
    Opcao& op = opcoes[opcao_selecionada];
    int yNovoAnt = -1;
    for (int px = G_X0 + 1; px <= G_X1; px++)
    {
        // apaga o pixel antigo restaurando o fundo (grade/preto)
        int pyVelho = curvaY[px];
        lcd.drawPixel(px, pyVelho, corFundo(px, pyVelho));

        // calcula o novo ponto
        float xRad = (px - G_X0) * op.freq;
        float yVal = op.func(xRad);
        int pyNovo = G_CY - (int)(yVal * op.amp);
        pyNovo = constrain(pyNovo, G_Y0, G_Y1 - 1);

        if (yNovoAnt >= 0)
            lcd.drawLine(px - 1, yNovoAnt, px, pyNovo, cor);
        yNovoAnt = pyNovo;
        curvaY[px] = pyNovo;                   // atualiza o buffer
    }
}

void muda_tela(GFButton& btn)
{
     Serial.println(">>> MUDA_TELA FOI CHAMADA");
    if (millis() - ultimoMovimentoMenu < bloqueioCliqueAposGiro)
    {
        return;
    }

    if (telaAtual == 1) {
        telaAtual = 0;
        encY.setPosition(0); pos_ant = 0;
        posMenu_ant = 0; posAmp_ant = 0;   // zera os dois "ant" pra nao herdar lixo da outra tela
        desenhaTelaSelecao();
    } else {
        telaAtual = 1;
        encY.setPosition(0); pos_ant = 0;
        posMenu_ant = 0; posAmp_ant = 0;   // idem ao entrar no grafico
        desenhaTelaGrafico();
    }
}

void setup()
{
    Serial.begin(9600);
    lcd.begin();                     // ILI9341 nao usa readID()
    pinMode(encY_SW, INPUT_PULLUP);  // pullup pro botao no Wokwi
    encY.setPosition(0);
    botao_enc.setPressHandler(muda_tela);
    desenhaTelaSelecao();
}


void loop()
{
    encY.tick();   // leitura do encoder por polling (substitui a interrupcao)
    botao_enc.process();
    long posY = encY.getPosition();
    if (telaAtual == 0)
    {
        int posMenu = posY / 4;

        if (posMenu != posMenu_ant && millis() - ultimoMovimentoMenu > intervaloMenu)
        {
            int delta = (posMenu > posMenu_ant) ? 1 : -1;

            // selecao circular: passa do fim volta pro inicio e vice-versa
            opcao_selecionada = (opcao_selecionada + delta + NUM_OPCOES) % NUM_OPCOES;

            posMenu_ant = posMenu;

            // viewport rolavel: mantem a opcao selecionada sempre visivel
            if (opcao_selecionada < menu_offset)
                menu_offset = opcao_selecionada;
            if (opcao_selecionada >= menu_offset + OPCOES_VISIVEIS)
                menu_offset = opcao_selecionada - OPCOES_VISIVEIS + 1;
            // ao dar a volta (fim->inicio), reposiciona o viewport
            if (opcao_selecionada == 0) menu_offset = 0;
            if (opcao_selecionada == NUM_OPCOES - 1)
                menu_offset = max(0, NUM_OPCOES - OPCOES_VISIVEIS);

            atualizaOpcoes();

            ultimoMovimentoMenu = millis();
            posAmp_ant = posY;
        }
    }
    else
    {
        if (posY != posAmp_ant)
        {
            opcoes[opcao_selecionada].amp += (posY - posAmp_ant) * 2.0f;
            opcoes[opcao_selecionada].amp = constrain(
                opcoes[opcao_selecionada].amp,
                10.0f,
                120.0f
            );
            posAmp_ant = posY;
            // redesenho rapido: so a curva, sem limpar a area toda
            redesenhaCurva(opcoes[opcao_selecionada].cor);
            atualizaInfoEncoders();
        }
    }
}