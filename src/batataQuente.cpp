#include <Arduino.h>
#include "WiFiManager.h"
#include "MqttManager.h"
#include "DebugManager.h"
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ======================================================
// CONFIGURAÇÕES
// ======================================================

#define PINO_LAMPADA 17
#define PINO_LED_RGB 48
#define QUANTIDADE_LEDS 1

const char TOPICO_COMANDO[] = "senai134/esp32/comando";

const char MEU_ID[] = "ESP32_A";
const char OUTRO_ID[] = "ESP32_B";

// ======================================================
// OBJETOS
// ======================================================

Adafruit_NeoPixel ledRGB(QUANTIDADE_LEDS, PINO_LED_RGB, NEO_GRB + NEO_KHZ800);

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ======================================================
// ESTADOS DO JOGO
// ======================================================

enum EstadoJogo
{
    SEM_BATATA,
    COM_BATATA,
    ALERTA_PASSAR,
    EXPLODIU
};

EstadoJogo estadoAtual = SEM_BATATA;

// ======================================================
// CONTROLE DE TEMPO
// ======================================================

unsigned long tempoRecebeuBatata = 0;
unsigned long tempoExplosaoGlobal = 0;

const unsigned long TEMPO_ALERTA = 2000; // após 2 segundos
const unsigned long TEMPO_PASSAR = 3500; // total 3.5 segundos

// ======================================================
// CONTROLE PISCAR LED
// ======================================================

unsigned long ultimoPiscar = 0;
bool estadoPisca = false;

// ======================================================
// PROTÓTIPOS
// ======================================================

void tratarMensagemRecebida(const char *topico, const String &mensagem);
void tratarJsonComando(const String &mensagem);

void mostrarQuente();
void mostrarPassar();
void mostrarBoom();

void ledVerde();
void ledVermelho();
void ledApagado();

void passarBatata();
void executarExplosao();
void atualizarPisca();

void switchLampada(bool estado);

// ======================================================
// SETUP
// ======================================================

void setup()
{
    pinMode(PINO_LAMPADA, OUTPUT);

    configurarDebug();

    conectarWiFi();

    configurarMQTT();

    registrarCallbackMensagem(tratarMensagemRecebida);

    conectarMQTT();

    ledRGB.begin();
    ledRGB.setBrightness(20);
    ledRGB.clear();
    ledRGB.show();

    lcd.init();
    lcd.backlight();

    debugInfo("Sistema iniciado");
}

// ======================================================
// LOOP
// ======================================================

void loop()
{
    garantirWiFiConectado();

    garantirMQTTconectado();

    loopMQTT();

    unsigned long agora = millis();

    // ==================================================
    // EXPLOSÃO
    // ==================================================

    if (tempoExplosaoGlobal > 0 &&
        agora >= tempoExplosaoGlobal &&
        estadoAtual != EXPLODIU)
    {
        estadoAtual = EXPLODIU;

        executarExplosao();
    }

    // ==================================================
    // ALERTA PARA PASSAR
    // ==================================================

    if (estadoAtual == COM_BATATA)
    {
        if (agora - tempoRecebeuBatata >= TEMPO_ALERTA)
        {
            estadoAtual = ALERTA_PASSAR;

            ledVermelho();

            mostrarPassar();
        }
    }

    // ==================================================
    // PASSAR BATATA
    // ==================================================

    if (estadoAtual == ALERTA_PASSAR)
    {
        if (agora - tempoRecebeuBatata >= TEMPO_PASSAR)
        {
            passarBatata();
        }
    }

    // ==================================================
    // PISCAR LED APÓS EXPLOSÃO
    // ==================================================

    atualizarPisca();
}

// ======================================================
// MQTT
// ======================================================

void tratarMensagemRecebida(const char *topico, const String &mensagem)
{
    debugInfo("=======================================");
    debugInfo("Mensagem recebida");
    debugInfo("=======================================");

    debugInfo("Topico: " + String(topico));
    debugInfo("Mensagem: " + mensagem);

    if (strcmp(topico, TOPICO_COMANDO) == 0)
    {
        tratarJsonComando(mensagem);
        return;
    }

    debugErro("Topico nao tratado");
}

// ======================================================
// JSON
// ======================================================

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

    // ==================================================
    // INICIAR JOGO
    // ==================================================

    if (doc["estado"] == "inicio")
    {
        long tempoExplosao = doc["tempoExplosao"].as<long>();

        tempoExplosaoGlobal = millis() + tempoExplosao;

        debugInfo("Jogo iniciado");
        debugInfo("Tempo explosao: " + String(tempoExplosao));
    }

    // ==================================================
    // RECEBER BATATA
    // ==================================================

    if (doc["estado"] == "passar")
    {
        String destino = doc["para"].as<String>();

        if (destino == MEU_ID)
        {
            estadoAtual = COM_BATATA;

            tempoRecebeuBatata = millis();

            ledApagado();

            mostrarQuente();

            switchLampada(false);

            debugInfo("Batata recebida");
        }
    }
}

// ======================================================
// PASSAR BATATA
// ======================================================

void passarBatata()
{
    estadoAtual = SEM_BATATA;

    ledApagado();

    JsonDocument doc;

    doc["estado"] = "passar";
    doc["para"] = OUTRO_ID;

    char buffer[128];

    serializeJson(doc, buffer);

    publicarMensagem(TOPICO_COMANDO, buffer);

    debugInfo("Batata enviada");
}

// ======================================================
// EXPLOSÃO
// ======================================================

void executarExplosao()
{
    switchLampada(true);

    mostrarBoom();

    debugErro("BOOM! Batata explodiu.");
}

// ======================================================
// LCD
// ======================================================

void mostrarQuente()
{
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("QUENTE");

    lcd.setCursor(0, 1);
    lcd.print("Segura!");
}

void mostrarPassar()
{
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("PASSA!");
}

void mostrarBoom()
{
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("BOOM!");
}

// ======================================================
// LED RGB
// ======================================================

void ledVerde()
{
    ledRGB.setPixelColor(0, ledRGB.Color(0, 255, 0));
    ledRGB.show();
}

void ledVermelho()
{
    ledRGB.setPixelColor(0, ledRGB.Color(255, 0, 0));
    ledRGB.show();
}

void ledApagado()
{
    ledRGB.setPixelColor(0, ledRGB.Color(0, 0, 0));
    ledRGB.show();
}

// ======================================================
// PISCAR LED
// ======================================================

void atualizarPisca()
{
    if (estadoAtual != EXPLODIU)
        return;

    unsigned long agora = millis();

    if (agora - ultimoPiscar >= 200)
    {
        ultimoPiscar = agora;

        estadoPisca = !estadoPisca;

        if (estadoPisca)
        {
            ledVermelho();
        }

        else
        {
            ledApagado();
        }
    }
}

// ======================================================
// LÂMPADA
// ======================================================

void switchLampada(bool estado)
{
    digitalWrite(PINO_LAMPADA, estado ? HIGH : LOW);
}