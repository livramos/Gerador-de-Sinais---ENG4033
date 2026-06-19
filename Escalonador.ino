#include <RotaryEncoder.h>
#include <math.h>
#include <EEPROM.h>

RotaryEncoder encoder1(20, 21);
RotaryEncoder encoder2(18, 19);

int posicaoAnterior_enc1 = 0;
int posicaoAnterior_enc2 = 0;

float frequenciaAtual = 1.0; //ciclos por segundo
float minimo = 0;
float maximo = 0;
float amplitudeAtual = 0;


String funcao = "";
char nomeFuncao[20];
String trecho;
bool state = false;

float t = 0; //
float value = 0;

unsigned long instante_anterior = 0;
unsigned long intervalo = 100; // 10ms = 100 Hz de amostragem
unsigned long instante_atual = 0;

int dir = 1;
float saw = 0;

void setup(void)
{
    Serial.begin(9600);

    attachInterrupt(digitalPinToInterrupt(20), tickDoEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(21), tickDoEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(18), tickDoEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(19), tickDoEncoder, CHANGE);
    randomSeed(analogRead(0));
}

void salvarConfiguracao()
{
    int endereco = random(4095);

    EEPROM.put(endereco, frequenciaAtual);
    EEPROM.put(endereco, minimo);
    EEPROM.put(endereco, maximo);
    
    funcao.toCharArray(nomeFuncao, sizeof(nomeFuncao));
    EEPROM.put(endereco, nomeFuncao);

    Serial.println("Configuracao da curva salva!");
    Serial.println("Endereco: ");
    Serial.println(endereco);
}


void carregarConfiguracao(int num)
{
    int endereco = num;

    EEPROM.get(endereco, frequenciaAtual);
    EEPROM.get(endereco, minimo);
    EEPROM.get(endereco, maximo);
    char nomeFuncao[20];

    EEPROM.get(endereco, nomeFuncao);

    funcao = String(nomeFuncao);
    Serial.println("Configuracao da curva carregada!");

    Serial.print("Frequencia: ");
    Serial.println(frequenciaAtual);

    Serial.print("Minimo: ");
    Serial.println(minimo);

    Serial.print("Maximo: ");
    Serial.println(maximo);

    Serial.print("Funcao: ");
    Serial.println(funcao);
}

void tickDoEncoder()
{
    encoder1.tick();
    encoder2.tick();
}

void senoidal()
{
    instante_atual = millis();

    if (instante_atual - instante_anterior >= intervalo)
    {
        instante_anterior = instante_atual;

        value = amplitudeAtual * sin(2 * PI * frequenciaAtual * t);

        Serial.println(value);

        t += 0.01;

        if (t == 1.0){
          t = 0;
        }
    }
}

void quadrado()
{
    unsigned long halfPeriod =
        1000 / (frequenciaAtual * 2);

    instante_atual = millis();

    if (instante_atual - instante_anterior >= halfPeriod)
    {
        instante_anterior = instante_atual;
        state = !state;
    }

    Serial.println(state ? amplitudeAtual : -amplitudeAtual);
}


void triangulo()
{
    unsigned long intervaloTri =
        1000 / (frequenciaAtual * amplitudeAtual * 2);

    instante_atual = millis();

    if (instante_atual - instante_anterior >= intervaloTri)
    {
        instante_anterior = instante_atual;

        Serial.println(value);

        value += dir;

        if (value >= amplitudeAtual ||
            value <= -amplitudeAtual)
        {
            dir = -dir;
        }
    }
}

void serra()
{
    unsigned long period =
        1000 / (frequenciaAtual * amplitudeAtual);

    instante_atual = millis();

    if (instante_atual - instante_anterior >= period)
    {
        instante_anterior = instante_atual;

        Serial.println(saw);

        saw++;

        if (saw >= amplitudeAtual)
            saw = 0;
    }
}

void loop()
{
    if (Serial.available() > 0)
    {
        String texto =
            Serial.readStringUntil('\n');

        texto.trim();

        if (texto.startsWith("frequencia"))
        {
            trecho = texto.substring(11);
            frequenciaAtual = trecho.toFloat();
            Serial.println(trecho);
        }
        else if (texto.startsWith("minimo:"))
        {
        minimo =
            texto.substring(7).toFloat();

        Serial.print("Minimo = ");
        Serial.println(minimo);
        }
        else if (texto.startsWith("maximo:"))
        {
            maximo =
                texto.substring(7).toFloat();

            Serial.print("Maximo = ");
            Serial.println(maximo);
        }
        else if (texto.startsWith("funcao"))
        {
            trecho = texto.substring(7);
            funcao = trecho;
            Serial.println(trecho);
        }
        else if (texto.startsWith("load"))
        {
            trecho = texto.substring(5);
            int num = trecho.toInt();
            carregarConfiguracao(num);
        }
        salvarConfiguracao();
        amplitudeAtual = maximo - minimo;
    }

    int posicao_freq = encoder1.getPosition();

    if (posicao_freq > posicaoAnterior_enc1)
    {
        frequenciaAtual += 0.1;
    }
    else if (posicao_freq < posicaoAnterior_enc1 &&
             frequenciaAtual > 0.0)
    {
        frequenciaAtual -= 0.1;
    }

    posicaoAnterior_enc1 = posicao_freq;

    int posicao_amp = encoder2.getPosition();

    if (posicao_amp > posicaoAnterior_enc2)
    {
        amplitudeAtual += 0.1;
        Serial.println(amplitudeAtual);
    }
    else if (posicao_amp < posicaoAnterior_enc2 &&
             amplitudeAtual > 0.0)
    {
        amplitudeAtual -= 0.1;
        Serial.println(amplitudeAtual);
    }

    posicaoAnterior_enc2 = posicao_amp;

    if (funcao == "senoidal")
    {
        senoidal();
    }
    else if (funcao == "quadrado")
    {
        quadrado();
    }
    else if (funcao == "triangulo")
    {
        triangulo();
    }
    else if (funcao == "serra")
    {
        serra();
    }

    delay(10);
}
