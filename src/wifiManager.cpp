//*WIFimanager.cpp
#include <arduino.h>
#include "WiFiManager.h"
#include <WiFiClientSecure.h>
#include "secrets.h"
#include <WiFi.h>
#include "DebugManager.h"

void conectarWiFi()
{
  debugInfo("=======================");
  debugInfo("iniciando conexão WiFi");
  debugInfo("=======================");

  // Configura o ESP32 como station, ou seja, ele vai se conectar a um roteador existente
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_SENHA);

  debugInfo("Conectando");

  int tentativas = 0;
  const int maxTentativas = 30;

  // aguarda a conexão por até 30 tentativas
  while (WiFi.status() != WL_CONNECTED && tentativas < maxTentativas)
  {
    debugInfoSemLinha(".");
    delay(500);
    debugInfoSemLinha(".");
    tentativas++;
  }
  debugInfoSemLinha("\n\r");

  if (WiFi.status() == WL_CONNECTED)
  {
    debugInfo("WiFi conectado com sucesso!");
    debugInfoSemLinha("[INFO]Endereço IP: ");
    debugInfoSemLinha(WiFi.localIP().toString());
    debugInfoSemLinha("\n\r");
  }

  else
  {
    debugErro("Falha ao conectar no WiFi");
    debugErro("Verifique o SSID, senha e sinal de rede.");
  }
}

void garantirWiFiConectado()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    debugInfo("WiFi desconectado. Tentando reconectar...");
    conectarWiFi();
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    debugErro("Não foi possível conectar ao WiFi");
  }
}

bool wifiEstaConectado()
{
  return WiFi.status() == WL_CONNECTED;
}