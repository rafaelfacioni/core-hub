/**
 *
 * Copyright (c) 2023 HT Micron Semicondutores S.A.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "HT_MQTT_Api.h"
#include "stdio.h"
#include "string.h"

/* Define bool if not already defined */
#ifndef bool
#define bool uint8_t
#define true 1
#define false 0
#endif

/* Global variables */
static void (*g_mqtt_message_callback)(MessageData *msg) = NULL;

static MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

/* TLS context - simplified for non-TLS usage */
#if MQTT_TLS_ENABLE == 1
static struct {
    uint32_t caCertLen;
    int32_t port;
    char *host;
    uint32_t timeout_ms;
    bool isMqtt;
    uint32_t timeout_r;
    uint32_t timeout_s;
} mqtt_client_ctx;
#endif

uint8_t HT_MQTT_Connect(MQTTClient *mqtt_client, Network *mqtt_network, char *addr, int32_t port, uint32_t send_timeout, uint32_t rcv_timeout, char *clientID, 
                                        char *username, char *password, uint8_t mqtt_version, uint32_t keep_alive_interval, uint8_t *sendbuf, 
                                        uint32_t sendbuf_size, uint8_t *readbuf, uint32_t readbuf_size) {

    printf("HT_MQTT_Connect: Iniciando conexão...\n");

#if  MQTT_TLS_ENABLE == 1
    mqtt_client_ctx.caCertLen = 0;
    mqtt_client_ctx.port = port;
    mqtt_client_ctx.host = addr;
    mqtt_client_ctx.timeout_ms = MQTT_GENERAL_TIMEOUT;
    mqtt_client_ctx.isMqtt = true;
    mqtt_client_ctx.timeout_r = MQTT_GENERAL_TIMEOUT;
    mqtt_client_ctx.timeout_s = MQTT_GENERAL_TIMEOUT;
#endif

    printf("HT_MQTT_Connect: Configurando dados de conexão...\n");
    connectData.MQTTVersion = mqtt_version;
    connectData.clientID.cstring = clientID;
    connectData.username.cstring = username;
    connectData.password.cstring = password;
    connectData.keepAliveInterval = keep_alive_interval;
    connectData.will.qos = QOS0;
    connectData.cleansession = false;

#if MQTT_TLS_ENABLE == 1
    /* TLS connection - simplified implementation */
    printf("TLS not fully implemented, using non-TLS connection\n");
    mqtt_client_ctx.caCertLen = 0;
    mqtt_client_ctx.port = port;
    mqtt_client_ctx.host = addr;
    mqtt_client_ctx.timeout_ms = MQTT_GENERAL_TIMEOUT;
    mqtt_client_ctx.isMqtt = true;
    mqtt_client_ctx.timeout_r = MQTT_GENERAL_TIMEOUT;
    mqtt_client_ctx.timeout_s = MQTT_GENERAL_TIMEOUT;
    
    /* For now, fall through to non-TLS implementation */
#endif

    printf("HT_MQTT_Connect: Inicializando rede...\n");
    NetworkInit(mqtt_network);
    printf("HT_MQTT_Connect: Inicializando cliente MQTT...\n");
    MQTTClientInit(mqtt_client, mqtt_network, MQTT_GENERAL_TIMEOUT, (unsigned char *)sendbuf, sendbuf_size, (unsigned char *)readbuf, readbuf_size);
    
    printf("HT_MQTT_Connect: Configurando timeout de conexão...\n");
    /* Set connection timeout */
    if(NetworkSetConnTimeout(mqtt_network, send_timeout, rcv_timeout) != 0) {
        printf("HT_MQTT_Connect: Failed to set connection timeout\n");
        return 1;
    }
    
    printf("HT_MQTT_Connect: Conectando à rede...\n");
    /* Connect to network */
    if(NetworkConnect(mqtt_network, addr, port) != 0) {
        printf("HT_MQTT_Connect: Network connection failed\n");
        return 1;
    }
    
    printf("HT_MQTT_Connect: Conectando ao broker MQTT...\n");
    /* Connect to MQTT broker */
    if(MQTTConnect(mqtt_client, &connectData) != 0) {
        printf("HT_MQTT_Connect: MQTT connection failed\n");
        return 1;
    }
    
    printf("HT_MQTT_Connect: Successfully connected to MQTT broker %s:%ld\n", addr, port);

    return 0;
}

