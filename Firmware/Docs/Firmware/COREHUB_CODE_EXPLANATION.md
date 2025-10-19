# CoreHub - Explicação do Código

## 📋 Índice
1. [Estrutura de Dados](#estrutura-de-dados)
2. [Estados da FSM](#estados-da-fsm)
3. [Inicialização do Sistema](#inicialização-do-sistema)
4. [Callback de Mensagens MQTT](#callback-de-mensagens-mqtt)
5. [Máquina de Estados](#máquina-de-estados)
6. [Sistema de Watchdog](#sistema-de-watchdog)
7. [Task Principal MQTT](#task-principal-mqtt)
8. [Funções de Suporte](#funções-de-suporte)

---

## 🏗️ Estrutura de Dados

### Definição dos Estados da FSM
```c
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
```

**Explicação**: Define todos os estados possíveis da máquina de estados finita (FSM) do CoreHub, seguindo exatamente o diagrama de fluxo especificado.

### Estrutura de Dados por Ambiente
```c
typedef struct {
    float temperature;           // Temperatura atual
    float humidity;              // Umidade atual
    uint8_t door_state;          // 0=CLOSED, 1=OPEN
    uint8_t light_state;         // 0=OFF, 1=ON
    uint8_t ac_state;            // 0=OFF, 1=ON
    uint8_t buzzer_state;        // 0=OFF, 1=ON
    uint32_t alarm_start_time;   // Tempo de início do alarme
    uint32_t buzzer_start_time;  // Tempo de início do buzzer
    uint32_t system_uptime;      // Uptime do sistema
    uint8_t mqtt_connected;      // Status da conexão MQTT
    uint8_t alarm_active;        // 0=INACTIVE, 1=ACTIVE
} CoreHub_Data_t;
```

**Explicação**: Estrutura que armazena todos os dados de um ambiente específico, incluindo estados dos dispositivos, timers e informações de conectividade.

### Variáveis Globais de Performance
```c
// Buffers de dados por ambiente (otimização de performance)
static volatile int new_temp_data[NUM_AMBIENTES] = {0};
static volatile int new_hum_data[NUM_AMBIENTES] = {0};
static float buffered_temp[NUM_AMBIENTES] = {0.0f};
static float buffered_hum[NUM_AMBIENTES] = {0.0f};

// Controle de performance e watchdog
static uint32_t last_fsm_execution[NUM_AMBIENTES] = {0};
static uint32_t fsm_execution_count[NUM_AMBIENTES] = {0};
static uint8_t fsm_stuck_detected[NUM_AMBIENTES] = {0};
```

**Explicação**: Sistema de buffers isolados por ambiente para evitar conflitos de dados e controle de performance para prevenir travamentos.

---

## 🔄 Estados da FSM

### Estado IDLE - Processamento de Temperatura
```c
case COREHUB_IDLE_STATE:
    if (new_temp_data[ambiente_idx]) {
        data->temperature = buffered_temp[ambiente_idx];
        new_temp_data[ambiente_idx] = 0;
        if (data->door_state == 0 && data->light_state == 1 && 
            !data->alarm_active && !data->buzzer_state) {
            if (data->temperature > HT_COREHUB_TEMP_LIMIT_UPPER) {
                printf("[CoreHub][%s] Temp %.1f°C > %.1f°C - Ligando AC\n", 
                       ambientes[ambiente_idx], data->temperature, HT_COREHUB_TEMP_LIMIT_UPPER);
                *state = COREHUB_AC_ON_STATE;
            } else if (data->temperature < HT_COREHUB_TEMP_LIMIT_LOWER) {
                printf("[CoreHub][%s] Temp %.1f°C < %.1f°C - Desligando AC\n", 
                       ambientes[ambiente_idx], data->temperature, HT_COREHUB_TEMP_LIMIT_LOWER);
                *state = COREHUB_AC_OFF_STATE;
            }
        }
    }
    break;
```

**Explicação**: No estado ocioso, o sistema processa dados de temperatura apenas quando as condições são adequadas (porta fechada, luz ligada, sem alarme ativo). Controla o AC baseado nos limites de temperatura configurados.

### Estado ANALYZE_DOOR - Lógica de Porta e Luz
```c
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
```

**Explicação**: Analisa as condições de porta e luz para decidir se deve ligar/desligar o AC ou ativar o alarme. A lógica corrigida agora considera o cenário de porta fechada + luz apagada.

---

## 🚀 Inicialização do Sistema

### Configuração de Rede NB-IoT
```c
static void HT_SetConnectioParameters(void) {
    uint8_t cid = 0;
    PsAPNSetting apnSetting;
    int32_t ret;
    uint8_t networkMode = 0; //nb-iot network mode
    uint8_t bandNum = 1;
    uint8_t band = 28;

    ret = appSetBandModeSync(networkMode, bandNum, &band);
    if(ret == CMS_RET_SUCC) {
        printf("SetBand Result: %d\n", ret);
    }

    apnSetting.cid = 0;
    apnSetting.apnLength = strlen("iot.datatem.com.br");
    strcpy((char *)apnSetting.apnStr, "iot.datatem.com.br");
    apnSetting.pdnType = CMI_PS_PDN_TYPE_IP_V4V6;
    ret = appSetAPNSettingSync(&apnSetting, &cid);
}
```

**Explicação**: Configura os parâmetros de rede NB-IoT, incluindo banda 28 e APN específico para IoT, garantindo conectividade LTE adequada.

### Inicialização de Ambientes
```c
// Inicializa todos os ambientes
for (int i = 0; i < NUM_AMBIENTES; ++i) {
    HT_CoreHub_InitAmbiente(i, ambientes[i]);
}

// Cria uma única task global para todos os ambientes
xTaskCreate(HT_CoreHub_MqttTask, "CoreHub_Global", 
           HT_COREHUB_MQTT_TASK_STACK_SIZE, NULL, 
           HT_COREHUB_MQTT_TASK_PRIORITY, NULL);
```

**Explicação**: Inicializa todos os ambientes (externo, mesanino, prototipagem) e cria uma única task FreeRTOS para gerenciar todos eles, otimizando o uso de recursos.

---

## 📡 Callback de Mensagens MQTT

### Processamento de Mensagens
```c
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
        printf("[CoreHub] ERRO: Mensagem MQTT muito grande (topic:%zu, payload:%zu)\n", 
               topic_len, payload_len);
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
```

**Explicação**: Callback que recebe todas as mensagens MQTT, com proteções robustas contra dados inválidos e identificação automática do ambiente baseada no tópico.

### Processamento de Dados de Sensores
```c
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
else if (strstr(topic, "senseclima/01/temperature")) {
    float temperature = string_to_float(payload);
    buffered_temp[ambiente_idx] = temperature;
    new_temp_data[ambiente_idx] = 1;
}
```

**Explicação**: Processa diferentes tipos de mensagens MQTT, atualizando os estados dos dispositivos e bufferizando dados de sensores para processamento posterior na FSM.

---

## 🧠 Máquina de Estados

### Proteções de Performance
```c
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
```

**Explicação**: Implementa proteções críticas para evitar travamentos, incluindo controle de frequência de execução e sistema de watchdog para detectar e recuperar FSMs travadas.

### Estado AC_ON - Controle de Ar Condicionado
```c
case COREHUB_AC_ON_STATE:
    if (!data->ac_state) {
        // Verifica se veio do ANALYZE_DOOR_STATE (liga power) ou ANALYZE_TEMP_STATE (só temperatura)
        if (data->door_state == 0 && data->light_state == 1) {
            // Veio do ANALYZE_DOOR_STATE - liga o AC
            CoreHub_MQTTPublishWithRetry(&mqttClient_global, topic_aircontrol_power[ambiente_idx], 
                                       (uint8_t*)"ON", 2, QOS0, 1, 0, 0, 3);
            printf("[CoreHub][%s] AC LIGADO (Porta fechada + Luz ligada)\n", ambientes[ambiente_idx]);
        }
        
        // Sempre ajusta o setpoint de temperatura
        char temp_str[8];
        sprintf(temp_str, "%d", HT_COREHUB_AC_TEMP_SETPOINT);
        CoreHub_MQTTPublishWithRetry(&mqttClient_global, topic_aircontrol_temp[ambiente_idx], 
                                   (uint8_t*)temp_str, strlen(temp_str), QOS0, 1, 0, 0, 3);
        data->ac_state = 1;
    }
    // Transição: Ligar Ar Condicionado --> Ocioso
    *state = COREHUB_IDLE_STATE;
    break;
```

**Explicação**: Estado responsável por ligar o ar condicionado, publicando comandos MQTT para power ON e ajustando o setpoint de temperatura. Distingue entre diferentes origens (porta/luz vs temperatura).

---

## 🛡️ Sistema de Watchdog

### Watchdog Global
```c
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
```

**Explicação**: Sistema de monitoramento global que verifica a saúde de todas as FSMs, detecta travamentos e fornece logs de status do sistema.

### Proteção de Timer de Alarme
```c
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
            break;
        } else if (!data->door_state || !data->light_state) {
            // Transição: Porta Fechou ou Luz Apagou --> Desligar Buzzer
            *state = COREHUB_BUZZER_OFF_STATE;
            break;
        }
    } else {
        *state = COREHUB_IDLE_STATE;
    }
    break;
```

**Explicação**: Estado que gerencia o timer do alarme com proteção contra overflow de tempo, garantindo que o sistema não trave em loops infinitos.

---

## 🔄 Task Principal MQTT

### Loop Principal com Proteções
```c
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
```

**Explicação**: Loop principal que gerencia todas as operações do CoreHub, incluindo processamento MQTT, execução das FSMs, monitoramento de saúde e recuperação de falhas.

---

## 🛠️ Funções de Suporte

### Publicação MQTT com Retry
```c
static int CoreHub_MQTTPublishWithRetry(MQTTClient *mqtt_client, char *topic, 
                                       uint8_t *payload, uint32_t len, enum QoS qos, 
                                       uint8_t retained, uint16_t id, uint8_t dup, int max_retries) {
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
```

**Explicação**: Função robusta para publicação MQTT com sistema de retry progressivo, proteção contra parâmetros inválidos e logs de erro detalhados.

### Identificação de Ambiente por Tópico
```c
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
```

**Explicação**: Função que identifica automaticamente qual ambiente uma mensagem MQTT pertence baseada no prefixo do tópico (ex: "hana/externo/", "hana/mesanino/", etc.).

### Conversão de String para Float
```c
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
```

**Explicação**: Implementação manual de conversão string para float, otimizada para sistemas embarcados sem dependência de bibliotecas externas.

---

## 📊 Resumo da Arquitetura

### Características Principais
- **Multi-ambiente**: Suporte a múltiplos ambientes simultâneos
- **FSM Robusta**: Máquina de estados com proteções contra travamentos
- **Comunicação NB-IoT**: Conectividade celular otimizada para IoT
- **Watchdog Global**: Sistema de monitoramento e recuperação automática
- **Performance Otimizada**: Controles de execução e buffers isolados
- **Logs Essenciais**: Monitoramento sem poluição de dados

### Fluxo de Dados
1. **Recepção MQTT** → Callback processa mensagens
2. **Bufferização** → Dados armazenados por ambiente
3. **Processamento FSM** → Lógica de decisão baseada em estados
4. **Ação MQTT** → Comandos enviados para dispositivos
5. **Monitoramento** → Watchdog verifica saúde do sistema

### Tecnologias Utilizadas
- **FreeRTOS**: Sistema operacional em tempo real
- **MQTT**: Protocolo de comunicação IoT
- **NB-IoT**: Conectividade celular de baixo consumo
- **FSM**: Máquina de estados finita para lógica de controle
- **Watchdog**: Sistema de monitoramento e recuperação 