#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "WiFiManager.h"
#include "MqttManager.h"
#include "DebugManager.h"
#include <LiquidCrystal_I2C.h>

// ================= CONFIG =================
#define PINO_LAMPADA 17
#define PINO_LED_RGB 48
#define QTD_LEDS 1

const char MEU_ID[] = "ESP32_A";
const char OUTRO_ID[] = "ESP32_B";

const char TOPICO[] = "senai134/esp32/comando";

// ================= OBJETOS =================
Adafruit_NeoPixel ledRGB(QTD_LEDS, PINO_LED_RGB, NEO_GRB + NEO_KHZ800);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= ESTADOS =================
enum EstadoJogo
{
    SEM_BATATA,
    COM_BATATA,
    ALERTA_PASSAR,
    EXPLODIU
};

EstadoJogo estadoAtual = SEM_BATATA;

// ================= TEMPO =================
unsigned long tempoRecebeu = 0;
unsigned long tempoExplosaoGlobal = 0;

const unsigned long TEMPO_ALERTA = 2000; // 2s
const unsigned long TEMPO_LIMITE = 3500; // 3.5s

// ================= LED PISCAR =================
unsigned long ultimoPiscar = 0;
bool estadoPisca = false;

// ================= PROTÓTIPOS =================
void tratarMensagemRecebida(const char *topico, const String &mensagem);
void tratarJsonJogo(const String &mensagem);
void passarBatata();
void executarExplosao();
void atualizarPisca();

// LCD
void mostrarQuente();
void mostrarPassar();
void mostrarBoom();

// LED
void ledVerde();
void ledVermelho();
void ledApagado();

// ================= SETUP =================
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
    ledRGB.show();

    lcd.init();
    lcd.backlight();

    debugInfo("Sistema iniciado");
}

// ================= LOOP =================
void loop()
{
    garantirWiFiConectado();
    garantirMQTTconectado();
    loopMQTT();

    unsigned long agora = millis();

    // ===== EXPLOSÃO GLOBAL =====
    if (tempoExplosaoGlobal > 0 && agora >= tempoExplosaoGlobal && estadoAtual != EXPLODIU)
    {
        estadoAtual = EXPLODIU;
        executarExplosao();
    }

    // ===== COM BATATA =====
    if (estadoAtual == COM_BATATA)
    {
        if (agora - tempoRecebeu >= TEMPO_ALERTA)
        {
            estadoAtual = ALERTA_PASSAR;
            ledVermelho();
            mostrarPassar();
        }
    }

    // ===== ALERTA =====
    if (estadoAtual == ALERTA_PASSAR)
    {
        if (agora - tempoRecebeu >= TEMPO_LIMITE)
        {
            passarBatata();
        }
    }

    // ===== PISCAR SE EXPLODIU =====
    atualizarPisca();
}

// ================= MQTT =================
void tratarMensagemRecebida(const char *topico, const String &mensagem)
{
    tratarJsonJogo(mensagem);
}

// ================= JSON =================
void tratarJsonJogo(const String &mensagem)
{
    JsonDocument doc;

    if (deserializeJson(doc, mensagem))
    {
        debugErro("Erro JSON");
        return;
    }

    String estado = doc["estado"];

    // ===== INICIAR JOGO =====
    if (estado == "inicio")
    {
        long tempo = doc["tempoExplosao"];

        tempoExplosaoGlobal = millis() + tempo;

        debugInfo("Jogo iniciado");
        debugInfo("Explosao em: " + String(tempo) + " ms");
    }

    // ===== RECEBER BATATA =====
    if (estado == "passar")
    {
        String destino = doc["para"];

        if (destino == MEU_ID)
        {
            estadoAtual = COM_BATATA;
            tempoRecebeu = millis();

            ledApagado();
            mostrarQuente();
            digitalWrite(PINO_LAMPADA, LOW);
        }
    }
}

// ================= AÇÕES =================
void passarBatata()
{
    estadoAtual = SEM_BATATA;

    ledApagado();

    JsonDocument doc;

    doc["estado"] = "passar";
    doc["para"] = OUTRO_ID;

    char buffer[128];
    serializeJson(doc, buffer);

    publicarMensagem(TOPICO, buffer);

    debugInfo("Batata enviada");
}

void executarExplosao()
{
    digitalWrite(PINO_LAMPADA, HIGH);

    mostrarBoom();

    debugErro("BOOM! Você perdeu.");
}

// ================= LED =================
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

// ================= PISCAR =================
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
            ledVermelho();
        else
            ledApagado();
    }
}

// ================= LCD =================
void mostrarQuente()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("quente");
    lcd.setCursor(0, 1);
    lcd.print("quente");
}

void mostrarPassar()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PASSAR!");
}

void mostrarBoom()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("BOOM!");
}