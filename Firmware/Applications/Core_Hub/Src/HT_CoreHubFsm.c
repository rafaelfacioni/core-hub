
#include "HT_CoreHubFsm.h"
#include "HT_MQTT_Api.h"
#include "stdio.h"
#include "string.h"
#include "FreeRTOS.h"
#include "task.h"
#include "osasys.h" 
#include "cJSON.h"

extern const char* ambientes[NUM_AMBIENTES];

/* Estados da FSM - Exatamente como no diagrama */
typedef enum {
    COREHUB_INIT_STATE = 0,           // Início
    COREHUB_CONNECT_MQTT_STATE,       // Conectado ao MQTT?
    COREHUB_IDLE_STATE,               // Ocioso: Aguardando Eventos
    COREHUB_RECONNECT_STATE,          // Tenta Reconectar
    COREHUB_ANALYZE_DOOR_STATE,       // Análise: Luz / Porta
    COREHUB_ANALYZE_TEMP_STATE,       // Análise: Temperatura
    COREHUB_AC_ON_STATE,              // Ligar Ar Condicionado
    COREHUB_AC_OFF_STATE,             // Desligar Ar Condicionado
    COREHUB_ALARM_LOGIC_STATE,        // Lógica do Alarme
    COREHUB_WAIT_TIMER_STATE,         // Aguardando Timer (60s)
    COREHUB_BUZZER_ON_STATE,          // Ligar Buzzer
    COREHUB_BUZZER_OFF_STATE          // Desligar Buzzer
} CoreHub_FSM_States;

/* Estrutura de dados */
typedef struct {
    float temperature;
    float humidity;
    uint8_t door_state;      // 0=CLOSED, 1=OPEN
    uint8_t light_state;     // 0=OFF, 1=ON
    uint8_t ac_state;        // 0=OFF, 1=ON
    uint8_t buzzer_state;    // 0=OFF, 1=ON
    uint32_t alarm_start_time;
    uint32_t buzzer_start_time;  // Tempo quando buzzer foi ligado
    uint32_t system_uptime;
    uint8_t mqtt_connected;
    uint8_t alarm_active;    // 0=INACTIVE, 1=ACTIVE
} CoreHub_Data_t;

/* Variáveis globais */
// Instâncias para múltiplos ambientes
CoreHub_Data_t corehub_data[NUM_AMBIENTES];
CoreHub_FSM_States current_state[NUM_AMBIENTES];
// Buffers de tópicos por ambiente
static char topic_smartdoor_door[NUM_AMBIENTES][64];
static char topic_smartdoor_light[NUM_AMBIENTES][64];
static char topic_smartdoor_buzzer[NUM_AMBIENTES][64];
static char topic_senseclima_temp[NUM_AMBIENTES][64];
static char topic_senseclima_humidity[NUM_AMBIENTES][64];
static char topic_aircontrol_power[NUM_AMBIENTES][64];
static char topic_aircontrol_temp[NUM_AMBIENTES][64];

// Cliente MQTT único para todos os ambientes
static MQTTClient mqttClient_global;
static Network mqttNetwork_global;
static uint8_t mqttSendbuf_global[HT_COREHUB_MQTT_BUFFER_SIZE];
static uint8_t mqttReadbuf_global[HT_COREHUB_MQTT_BUFFER_SIZE];
static uint8_t mqtt_connection_active = 0;

// Buffers de dados por ambiente (otimização de performance)
static volatile int new_temp_data[NUM_AMBIENTES] = {0};
static volatile int new_hum_data[NUM_AMBIENTES] = {0};
static float buffered_temp[NUM_AMBIENTES] = {0.0f};
static float buffered_hum[NUM_AMBIENTES] = {0.0f};

// Controle de performance e watchdog
static uint32_t last_fsm_execution[NUM_AMBIENTES] = {0};
static uint32_t fsm_execution_count[NUM_AMBIENTES] = {0};
static uint8_t fsm_stuck_detected[NUM_AMBIENTES] = {0};

