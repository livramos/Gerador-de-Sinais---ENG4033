#include <RotaryEncoder.h>
#include <math.h>
#include <EEPROM.h>

RotaryEncoder encoder1(20, 21);
RotaryEncoder encoder2(18, 19);

int posicaoAnterior_enc1 = 0;
int posicaoAnterior_enc2 = 0;

struct Sinal {
  char nome[20];
  int valores[100];
  int qtdPontos;
};

int ultimoEndereco = 8;
int qtdSinais = 0;

float frequenciaAtual = 1.0;
float amplitudeAtual = 0;

float listaValores[100];
int qtdValores = 0;
float minimo;
float maximo;

String nome = "";
String funcao = "";

bool state = false;

float t = 0;
float value = 0;

unsigned long instante_anterior = 0;
unsigned long intervalo = 100;

int dir = 1;
float saw = 0;

void setup() {
  Serial.begin(9600);

  attachInterrupt(digitalPinToInterrupt(20), tickDoEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(21), tickDoEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(18), tickDoEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(19), tickDoEncoder, CHANGE);

  randomSeed(analogRead(0));

  EEPROM.get(0, qtdSinais);
  EEPROM.get(4, ultimoEndereco);
}

// ================= EEPROM SALVAR =================
void salvarConfiguracao(Sinal s) {
  int endereco = ultimoEndereco;
  byte tamNome = strlen(s.nome);
  EEPROM.put(endereco, tamNome);
  endereco += sizeof(byte);
  for (int i = 0; i < tamNome; i++) {
    EEPROM.put(endereco, s.nome[i]);
    endereco += sizeof(char);
  }
  EEPROM.put(endereco, s.qtdPontos);
  endereco += sizeof(int);
  for (int i = 0; i < s.qtdPontos; i++) {
    EEPROM.put(endereco, s.valores[i]);
    endereco += sizeof(int);
  }
  ultimoEndereco = endereco;
  EEPROM.put(4, ultimoEndereco);
  qtdSinais++;
  EEPROM.put(0, qtdSinais);

  Serial.println("Configuracao salva!");
}

void carregarConfiguracao(const char* nomeBusca) {

  int endereco = 8;

  for (int i = 0; i < qtdSinais; i++) {
    byte tamNome;
    EEPROM.get(endereco, tamNome);
    endereco += sizeof(byte);
    char nomeLido[20];
    for (int j = 0; j < tamNome; j++) {
      EEPROM.get(endereco, nomeLido[j]);
      endereco += sizeof(char);
    }
    nomeLido[tamNome] = '\0';
    int qtd;
    EEPROM.get(endereco, qtd);
    endereco += sizeof(int);
    if (strcmp(nomeLido, nomeBusca) == 0) {
      nome = String(nomeLido);
      qtdValores = qtd;
      for (int j = 0; j < qtdValores; j++) {
        EEPROM.get(endereco, listaValores[j]);
        endereco += sizeof(int);
      }
      Serial.println("Sinal carregado!");
      return;
    }
    endereco += qtd * sizeof(int);
  }

  Serial.println("Sinal nao encontrado!");
}

void tickDoEncoder() {
  encoder1.tick();
  encoder2.tick();
}

void calculaAmplitude() {

  maximo = listaValores[0];
  minimo = listaValores[0];

  for (int i = 1; i < qtdValores; i++) {
    if (listaValores[i] > maximo) maximo = listaValores[i];
    if (listaValores[i] < minimo) minimo = listaValores[i];
  }

  amplitudeAtual = maximo - minimo;
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

void loop() {

  if (Serial.available() > 0) {

    String texto = Serial.readStringUntil('\n');
    texto.trim();

    if (texto.startsWith("sinal ")) {

      texto = texto.substring(6);

      int p1 = texto.indexOf(';');
      int p2 = texto.indexOf(';', p1 + 1);

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

      calculaAmplitude();
      calculaFrequencia();

      Sinal s;
      nome.toCharArray(s.nome, sizeof(s.nome));
      s.qtdPontos = qtdValores;
      for (int i = 0; i < qtdValores; i++) {
        s.valores[i] = (int)listaValores[i];
      }
      salvarConfiguracao(s);
    }

    if (texto.startsWith("carregar")) {

      texto = texto.substring(9);
      carregarConfiguracao(texto.c_str());
    }
  }

  int pos_freq = encoder1.getPosition();

  if (pos_freq > posicaoAnterior_enc1)
    frequenciaAtual += 0.1;
  else if (frequenciaAtual > 0)
    frequenciaAtual -= 0.1;

  posicaoAnterior_enc1 = pos_freq;

  int pos_amp = encoder2.getPosition();

  if (pos_amp > posicaoAnterior_enc2)
    amplitudeAtual += 0.1;
  else if (amplitudeAtual > 0)
    amplitudeAtual -= 0.1;

  posicaoAnterior_enc2 = pos_amp;

  if (funcao == "senoidal") senoidal();
  else if (funcao == "quadrado") quadrado();
  else if (funcao == "triangulo") triangulo();
  else if (funcao == "serra") serra();
}