int HT_MQTT_Publish(MQTTClient *mqtt_client, char *topic, uint8_t *payload, uint32_t len, enum QoS qos, uint8_t retained, uint16_t id, uint8_t dup) {
    MQTTMessage message;

    message.qos = qos;
    message.retained = retained;
    message.id = id;
    message.dup = dup;
    message.payload = payload;
    message.payloadlen = len;

    return MQTTPublish(mqtt_client, topic, &message);
}

void HT_MQTT_SubscribeCallback(MessageData *msg) {
    /* Call the registered callback function if available */
    if (g_mqtt_message_callback != NULL) {
        g_mqtt_message_callback(msg);
    } else {
        /* Default callback - just print the message */
        char topic[128] = {0};
        char payload[256] = {0};
        
        /* Extract topic */
        if (msg->topicName->lenstring.len < sizeof(topic)) {
            memcpy(topic, msg->topicName->lenstring.data, msg->topicName->lenstring.len);
            topic[msg->topicName->lenstring.len] = '\0';
        }
        
        /* Extract payload */
        if (msg->message->payloadlen < sizeof(payload)) {
            memcpy(payload, msg->message->payload, msg->message->payloadlen);
            payload[msg->message->payloadlen] = '\0';
        }
        
        printf("HT_MQTT_SubscribeCallback: Received message - Topic: %s, Payload: %s\n", topic, payload);
    }
}

void HT_MQTT_Subscribe(MQTTClient *mqtt_client, char *topic, enum QoS qos) {
    int result = MQTTSubscribe(mqtt_client, topic, qos, HT_MQTT_SubscribeCallback);
    if (result != 0) {
        printf("HT_MQTT_Subscribe: Failed to subscribe to topic %s, result = %d\n", topic, result);
    } else {
        printf("HT_MQTT_Subscribe: Successfully subscribed to topic: %s\n", topic);
    }
}

/*!******************************************************************
 * \fn void HT_MQTT_SetMessageCallback(void (*callback)(MessageData *msg))
 * \brief Set the callback function for MQTT messages.
 *
 * \param[in] void (*callback)(MessageData *msg)  Callback function pointer.
 * 
 * \retval none
 *******************************************************************/
void HT_MQTT_SetMessageCallback(void (*callback)(MessageData *msg))
{
    g_mqtt_message_callback = callback;
}

/*!******************************************************************
 * \fn int HT_MQTT_Yield(MQTTClient *mqtt_client, int timeout_ms)
 * \brief Yield to allow MQTT client to process incoming messages.
 *
 * \param[in] MQTTClient *mqtt_client           MQTT client handle.
 * \param[in] int timeout_ms                    Timeout in milliseconds.
 * 
 * \retval int                                  0 = Success, 1 = Error
 *******************************************************************/
int HT_MQTT_Yield(MQTTClient *mqtt_client, int timeout_ms)
{
    return MQTTYield(mqtt_client, timeout_ms);
}

/*!******************************************************************
 * \fn int HT_MQTT_Disconnect(MQTTClient *mqtt_client)
 * \brief Disconnect from MQTT broker.
 *
 * \param[in] MQTTClient *mqtt_client           MQTT client handle.
 * 
 * \retval int                                  0 = Success, 1 = Error
 *******************************************************************/
int HT_MQTT_Disconnect(MQTTClient *mqtt_client)
{
    int result = MQTTDisconnect(mqtt_client);
    if (result != 0) {
        printf("HT_MQTT_Disconnect: Failed to disconnect, result = %d\n", result);
        return 1;
    }
    
    printf("HT_MQTT_Disconnect: Successfully disconnected from MQTT broker\n");
    return 0;
}

int HT_MQTT_Unsubscribe(MQTTClient *mqtt_client, char *topic) {
    int result = MQTTUnsubscribe(mqtt_client, topic);
    if (result != 0) {
        printf("HT_MQTT_Unsubscribe: Failed to unsubscribe from topic %s, result = %d\n", topic, result);
        return 1;
    } else {
        printf("HT_MQTT_Unsubscribe: Successfully unsubscribed from topic: %s\n", topic);
        return 0;
    }
}

/************************ HT Micron Semicondutores S.A *****END OF FILE****/