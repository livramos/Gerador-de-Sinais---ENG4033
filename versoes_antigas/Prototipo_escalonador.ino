#include <RotaryEncoder.h> 
#include <EEPROM.h> 


RotaryEncoder encoder1(20, 21); 
RotaryEncoder encoder2(18, 19);
int posicaoAnterior_enc1 = 0;
int posicaoAnterior_enc2 = 0;
float frequenciaAtual = 1.0;
float ampitudeAtual = 1.0;
//EEPROM.get(endereco, minhaVariavel); 



void setup(void){
  Serial.begin(9600);
  int origem1 = digitalPinToInterrupt(20); 
  attachInterrupt(origem1, tickDoEncoder, CHANGE); 
  int origem2 = digitalPinToInterrupt(21); 
  attachInterrupt(origem2, tickDoEncoder, CHANGE); 
  int origem3 = digitalPinToInterrupt(20); 
  attachInterrupt(origem3, tickDoEncoder, CHANGE); 
  int origem4 = digitalPinToInterrupt(21); 
  attachInterrupt(origem4, tickDoEncoder, CHANGE);
}

void tickDoEncoder() {
  encoder1.tick();
  encoder2.tick();
}

void loop() {
  int posicao_freq = encoder1.getPosition();
  if (posicao_freq > posicaoAnterior_enc1) {
    frequenciaAtual += 0.1;
  }
  else if(posicao_freq < posicaoAnterior_enc1 && frequenciaAtual > 0.0){ 
    frequenciaAtual -= 0.1;
  }
  posicaoAnterior_enc1 = posicao_freq;
  int posicao_amp = encoder2.getPosition();
  if (posicao_amp > posicaoAnterior_enc2) {
    amplitudeAtual += 0.1;
  }
  else if(posicao_amp < posicaoAnterior_enc2 && amplitudeAtual > 0.0){
    amplitudeAtual -= 0.1;
  }
  posicaoAnterior_enc2 = posicao_amp;

  float seno = amplitudeAtual*sin(frequenciaAtual);
  Serial.println(seno);
  Serial.print(" ");
  delay(10);
}