// Watchdog global do sistema
static uint32_t last_watchdog_check = 0;

/* Declaração antecipada da função de tempo */
static uint32_t CoreHub_GetTimeSecs(void);

/* Função de watchdog global */
static void CoreHub_WatchdogCheck(void) {
    uint32_t current_time = CoreHub_GetTimeSecs();
    
    // Verifica a cada 30 segundos
    if (current_time - last_watchdog_check < 30) {
        return;
    }
    last_watchdog_check = current_time;
    
    // Verifica se alguma FSM está travada
    for (int i = 0; i < NUM_AMBIENTES; i++) {
        if (fsm_execution_count[i] > 500) { // Se executou mais de 500 vezes sem reset
            fsm_stuck_detected[i] = 1;
            printf("[CoreHub] WATCHDOG: FSM %s detectada como travada\n", ambientes[i]);
        }
    }
    
    // Log de saúde do sistema a cada 5 minutos
    static uint32_t health_log_counter = 0;
    health_log_counter++;
    if (health_log_counter >= 10) { // 30s * 10 = 5 minutos
        printf("[CoreHub] SAÚDE: Sistema operando normalmente (%lu s uptime)\n", current_time);
        health_log_counter = 0;
    }
}

/* Configurações MQTT */
static const char clientID[] = {"corehub01"};
static const char username[] = {""};
static const char password[] = {""};
static const char broker_addr[] = {"131.255.82.115"};
static const int32_t broker_port = HT_MQTT_PORT;

/* Função para montar tópicos MQTT dinamicamente */
static void CoreHub_MontaTopicos(int ambiente_idx, const char* ambiente) {
    snprintf(topic_smartdoor_door[ambiente_idx], sizeof(topic_smartdoor_door[ambiente_idx]), "hana/%s/smartdoor/door", ambiente);
    snprintf(topic_smartdoor_light[ambiente_idx], sizeof(topic_smartdoor_light[ambiente_idx]), "hana/%s/smartdoor/light", ambiente);
    snprintf(topic_smartdoor_buzzer[ambiente_idx], sizeof(topic_smartdoor_buzzer[ambiente_idx]), "hana/%s/smartdoor/buzzer", ambiente);
    snprintf(topic_senseclima_temp[ambiente_idx], sizeof(topic_senseclima_temp[ambiente_idx]), "hana/%s/senseclima/01/temperature", ambiente);
    snprintf(topic_senseclima_humidity[ambiente_idx], sizeof(topic_senseclima_humidity[ambiente_idx]), "hana/%s/senseclima/01/humidity", ambiente);
    snprintf(topic_aircontrol_power[ambiente_idx], sizeof(topic_aircontrol_power[ambiente_idx]), "hana/%s/aircontrol/01/power", ambiente);
    snprintf(topic_aircontrol_temp[ambiente_idx], sizeof(topic_aircontrol_temp[ambiente_idx]), "hana/%s/aircontrol/01/temperature", ambiente);
}

void HT_CoreHub_InitAmbiente(int ambiente_idx, const char* nome) {
    memset(&corehub_data[ambiente_idx], 0, sizeof(CoreHub_Data_t));
    current_state[ambiente_idx] = COREHUB_INIT_STATE;
    CoreHub_MontaTopicos(ambiente_idx, nome);
}

void HT_CoreHub_StartAmbienteTask(int ambiente_idx) {
    // Agora apenas inicializa o ambiente, a task global será criada uma única vez
}

/* Função para obter tempo em segundos */
static uint32_t CoreHub_GetTimeSecs(void) {
    static uint32_t start_time = 0;
    uint32_t current_time = OsaSystemTimeReadSecs();
    
    if (start_time == 0) {
        start_time = current_time;
    }
    
    return current_time - start_time;
}

