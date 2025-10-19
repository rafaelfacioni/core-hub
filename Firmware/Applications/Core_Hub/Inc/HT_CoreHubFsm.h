/*
  _    _  _____   _____  ____   _    _ _    _ ____  _    _ 
 | |  | |/ ____| |  __ \|  _ \ | |  | | |  | |  _ \| |  | |
 | |__| | |      | |__) | |_) || |  | | |  | | |_) | |  | |
 |  __  | |      |  _  /|  _ < | |  | | |  | |  _ <| |  | |
 | |  | | |____  | | \ \| |_) || |__| | |__| | |_) | |__| |
 |_|  |_|\_____| |_|  \_\____/  \____/ \____/|____/ \____/ 
==================== CoreHub ==============================

Copyright (c) 2024
Licensed under the Apache License, Version 2.0 (the "License");

*/

#ifndef __HT_COREHUB_FSM_H__
#define __HT_COREHUB_FSM_H__

#include "stdint.h"
#include "main.h"
#include "HT_MQTT_Api.h"
#include "MQTTClient.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Configurações MQTT */
#define HT_COREHUB_MQTT_BUFFER_SIZE 1024

/* Definições MQTT baseadas no exemplo */
#define HT_MQTT_KEEP_ALIVE_INTERVAL 240                   /**</ Keep alive interval in ms. */
#define HT_MQTT_VERSION 4                                 /**</ MQTT protocol version. */

#if MQTT_TLS_ENABLE == 1
#define HT_MQTT_PORT   8883                               /**</ MQTT TCP TLS port. */
#else
#define HT_MQTT_PORT   1883                               /**</ MQTT TCP port. */
#endif

#define HT_MQTT_SEND_TIMEOUT 60000                        /**</ MQTT TX timeout. */
#define HT_MQTT_RECEIVE_TIMEOUT   60000                   /**</ MQTT RX timeout. */
#define HT_SUBSCRIBE_BUFF_SIZE  40                         /**</ Maximum buffer size to received from MQTT subscribe. */

/* Configurações do CoreHub */
#define HT_COREHUB_TEMP_LIMIT_UPPER    28.0f              /**</ Limite superior de temperatura (°C) */
#define HT_COREHUB_TEMP_LIMIT_LOWER    24.0f              /**</ Limite inferior de temperatura (°C) */
#define HT_COREHUB_ALARM_TIMEOUT_MS    60000              /**</ Timeout do alarme (60 segundos) */
#define HT_COREHUB_STATUS_INTERVAL_MS  10000              /**</ Intervalo para status (10 segundos) */
#define HT_COREHUB_AC_TEMP_SETPOINT    22                 /**</ Temperatura de setpoint do AC (°C) */

/* Configurações de Conexão Inteligente */
#define HT_COREHUB_SENSECLIMA_INTERVAL_MS  10000          /**</ Intervalo para resgate de dados SenseClima (10s) */
#define HT_COREHUB_AIRCONTROL_TIMEOUT_MS  10000           /**</ Timeout para desconectar AirControl (10s) */
#define HT_COREHUB_SMARTDOOR_ALWAYS_ON    1               /**</ SmartDoor sempre conectado */

/* Configurações FreeRTOS */
#define HT_COREHUB_MQTT_TASK_PRIORITY      (configMAX_PRIORITIES - 1)  /**</ Prioridade máxima para MQTT */
#define HT_COREHUB_SMARTDOOR_TASK_PRIORITY (configMAX_PRIORITIES - 2)  /**</ Alta prioridade para SmartDoor */
#define HT_COREHUB_SENSECLIMA_TASK_PRIORITY (configMAX_PRIORITIES - 2) /**</ Alta prioridade para SenseClima (controle AC) */
#define HT_COREHUB_AIRCONTROL_TASK_PRIORITY (configMAX_PRIORITIES - 3) /**</ Baixa prioridade para AirControl */

#define HT_COREHUB_MQTT_TASK_STACK_SIZE    2048           /**</ Stack size para task MQTT */
#define HT_COREHUB_SMARTDOOR_TASK_STACK_SIZE 2048         /**</ Stack size para task SmartDoor */
#define HT_COREHUB_SENSECLIMA_TASK_STACK_SIZE 2048        /**</ Stack size para task SenseClima */
#define HT_COREHUB_AIRCONTROL_TASK_STACK_SIZE 2048       /**</ Stack size para task AirControl */

#define HT_COREHUB_QUEUE_SIZE              10              /**</ Tamanho da fila de mensagens */

#define NUM_AMBIENTES 3

/* Estados da FSM do CoreHub */
typedef enum {
    HT_COREHUB_INIT_STATE = 0,           /**</ Início */
    HT_COREHUB_CONNECT_MQTT_STATE,       /**</ Conectado ao MQTT? */
    HT_COREHUB_IDLE_STATE,               /**</ Ocioso: Aguardando Eventos */
    HT_COREHUB_RECONNECT_STATE,          /**</ Tenta Reconectar */
    HT_COREHUB_ANALYZE_DOOR_STATE,       /**</ Análise: Luz / Porta */
    HT_COREHUB_ANALYZE_TEMP_STATE,       /**</ Análise: Temperatura */
    HT_COREHUB_AC_ON_STATE,              /**</ Ligar Ar Condicionado */
    HT_COREHUB_AC_OFF_STATE,             /**</ Desligar Ar Condicionado */
    HT_COREHUB_ALARM_LOGIC_STATE,        /**</ Lógica do Alarme */
    HT_COREHUB_WAIT_TIMER_STATE,         /**</ Aguardando Timer (60s) */
    HT_COREHUB_BUZZER_ON_STATE,          /**</ Ligar Buzzer */
    HT_COREHUB_BUZZER_OFF_STATE          /**</ Desligar Buzzer */
} HT_CoreHub_FSM_States;

/* Estrutura de dados do CoreHub */
typedef struct {
    float temperature;                   /**</ Temperatura atual (°C) */
    float humidity;                      /**</ Umidade atual (%) */
    uint8_t door_state;                  /**</ Estado da porta (0=CLOSED, 1=OPEN) */
    uint8_t light_state;                 /**</ Estado da luz (0=OFF, 1=ON) */
    uint8_t ac_state;                    /**</ Estado do AC (0=OFF, 1=ON) */
    uint8_t buzzer_state;                /**</ Estado do buzzer (0=OFF, 1=ON) */
    uint32_t door_open_time;             /**</ Tempo quando porta abriu (s) */
    uint32_t alarm_start_time;           /**</ Tempo quando alarme iniciou (s) */
    uint32_t buzzer_start_time;          /**</ Tempo quando buzzer foi ligado (s) */
    uint32_t system_uptime;              /**</ Tempo de funcionamento (s) */
    uint8_t mqtt_connected;              /**</ Status da conexão MQTT (0=OFF, 1=ON) */
    uint8_t alarm_active;                /**</ Status do alarme (0=INACTIVE, 1=ACTIVE) */
} HT_CoreHub_Data_t;

/* Inicialização de ambientes */
void HT_CoreHub_InitAmbiente(int ambiente_idx, const char* nome);
void HT_CoreHub_StartAmbienteTask(int ambiente_idx);
// Cada ambiente terá sua própria instância de dados e FSM
void HT_CoreHub_MqttTask(void *pvParameters);

#endif /* __HT_COREHUB_FSM_H__ */

/************************ CoreHub *****END OF FILE****/ 