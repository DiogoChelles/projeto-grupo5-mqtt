//*main.cpp

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

#include "WiFiManager.h"
#include "MqttManager.h"
#include "DebugManager.h"

//*============================
//*       Constantes
//*============================

const int pinoLampada = 17;
const int PinoLedRGB = 48;
const int QntLeds = 1;
const char TOPICO_COMANDO[] = "senai134/luanda/esp32/comando";

//*============================
//*       Instancias
//*============================
Adafruit_NeoPixel ledRGB(
    QntLeds, PinoLedRGB,
    NEO_GRB + NEO_KHZ800);

//*============================
//*   Protótipo das funções
//*============================
void tratarMensagemRecebida(const char *topico, const String &mensagem);
void alterarCorDoLedRGB(int vermelho, int verde, int azul);
void configurarLedRGB();
void tratarJsonComando(const String &mensagem);
void switchLampada(bool estado);

void setup()
{
  configurarDebug();
  conectarWiFi();
  configurarMQTT();
  registrarCallbackMensagem(tratarMensagemRecebida);
  conectarMQTT();
  configurarLedRGB();
  pinMode(17, OUTPUT);
}

void loop()
{
  garantirWiFiConectado();
  garantirMQTTConectado();
  loopMQTT();
}

void tratarMensagemRecebida(const char *topico, const String &mensagem)
{
  debugInfo("========================");
  debugInfo("Mensagem recebida na aplicação.");
  debugInfo("========================");

  if (topico == nullptr)
  {
    debugErro("Tópico MQTT inválido");
    return;
  }

  debugInfo("Tópico: " + String(topico));
  debugInfo("Mensagem: " + String(mensagem));

  if (strcmp(topico, TOPICO_COMANDO) == 0)
  {
    tratarJsonComando(mensagem);
    return;
  }
  debugErro("Tópico não tratado: " + String(topico));
}

void configurarLedRGB()
{
  ledRGB.begin();
  ledRGB.setBrightness(80);
  ledRGB.clear();
  ledRGB.show();

  debugInfo("LED RGB configurado no pino" + String(PinoLedRGB));
}

void alterarCorDoLedRGB(int vermelho, int verde, int azul)
{
  vermelho = constrain(vermelho, 0, 255);
  verde = constrain(verde, 0, 255);
  azul = constrain(azul, 0, 255);

  ledRGB.setPixelColor(0, ledRGB.Color(vermelho, verde, azul));
  ledRGB.show();

  debugInfo("Cor aplicada no LED RGB. ");
  debugInfo("R: " + String(vermelho));
  debugInfo("G: " + String(verde));
  debugInfo("B: " + String(azul));
}

void tratarJsonComando(const String &mensagem)
{
  JsonDocument doc;

  DeserializationError erro = deserializeJson(doc, mensagem);

  if (erro)
  {
    debugErro("Erro ao interpretar JSON");
    debugErro(erro.c_str());
    return;
  }

  if (!doc["led"].is<JsonObject>())
  {
    debugInfo("Não encontrado o comando para o LED RGB");
  }

  else
  {
    if (!doc["led"]["r"].is<int>() ||
        !doc["led"]["g"].is<int>() ||
        !doc["led"]["b"].is<int>())
    {
      debugErro("JSON inválido. Use led.r, led.g e led.b");
    }
    else
    {
      int vermelho = doc["led"]["r"].as<int>();
      int verde = doc["led"]["g"].as<int>();
      int azul = doc["led"]["b"].as<int>();

      alterarCorDoLedRGB(vermelho, verde, azul);
    }
  }

  if (!doc["lampada"].is<bool>())
  {
    debugErro("Não encontrado o comando para a lampada.");
  }

  else
  {
    bool estadoLampada = doc["lampada"].as<bool>();
    switchLampada(estadoLampada);
    debugInfo("Lampada ligada com sucesso");
    digitalWrite(pinoLampada, estadoLampada ? HIGH : LOW);
  }
}

void switchLampada(bool estado)
{
  const uint8_t pinoLampada = 17;
  digitalWrite(pinoLampada, estado);
}