/* Conversão manual de string para float para ambientes embarcados */
static float simple_str_to_float(const char* str) {
    float result = 0.0f, factor = 1.0f;
    int sign = 1, point_seen = 0;
    if (!str) return 0.0f;
    if (*str == '-') { sign = -1; str++; }
    for (; *str; str++) {
        if (*str == '.') {
            point_seen = 1;
            continue;
        }
        if (*str < '0' || *str > '9') break;
        if (point_seen) {
            factor /= 10.0f;
            result += (*str - '0') * factor;
        } else {
            result = result * 10.0f + (*str - '0');
        }
    }
    return sign * result;
}

static float string_to_float(const char* str) {
    return simple_str_to_float(str);
}

// Função de publish com retry otimizada
static int CoreHub_MQTTPublishWithRetry(MQTTClient *mqtt_client, char *topic, uint8_t *payload, uint32_t len, enum QoS qos, uint8_t retained, uint16_t id, uint8_t dup, int max_retries) {
    // Proteção contra parâmetros inválidos
    if (!mqtt_client || !topic || !payload || len == 0) {
        return -1;
    }
    
    int rc = -1;
    for (int attempt = 1; attempt <= max_retries; ++attempt) {
        rc = HT_MQTT_Publish(mqtt_client, topic, payload, len, qos, retained, id, dup);
        if (rc == 0) {
            return 0;
        }
        
        // Delay progressivo para evitar sobrecarga
        uint32_t delay_ms = (attempt * 50); // 50ms, 100ms, 150ms...
        if (delay_ms > 500) delay_ms = 500; // Máximo 500ms
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
    
    printf("[CoreHub] ERRO: Falha ao publicar %s após %d tentativas\n", topic, max_retries);
    return rc;
}

static int CoreHub_IdentificaAmbientePorTopico(const char* topic) {
    for (int i = 0; i < NUM_AMBIENTES; ++i) {
        char prefix[32];
        snprintf(prefix, sizeof(prefix), "hana/%s/", ambientes[i]);
        if (strncmp(topic, prefix, strlen(prefix)) == 0) {
            return i;
        }
    }
    return -1; // Não encontrado
}

/* Callback para mensagens MQTT */
static void HT_CoreHub_MessageCallback(MessageData *msg) {
    // Proteção crítica contra dados inválidos
    if (msg == NULL || msg->message == NULL || msg->topicName == NULL) {
        return;
    }
    if (msg->message->payload == NULL || msg->message->payloadlen == 0) {
        return;
    }

    // Proteção contra tamanhos excessivos
    size_t topic_len = msg->topicName->lenstring.len;
    size_t payload_len = msg->message->payloadlen;
    
    if (topic_len > 127 || payload_len > 63) {
        printf("[CoreHub] ERRO: Mensagem MQTT muito grande (topic:%zu, payload:%zu)\n", topic_len, payload_len);
        return;
    }

    char topic[128] = {0};
    char payload[64] = {0};

    // Cópia segura dos dados
    memcpy(topic, msg->topicName->lenstring.data, topic_len);
    topic[topic_len] = '\0';
    memcpy(payload, msg->message->payload, payload_len);
    payload[payload_len] = '\0';

    int ambiente_idx = CoreHub_IdentificaAmbientePorTopico(topic);
    if (ambiente_idx < 0 || ambiente_idx >= NUM_AMBIENTES) {
        return;
    }

    // Processa mensagens conforme diagrama
    CoreHub_Data_t* data = &corehub_data[ambiente_idx];
    CoreHub_FSM_States* state = &current_state[ambiente_idx];
    
    // Proteção contra dados corrompidos
    if (data == NULL || state == NULL) {
        printf("[CoreHub] ERRO: Dados do ambiente %d corrompidos\n", ambiente_idx);
        return;
    }

    if (strstr(topic, "smartdoor/door")) {
        if (strcmp(payload, "OPEN") == 0) {
            data->door_state = 1;
        } else if (strcmp(payload, "CLOSED") == 0) {
            data->door_state = 0;
            if (data->alarm_active || data->buzzer_state) {
                *state = COREHUB_BUZZER_OFF_STATE;
                return;
            }
        }
        *state = COREHUB_ANALYZE_DOOR_STATE;
    }
    else if (strstr(topic, "smartdoor/light")) {
        data->light_state = (strcmp(payload, "ON") == 0) ? 1 : 0;
        if (data->light_state == 0 && (data->alarm_active || data->buzzer_state)) {
            *state = COREHUB_BUZZER_OFF_STATE;
            return;
        }
        *state = COREHUB_ANALYZE_DOOR_STATE;
    }
    else if (strstr(topic, "senseclima/01/temperature")) {
        float temperature = string_to_float(payload);
        buffered_temp[ambiente_idx] = temperature;
        new_temp_data[ambiente_idx] = 1;
    }
    else if (strstr(topic, "senseclima/01/humidity")) {
        float humidity = string_to_float(payload);
        buffered_hum[ambiente_idx] = humidity;
        new_hum_data[ambiente_idx] = 1;
    }
    else if (strstr(topic, "aircontrol/01/power")) {
        data->ac_state = (strcmp(payload, "ON") == 0) ? 1 : 0;
    }
}



/* Máquina de Estados - Exatamente como no diagrama */
static void HT_CoreHub_StateMachine(int ambiente_idx) {
    // Proteção contra índice inválido
    if (ambiente_idx < 0 || ambiente_idx >= NUM_AMBIENTES) {
        return;
    }
    
    CoreHub_Data_t* data = &corehub_data[ambiente_idx];
    CoreHub_FSM_States* state = &current_state[ambiente_idx];
    
    // Controle de performance - evita execução excessiva
    uint32_t current_time = CoreHub_GetTimeSecs();
    if (current_time - last_fsm_execution[ambiente_idx] < 1) { // Máximo 1 execução por segundo
        return;
    }
    last_fsm_execution[ambiente_idx] = current_time;
    fsm_execution_count[ambiente_idx]++;
    
    // Watchdog - detecta FSM travada
    if (fsm_execution_count[ambiente_idx] > 1000) { // Reset a cada 1000 execuções
        fsm_execution_count[ambiente_idx] = 0;
        if (fsm_stuck_detected[ambiente_idx]) {
            printf("[CoreHub][%s] WATCHDOG: FSM resetada por travamento\n", ambientes[ambiente_idx]);
            *state = COREHUB_IDLE_STATE;
            fsm_stuck_detected[ambiente_idx] = 0;
        }
    }
    switch (*state) {
            case COREHUB_INIT_STATE:
            // Transição: Início --> Conectado ao MQTT?
            *state = COREHUB_CONNECT_MQTT_STATE;
                break;

            case COREHUB_CONNECT_MQTT_STATE:
            if (data->mqtt_connected) {
                // Transição: Sim --> Ocioso: Aguardando Eventos
                *state = COREHUB_IDLE_STATE;
            } else {
                // Transição: Não --> Tenta Reconectar
                *state = COREHUB_RECONNECT_STATE;
                }
                break;

            case COREHUB_IDLE_STATE:
            // Ocioso: Aguardando Eventos
            // Não faz nada, apenas aguarda mensagens MQTT
            if (new_temp_data[ambiente_idx]) {
                data->temperature = buffered_temp[ambiente_idx];
                new_temp_data[ambiente_idx] = 0;
                if (data->door_state == 0 && data->light_state == 1 && !data->alarm_active && !data->buzzer_state) {
                    if (data->temperature > HT_COREHUB_TEMP_LIMIT_UPPER) {
                        printf("[CoreHub][%s] Temp %.1f°C > %.1f°C - Ligando AC\n", ambientes[ambiente_idx], data->temperature, HT_COREHUB_TEMP_LIMIT_UPPER);
                        *state = COREHUB_AC_ON_STATE;
                    } else if (data->temperature < HT_COREHUB_TEMP_LIMIT_LOWER) {
                        printf("[CoreHub][%s] Temp %.1f°C < %.1f°C - Desligando AC\n", ambientes[ambiente_idx], data->temperature, HT_COREHUB_TEMP_LIMIT_LOWER);
                        *state = COREHUB_AC_OFF_STATE;
                    }
                }
            }
            if (new_hum_data[ambiente_idx]) {
                data->humidity = buffered_hum[ambiente_idx];
                new_hum_data[ambiente_idx] = 0;
            }
            break;

        case COREHUB_RECONNECT_STATE:
            // Tenta reconectar e volta para verificar conexão
            *state = COREHUB_CONNECT_MQTT_STATE;
                break;

        case COREHUB_ANALYZE_DOOR_STATE:
            if (data->light_state == 1 && data->door_state == 0) {
                // Transição: Luz ON & Porta FECHADA --> Ligar Ar Condicionado
                *state = COREHUB_AC_ON_STATE;
            } else if (data->light_state == 0 || (data->door_state == 0 && data->light_state == 0)) {
                // Transição: Luz OFF OU (Porta FECHADA & Luz OFF) --> Desligar Ar Condicionado
                *state = COREHUB_AC_OFF_STATE;
            } else if (data->light_state == 1 && data->door_state == 1) {
                // Transição: Luz ON & Porta ABERTA --> Lógica do Alarme
                *state = COREHUB_ALARM_LOGIC_STATE;
                } else {
                // Nenhuma condição atendida --> Volta para Ocioso
                *state = COREHUB_IDLE_STATE;
                }
                break;

            case COREHUB_AC_ON_STATE:
            if (!data->ac_state) {
                // Verifica se veio do ANALYZE_DOOR_STATE (liga power) ou ANALYZE_TEMP_STATE (só temperatura)
                if (data->door_state == 0 && data->light_state == 1) {
                    // Veio do ANALYZE_DOOR_STATE - liga o AC
                    CoreHub_MQTTPublishWithRetry(&mqttClient_global, topic_aircontrol_power[ambiente_idx], (uint8_t*)"ON", 2, QOS0, 1, 0, 0, 3);
                    printf("[CoreHub][%s] AC LIGADO (Porta fechada + Luz ligada)\n", ambientes[ambiente_idx]);
                }
                
                // Sempre ajusta o setpoint de temperatura
                char temp_str[8];
                sprintf(temp_str, "%d", HT_COREHUB_AC_TEMP_SETPOINT);
                CoreHub_MQTTPublishWithRetry(&mqttClient_global, topic_aircontrol_temp[ambiente_idx], (uint8_t*)temp_str, strlen(temp_str), QOS0, 1, 0, 0, 3);
                data->ac_state = 1;
            }
            // Transição: Ligar Ar Condicionado --> Ocioso
            *state = COREHUB_IDLE_STATE;
                break;

            case COREHUB_AC_OFF_STATE:
            if (data->ac_state) {
                // Verifica se veio do ANALYZE_DOOR_STATE (desliga power) ou ANALYZE_TEMP_STATE (só temperatura)
                if (data->light_state == 0 || (data->door_state == 0 && data->light_state == 0)) {
                    // Veio do ANALYZE_DOOR_STATE - desliga o AC
                    CoreHub_MQTTPublishWithRetry(&mqttClient_global, topic_aircontrol_power[ambiente_idx], (uint8_t*)"OFF", 3, QOS0, 1, 0, 0, 3);
                    if (data->door_state == 0 && data->light_state == 0) {
                        printf("[CoreHub][%s] AC DESLIGADO (Porta fechada + Luz apagada)\n", ambientes[ambiente_idx]);
                    } else {
                        printf("[CoreHub][%s] AC DESLIGADO (Luz apagada)\n", ambientes[ambiente_idx]);
                    }
                }
                data->ac_state = 0;
            }
            // Transição: Desligar Ar Condicionado --> Ocioso
            *state = COREHUB_IDLE_STATE;
                break;

            case COREHUB_ALARM_LOGIC_STATE:
            if (data->door_state == 1 && data->light_state == 1 && !data->alarm_active) {
                data->alarm_active = 1;
                data->alarm_start_time = CoreHub_GetTimeSecs();
                printf("[CoreHub][%s] ALARME ATIVADO - Timer iniciado\n", ambientes[ambiente_idx]);
                // Transição: Inicia Timer (60s) --> Aguardando Timer
                *state = COREHUB_WAIT_TIMER_STATE;
            } else {
                *state = COREHUB_IDLE_STATE;
            }
                break;

            case COREHUB_WAIT_TIMER_STATE:
            if (data->alarm_active) {
                uint32_t elapsed = CoreHub_GetTimeSecs() - data->alarm_start_time;
                uint32_t timeout = HT_COREHUB_ALARM_TIMEOUT_MS / 1000;
                
                // Proteção contra overflow de tempo
                if (elapsed > 3600) { // Máximo 1 hora
                    printf("[CoreHub][%s] WATCHDOG: Timer alarme resetado (overflow)\n", ambientes[ambiente_idx]);
                    data->alarm_active = 0;
                    *state = COREHUB_IDLE_STATE;
                    break;
                }
                
                if (elapsed >= timeout) {
                    // Transição: Timer Esgotado --> Ligar Buzzer
                    *state = COREHUB_BUZZER_ON_STATE;
                    // Executa imediatamente o próximo estado
                    break;
                } else if (!data->door_state || !data->light_state) {
                    // Transição: Porta Fechou ou Luz Apagou --> Desligar Buzzer
                    *state = COREHUB_BUZZER_OFF_STATE;
                    // Executa imediatamente o próximo estado
                    break;
                }
                // Timer ainda rodando - não faz delay aqui, deixa o loop principal controlar
            } else {
                *state = COREHUB_IDLE_STATE;
            }
                break;

            case COREHUB_BUZZER_ON_STATE:
            if (!data->buzzer_state) {
                CoreHub_MQTTPublishWithRetry(&mqttClient_global, topic_smartdoor_buzzer[ambiente_idx], (uint8_t*)"ON", 2, QOS0, 1, 0, 0, 3);
                data->buzzer_state = 1;
                data->buzzer_start_time = CoreHub_GetTimeSecs(); // Registra quando ligou
                printf("[CoreHub][%s] BUZZER LIGADO\n", ambientes[ambiente_idx]);
            }
            // Transição: Ligar Buzzer --> Ocioso
            *state = COREHUB_IDLE_STATE;
                break;

            case COREHUB_BUZZER_OFF_STATE:
            if (data->buzzer_state) {
                CoreHub_MQTTPublishWithRetry(&mqttClient_global, topic_smartdoor_buzzer[ambiente_idx], (uint8_t*)"OFF", 3, QOS0, 1, 0, 0, 3);
                data->buzzer_state = 0;
                printf("[CoreHub][%s] BUZZER DESLIGADO\n", ambientes[ambiente_idx]);
            }
            
            // Desativa completamente o alarme
            if (data->alarm_active) {
                data->alarm_active = 0;
                printf("[CoreHub][%s] ALARME DESATIVADO\n", ambientes[ambiente_idx]);
            }
            
            // Transição: Desligar Buzzer --> Ocioso
            *state = COREHUB_ANALYZE_DOOR_STATE;
                break;

            default:
            *state = COREHUB_IDLE_STATE;
                break;
    }
}

/* Task MQTT global única para todos os ambientes */
void HT_CoreHub_MqttTask(void *pvParameters) {
    printf("[CoreHub] Iniciando sistema para %d ambientes\n", NUM_AMBIENTES);
    
    while (1) {
        printf("[CoreHub] Conectando ao MQTT Broker...\n");
        
        int result = HT_MQTT_Connect(&mqttClient_global, &mqttNetwork_global,
                                    (char*)broker_addr, broker_port,
                                    HT_MQTT_SEND_TIMEOUT, HT_MQTT_RECEIVE_TIMEOUT, (char*)clientID,
                                    (char*)username, (char*)password, HT_MQTT_VERSION, HT_MQTT_KEEP_ALIVE_INTERVAL,
                                    mqttSendbuf_global, HT_COREHUB_MQTT_BUFFER_SIZE,
                                    mqttReadbuf_global, HT_COREHUB_MQTT_BUFFER_SIZE);

        if (result == 0) {
            printf("[CoreHub] Conectado ao MQTT Broker\n");
            HT_MQTT_SetMessageCallback(HT_CoreHub_MessageCallback);

            // Inscreve nos tópicos de todos os ambientes
            for (int i = 0; i < NUM_AMBIENTES; i++) {
                HT_MQTT_Subscribe(&mqttClient_global, topic_smartdoor_door[i], QOS0);
                HT_MQTT_Subscribe(&mqttClient_global, topic_smartdoor_light[i], QOS0);
                HT_MQTT_Subscribe(&mqttClient_global, topic_senseclima_temp[i], QOS0);
                HT_MQTT_Subscribe(&mqttClient_global, topic_senseclima_humidity[i], QOS0);
                corehub_data[i].mqtt_connected = 1;
            }
            mqtt_connection_active = 1;
            printf("[CoreHub] Inscrito em tópicos de %d ambientes\n", NUM_AMBIENTES);

            vTaskDelay(pdMS_TO_TICKS(1000));

            while (mqtt_connection_active) {
                // Atualiza uptime para todos os ambientes
                for (int i = 0; i < NUM_AMBIENTES; i++) {
                    corehub_data[i].system_uptime = CoreHub_GetTimeSecs();
                }

                // Yield MQTT com timeout reduzido para melhor responsividade
                HT_MQTT_Yield(&mqttClient_global, 10);

                // Executa watchdog global
                CoreHub_WatchdogCheck();
                
                // Executa FSM para todos os ambientes com proteção
                for (int i = 0; i < NUM_AMBIENTES; i++) {
                    // Verifica se FSM não está travada
                    if (!fsm_stuck_detected[i]) {
                        HT_CoreHub_StateMachine(i);
                    } else {
                        // Reset FSM travada
                        printf("[CoreHub][%s] Resetando FSM travada\n", ambientes[i]);
                        current_state[i] = COREHUB_IDLE_STATE;
                        fsm_stuck_detected[i] = 0;
                        fsm_execution_count[i] = 0;
                    }
                }

                // Verifica se algum ambiente perdeu conexão
                int all_connected = 1;
                for (int i = 0; i < NUM_AMBIENTES; i++) {
                    if (!corehub_data[i].mqtt_connected) {
                        all_connected = 0;
                        break;
                    }
                }
                
                if (!all_connected) {
                    printf("[CoreHub] Conexão MQTT perdida\n");
                    break;
                }

                // Delay otimizado para melhor performance
                vTaskDelay(pdMS_TO_TICKS(50));
            }

            printf("[CoreHub] Desconectando do MQTT Broker\n");
            HT_MQTT_Disconnect(&mqttClient_global);
            for (int i = 0; i < NUM_AMBIENTES; i++) {
                corehub_data[i].mqtt_connected = 0;
            }
            mqtt_connection_active = 0;
        } else {
            printf("[CoreHub] Falha na conexão MQTT (erro: %d)\n", result);
        }

        printf("[CoreHub] Aguardando 5s para reconectar...\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
