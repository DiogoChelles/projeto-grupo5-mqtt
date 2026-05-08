// main.cpp

#include <Arduino.h>
#include "WiFiManager.h"
#include "MqttManager.h"
#include "DebugManager.h"
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>
#include <LED.h>

// ======================================================
//* CONFIGURAÇÕES
// ======================================================

const int PINO_LED_RGB = 48;
const int QUANTIDADE_LEDS = 1;

const char TOPICO_COMANDO[] = "senai134/grupo5/esp32/comando";

// ======================================================
//* OBJETOS
// ======================================================

Adafruit_NeoPixel ledRGB(QUANTIDADE_LEDS, PINO_LED_RGB, NEO_GRB + NEO_KHZ800);

LiquidCrystal_I2C lcd(0x27, 20, 4);

Led lampada(17);

// ======================================================
//* ESTADOS DO JOGO
// ======================================================

const int AGUARDANDO_INICIO = 0;
const int COM_BATATA = 1;
const int ALERTA_PASSAR = 2;
const int EXPLODIU = 3;

int estadoAtual = AGUARDANDO_INICIO;

// ======================================================
//* CONTROLE DE TEMPO
// ======================================================

unsigned long tempoInicioJogo = 0;
unsigned long tempoUltimaRodada = 0;
unsigned long tempoExplosao = 0;

// ======================================================
//* CONSTANTES DE TEMPO
// ======================================================

// após 2 segundos acende LED vermelho
const unsigned long TEMPO_ALERTA = 2000;

// após 3.5 segundos reinicia ciclo
const unsigned long TEMPO_REINICIO = 3500;

// ======================================================
//* CONTROLE PISCAR LED
// ======================================================

unsigned long ultimoPiscar = 0;
bool estadoPisca = false;

// ======================================================
//* PROTÓTIPOS
// ======================================================

void tratarMensagemRecebida(const char *topico, const String &mensagem);

void tratarJsonComando(const String &mensagem);

void iniciarJogo();

void atualizarJogo();

void executarExplosao();

void atualizarPisca();

// LCD
void mostrarQuente();
void mostrarPassar();
void mostrarBoom();
void mostrarAguardando();

// LED RGB
void ledVermelho();
void ledApagado();

// ======================================================
//* SETUP
// ======================================================

void setup()
{
    configurarDebug();

    conectarWiFi();

    configurarMQTT();

    registrarCallbackMensagem(tratarMensagemRecebida);

    conectarMQTT();

    // ================= LED RGB =================

    ledRGB.begin();
    ledRGB.setBrightness(20);
    ledRGB.clear();
    ledRGB.show();

    // ================= LCD =================

    lcd.init();
    lcd.backlight();

    mostrarAguardando();

    randomSeed(esp_random()); // randomSeed(analogRead(0));

    debugInfo("Sistema iniciado");
}

// ======================================================
//* LOOP
// ======================================================

void loop()
{
    garantirWiFiConectado();

    garantirMQTTconectado();

    loopMQTT();

    atualizarJogo();

    atualizarPisca();

    lampada.update();
}

// ======================================================
//* MQTT
// ======================================================

void tratarMensagemRecebida(const char *topico, const String &mensagem)
{
    debugInfo("===================================");
    debugInfo("Mensagem recebida");
    debugInfo("===================================");

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
//* JSON
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
    //* INICIAR JOGO
    // ==================================================

    if (doc["estado"] == "inicio")
    {
        iniciarJogo();
    }
}

// ======================================================
//* INICIAR JOGO
// ======================================================

void iniciarJogo()
{
    debugInfo("Jogo iniciado");

    estadoAtual = COM_BATATA;

    tempoInicioJogo = millis();

    tempoUltimaRodada = millis();

    // sorteia explosao entre 9 e 30 segundos
    tempoExplosao = random(9000, 30000);

    lampada.desligar();

    ledApagado();

    mostrarQuente();

    debugInfo("Tempo explosao: " + String(tempoExplosao));
}

// ======================================================
//* ATUALIZAR JOGO
// ======================================================

void atualizarJogo()
{
    if (estadoAtual == AGUARDANDO_INICIO)
        return;

    unsigned long agora = millis();

    // ==================================================
    //* EXPLODIU
    // ==================================================

    if ((agora - tempoInicioJogo >= tempoExplosao) &&
        estadoAtual != EXPLODIU)
    {
        estadoAtual = EXPLODIU;

        executarExplosao();

        return;
    }

    // ==================================================
    //* ALERTA PARA PASSAR
    // ==================================================

    if (estadoAtual == COM_BATATA)
    {
        if (agora - tempoUltimaRodada >= TEMPO_ALERTA)
        {
            estadoAtual = ALERTA_PASSAR;

            ledVermelho();

            mostrarPassar();
        }
    }

    // ==================================================
    //* REINICIAR CICLO
    // ==================================================

    if (estadoAtual == ALERTA_PASSAR)
    {
        if (agora - tempoUltimaRodada >= TEMPO_REINICIO)
        {
            estadoAtual = COM_BATATA;

            tempoUltimaRodada = millis();

            ledApagado();

            mostrarQuente();
        }
    }
}

// ======================================================
//* EXPLOSÃO
// ======================================================

void executarExplosao()
{
    debugErro("BOOM! Batata explodiu.");

    lampada.piscar(8);

    mostrarBoom();
}

// ======================================================
//* PISCAR LED
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
//* LCD
// ======================================================

void mostrarQuente()
{
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("QUENTE");

    lcd.setCursor(0, 1);
    lcd.print("Passe rapido!");
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

void mostrarAguardando()
{
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Aguardando");

    lcd.setCursor(0, 1);
    lcd.print("inicio...");
}

// ======================================================
//* LED RGB
// ======================================================

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