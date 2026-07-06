# Gerador de Sinais com Arduino, Display TFT e Interface Python

Projeto de um gerador/visualizador de sinais que utiliza uma interface em Python para cadastrar, plotar e enviar sinais para um Arduino. No Arduino, os sinais recebidos são armazenados, exibidos em um display TFT e podem ter amplitude e frequência ajustadas por encoders.

## Objetivo

O objetivo do projeto é permitir que o usuário:

- Cadastre sinais em uma interface gráfica Python;
- Escolha entre sinais predefinidos ou personalizados;
- Gere 100 pontos da função selecionada;
- Envie esses pontos para o Arduino pela porta serial;
- Armazene os sinais recebidos em uma lista no Arduino;
- Exiba os sinais em um display TFT;
- Ajuste amplitude e frequência usando encoders;
- Salve os sinais na EEPROM.

## Tecnologias utilizadas

### Arduino

- Arduino / Wokwi
- Display TFT ILI9341
- Biblioteca `Adafruit_GFX`
- Biblioteca `Adafruit_ILI9341`
- Biblioteca `MCUFRIEND_kbv`
- Biblioteca `RotaryEncoder`
- Biblioteca `GFButton`
- Biblioteca `EEPROM`
- Biblioteca `LinkedList`

### Python

- Python 3
- Tkinter
- PyMongo
- Matplotlib
- NumPy
- PySerial
- MongoDB

## Estrutura geral do projeto

O projeto é dividido em duas partes principais:

```txt
/arduino
  └── integração_final.c++

/python
  └── projeto_final_phyton.py
  └── projeto_final_phyton+arduino.py