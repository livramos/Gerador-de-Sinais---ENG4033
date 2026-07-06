#include <RotaryEncoder.h>
#include <math.h>
#include <EEPROM.h>
#include <LinkedList.h>

RotaryEncoder encoder1(20, 21);
RotaryEncoder encoder2(18, 19);

int posicaoAnterior_enc1 = 0;
int posicaoAnterior_enc2 = 0;
int indiceAtual = 0;
unsigned long ultimoPonto = 0;

struct Sinal {
  char nome[20];
  float valores[100];
  int qtdPontos;
};

int seno40[] = {
     0,  16,  31,  45,  59,
    71,  81,  89,  95,  99,
   100,  99,  95,  89,  81,
    71,  59,  45,  31,  16,
     0, -16, -31, -45, -59,
   -71, -81, -89, -95, -99,
  -100, -99, -95, -89, -81,
   -71, -59, -45, -31, -16
};


int ultimoEndereco = 4;
int qtdSinais = 0;

Sinal sinalAtual;
bool ativo = false;

float frequenciaAtual = 1.0;
float amplitudeAtual = 0;

LinkedList<Sinal> *sinais = new LinkedList<Sinal>();

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

void setup() {
  Serial.begin(9600);
  Serial.println("Oi");
  attachInterrupt(digitalPinToInterrupt(20), tickDoEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(21), tickDoEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(18), tickDoEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(19), tickDoEncoder, CHANGE);
  EEPROM.get(0, qtdSinais);
  EEPROM.get(2, ultimoEndereco);
  if (ultimoEndereco == 0){
    ultimoEndereco = 4;
  }
  carregarConfiguracao();
}

void salvarConfiguracao(Sinal s) {
  Serial.println("ENTREI EM salvarConfiguracao");
  int endereco = ultimoEndereco;
  byte tamNome = strlen(s.nome);
  EEPROM.put(endereco, tamNome);
  
  Serial.println("Endereco de tamNome:");
  Serial.println(endereco);

  Serial.println("Tamanho nome:");
  Serial.println(tamNome);
  
  
  endereco += sizeof(byte);
  for (int i = 0; i < tamNome; i++) {
    EEPROM.put(endereco, s.nome[i]);
    endereco += sizeof(char);
  }
  EEPROM.put(endereco, s.qtdPontos);
  endereco += sizeof(int);
  Serial.println(endereco);
  for (int i = 0; i < s.qtdPontos; i++) {
    EEPROM.put(endereco, s.valores[i]);
    Serial.println(s.valores[i]);
    endereco += sizeof(float);
  }
  ultimoEndereco = endereco;
  EEPROM.put(2, ultimoEndereco);
  Serial.println("Ultimo Endereco:");
  Serial.println(ultimoEndereco);
  qtdSinais++;
  EEPROM.put(0, qtdSinais);

  Serial.println("Configuracao salva!");
}

void LimparEprom(){
  qtdSinais = 0;
  EEPROM.put(0, qtdSinais);
  ultimoEndereco = 4;
  EEPROM.put(2, ultimoEndereco);
  Serial.println("EEPROM apagada!");
}

void carregarConfiguracao() {

    int endereco = 4;

    for (int i = 0; i < qtdSinais; i++) {
        Serial.println("ENTREI EM carregarConfiguracao");
        Sinal temp;
        byte tamNome;
        Serial.println("1:");
        Serial.println(endereco);
        EEPROM.get(endereco, tamNome);
        Serial.println("tamNome:");
        Serial.println(tamNome);
        endereco += sizeof(byte);
        Serial.println("2:");
        Serial.println(endereco);
        for (int j = 0; j < tamNome; j++) {
            EEPROM.get(endereco, temp.nome[j]);
            endereco += sizeof(char);
        }
        Serial.println(temp.nome);
        temp.nome[tamNome] = '\0';

        EEPROM.get(endereco, temp.qtdPontos);
        endereco += sizeof(int);
        Serial.println("3:");
        Serial.println(endereco);
        for (int j = 0; j < temp.qtdPontos; j++) {
            EEPROM.get(endereco, temp.valores[j]);
            endereco += sizeof(float);
        }
        sinais->add(temp);
        Serial.println("Ultimo endereco: ");
        Serial.println(endereco);

    }

    Serial.println("Lista carregada!");
}

