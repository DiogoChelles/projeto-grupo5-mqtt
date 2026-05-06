// MqttManager.h

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>

void configurarMQTT();
void conectarMQTT();
void garantirMQTTConectado();
void loopMQTT();

void publicarMensagem(const char *topico, const char *mensagem);
void publicarMensagemNoTopico(int indiceTopico, const char *mensagem);

bool mqttEstaConectado();

const char *obterTopicoPublicacao();
const char *obterTopicoRecebimento();
int obterTotalTopicoRecebimento();

typedef void (*CallbackMensagemMQTT)(const char *topico, const String &mensagem);

void registrarCallbackMensagem(CallbackMensagemMQTT callback);
void alterarCorDoLedRGB(int vermelho, int verde, int azul);

void ligarLampada();

#endif