void printobj() {

  if (sinais->size() == 0) {
    Serial.println("Lista vazia");
    Serial.println("Qtd de Sinais:");
    Serial.println(qtdSinais);
    return;
  }

  Sinal myObject = sinais->get(sinais->size() - 1);
  Serial.print("Nome: ");
  Serial.println(myObject.nome);
  Serial.print("Qtd pontos: ");
  Serial.println(myObject.qtdPontos);
  Serial.print("Valores: ");
  for (int i = 0; i < myObject.qtdPontos; i++) {
    Serial.print(myObject.valores[i]);
    if (i < myObject.qtdPontos - 1)
      Serial.print(", ");
  }
  Serial.println();
  Serial.println(sinais->size());
}

void tickDoEncoder() {
  encoder1.tick();
  encoder2.tick();
}

void calculaAmplitude(Sinal &s) {

  maximo = s.valores[0];
  minimo = s.valores[0];

  for (int i = 1; i < qtdValores; i++) {
    if (s.valores[i] > maximo) maximo = s.valores[i];
    if (s.valores[i] < minimo) minimo = s.valores[i];
  }

  amplitudeAtual = (maximo - minimo) / 2;
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
/*
void senoidal() {

  if (millis() - instante_anterior >= intervalo) {
    instante_anterior = millis();

    value = amplitudeAtual * sin(2 * PI * frequenciaAtual * t);

    Serial.println(value);

    t += 0.01;
    if (t >= 1.0) t = 0;
  }
}

void quadrado() {

  unsigned long halfPeriod = 1000 / (frequenciaAtual * 2);

  if (millis() - instante_anterior >= halfPeriod) {
    instante_anterior = millis();
    state = !state;
  }

  Serial.println(state ? amplitudeAtual : -amplitudeAtual);
}

void triangulo() {

  unsigned long intervaloTri =
    1000 / (frequenciaAtual * amplitudeAtual * 2);

  if (millis() - instante_anterior >= intervaloTri) {
    instante_anterior = millis();

    Serial.println(value);

    value += dir;

    if (value >= amplitudeAtual || value <= -amplitudeAtual) {
      dir = -dir;
    }
  }
}

void serra() {

  unsigned long period =
    1000 / (frequenciaAtual * amplitudeAtual);

  if (millis() - instante_anterior >= period) {
    instante_anterior = millis();

    Serial.println(saw);

    saw++;

    if (saw >= amplitudeAtual)
      saw = 0;
  }
}
*/
void loop() {

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

      //calculaAmplitude();
      //calculaFrequencia();

      Sinal s;
      nome.toCharArray(s.nome, sizeof(s.nome));
      s.qtdPontos = qtdValores;
      for (int i = 0; i < qtdValores; i++) {
        s.valores[i] = (int)listaValores[i];
      }
      salvarConfiguracao(s);
      sinais->add(s);

    }

    if (texto.startsWith("printar")) {
      printobj();
      
    }
    if (texto.startsWith("plotar")){
      for (int i = 0; i < sinais->size(); i++){
        String nome_curva = texto.substring(7);
        nome_curva.trim();
        Sinal obj = sinais->get(i);
        if (nome_curva.equals(obj.nome)) {
        sinalAtual = obj;
        indiceAtual = 0;
        ativo = true;
        break;
        }
      }
    }
    if (texto.startsWith("limpar")){
      LimparEprom();
    }
    if (texto.startsWith("parar")){
      ativo = false;
    }
  }
  if (ativo) {
  reproduzirTabela(sinalAtual);
  int pos_freq = encoder1.getPosition();
  if (pos_freq != posicaoAnterior_enc1) {
    frequenciaAtual *= (pos_freq > posicaoAnterior_enc1) ? 1.1 : 0.9;
    if (frequenciaAtual < 0.1)
      frequenciaAtual = 0.1;
    posicaoAnterior_enc1 = pos_freq;
  }

  int pos_amp = encoder2.getPosition();
  calculaAmplitude(sinalAtual);
  if (pos_amp > posicaoAnterior_enc2)
    for(int i=0;i < sinalAtual.qtdPontos;i++)
    {
        sinalAtual.valores[i] *= 1.1;
    }
  else if (pos_amp < posicaoAnterior_enc2)
    for(int i=0;i < sinalAtual.qtdPontos;i++)
    {
        sinalAtual.valores[i] *= 0.9;
    }
  posicaoAnterior_enc2 = pos_amp;
  }
}
