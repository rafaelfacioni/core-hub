# 📋 Especificações Técnicas - CoreHub Sistema de Automação Inteligente

## 🎯 Visão Geral do Sistema

### Objetivo
O CoreHub é um sistema central de automação inteligente que integra múltiplos ambientes através de comunicação MQTT via **NB-IoT**, implementando lógica de controle automático baseada em condições ambientais e de segurança.

### Escopo
- **3 Ambientes**: externo, mesanino, prototipagem
- **3 Dispositivos**: SmartDoor, SenseClima, AirControl
- **1 Cliente MQTT**: Gerenciamento centralizado
- **1 FSM**: Máquina de estados para cada ambiente

---

## 🏗️ Arquitetura Técnica

### Hardware
- **MCU**: HTNB32L (ARM Cortex-M3)
- **Memória**: Flash + RAM conforme especificação do chip
- **Comunicação**: **NB-IoT** via SIM Card
- **Banda**: 28 (LTE Cat-NB1)
- **APN**: iot.datatem.com.br
- **Debug**: UART (115200 baud)

### Software
- **OS**: FreeRTOS
- **Protocolo**: MQTT v3.1.1
- **Linguagem**: C (ANSI C99)
- **Compilador**: ARM GCC

### Estrutura de Arquivos
```
Firmware/
├── Applications/Template/
│   ├── Inc/
│   │   ├── HT_CoreHubFsm.h      # Header principal
│   │   └── CoreHub_Config.h     # Configurações
│   └── Src/
│       ├── main.c               # Entry point
│       ├── HT_CoreHubFsm.c      # Implementação FSM
│       └── HT_MQTT_Api.c        # API MQTT
├── SDK/                         # SDK HTNB32L
└── Build/                       # Arquivos compilados
```

---

## 🔧 Especificações Funcionais

### 1. Controle de Ar Condicionado

#### Condições de Ativação
- **Ligar AC**: 
  - Temperatura > 28°C **E**
  - Porta FECHADA **E**
  - Luz LIGADA **E**
  - Alarme INATIVO **E**
  - Buzzer DESLIGADO

- **Desligar AC**:
  - Temperatura < 24°C **OU**
  - Luz DESLIGADA

#### Ações Executadas
```c
// Ligar AC
1. Publicar: hana/{ambiente}/aircontrol/01/power = "ON"
2. Publicar: hana/{ambiente}/aircontrol/01/temperature = "22"
3. Atualizar estado local: ac_state = 1

// Desligar AC
1. Publicar: hana/{ambiente}/aircontrol/01/power = "OFF"
2. Atualizar estado local: ac_state = 0
```

### 2. Sistema de Alarme

#### Condições de Ativação
- **Ativar Alarme**:
  - Porta ABERTA **E**
  - Luz LIGADA **E**
  - Alarme INATIVO

#### Sequência de Alarme
```c
1. Ativar alarme: alarm_active = 1
2. Iniciar timer: alarm_start_time = current_time
3. Aguardar 60 segundos
4. Se timer esgotado: Ligar buzzer
5. Se porta fechar OU luz apagar: Desligar buzzer
```

#### Ações do Buzzer
```c
// Ligar Buzzer
1. Publicar: hana/{ambiente}/smartdoor/buzzer = "ON"
2. Atualizar estado: buzzer_state = 1
3. Registrar tempo: buzzer_start_time = current_time

// Desligar Buzzer
1. Publicar: hana/{ambiente}/smartdoor/buzzer = "OFF"
2. Atualizar estado: buzzer_state = 0
3. Desativar alarme: alarm_active = 0
```

### 3. Monitoramento Ambiental

#### Processamento de Dados
```c
// Temperatura
- Receber: hana/{ambiente}/senseclima/01/temperature
- Converter string para float
- Armazenar em corehub_data[ambiente].temperature
- Executar lógica de controle de AC

// Umidade
- Receber: hana/{ambiente}/senseclima/01/humidity
- Converter string para float
- Armazenar em corehub_data[ambiente].humidity
- Log para monitoramento
```

---

## 📡 Especificações de Comunicação

### MQTT Broker via NB-IoT
- **Endereço**: 131.255.82.115
- **Porta**: 1883 (TCP)
- **Protocolo**: MQTT v3.1.1
- **QoS**: 0 (At most once)
- **Keep Alive**: 240 segundos
- **Client ID**: corehub01
- **Conectividade**: NB-IoT (LTE Cat-NB1)
- **APN**: iot.datatem.com.br
- **Banda**: 28

### Tópicos MQTT

#### SmartDoor
```
hana/{ambiente}/smartdoor/door
- Payload: "OPEN" | "CLOSED"
- Direção: Subscribe
- Retain: Sim

hana/{ambiente}/smartdoor/light
- Payload: "ON" | "OFF"
- Direção: Subscribe
- Retain: Sim

hana/{ambiente}/smartdoor/buzzer
- Payload: "ON" | "OFF"
- Direção: Publish
- Retain: Sim
```

#### SenseClima
```
hana/{ambiente}/senseclima/01/temperature
- Payload: float (ex: "25.6")
- Direção: Subscribe
- Retain: Sim

hana/{ambiente}/senseclima/01/humidity
- Payload: float (ex: "65.2")
- Direção: Subscribe
- Retain: Sim
```

#### AirControl
```
hana/{ambiente}/aircontrol/01/power
- Payload: "ON" | "OFF"
- Direção: Publish
- Retain: Sim

hana/{ambiente}/aircontrol/01/temperature
- Payload: int (ex: "22")
- Direção: Publish
- Retain: Sim
```

### Estrutura de Tópicos
```
hana/
├── externo/
│   ├── smartdoor/
│   ├── senseclima/
│   └── aircontrol/
├── mesanino/
│   ├── smartdoor/
│   ├── senseclima/
│   └── aircontrol/
└── prototipagem/
    ├── smartdoor/
    ├── senseclima/
    └── aircontrol/
```

---

## ⚙️ Especificações de Performance

### Tempos de Resposta
- **Processamento MQTT**: < 50ms
- **Transição de Estado**: < 100ms
- **Publicação de Comando**: < 200ms
- **Reconexão MQTT**: < 5 segundos

### Uso de Recursos
- **Memória RAM**: ~35KB
- **Memória Flash**: ~1.5MB
- **CPU**: ~20% (média)
- **Rede NB-IoT**: ~1KB/min (tráfego MQTT)
- **Consumo de Energia**: Otimizado para NB-IoT
- **Cobertura**: Rede celular de baixa frequência

### Confiabilidade
- **Retry MQTT**: 3 tentativas
- **Delay entre retries**: 200ms
- **Reconexão automática**: Sim
- **Logs de erro**: Detalhados

---

## 🔧 Especificações de Configuração

### Parâmetros do Sistema
```c
// Limites de Temperatura
#define HT_COREHUB_TEMP_LIMIT_UPPER    28.0f
#define HT_COREHUB_TEMP_LIMIT_LOWER    24.0f

// Timeout do Alarme
#define HT_COREHUB_ALARM_TIMEOUT_MS    60000

// Setpoint do AC
#define HT_COREHUB_AC_TEMP_SETPOINT    22

// Configurações MQTT
#define HT_COREHUB_MQTT_BUFFER_SIZE    1024
#define HT_MQTT_KEEP_ALIVE_INTERVAL    240
#define HT_MQTT_SEND_TIMEOUT           60000
#define HT_MQTT_RECEIVE_TIMEOUT        60000
```

### Configurações FreeRTOS
```c
// Prioridades
#define HT_COREHUB_MQTT_TASK_PRIORITY  (configMAX_PRIORITIES - 1)

// Stack Sizes
#define HT_COREHUB_MQTT_TASK_STACK_SIZE 2048

// Timeouts
#define APP_EVENT_QUEUE_SIZE           10
```

---

## 🔄 Especificações da FSM

### Estados da Máquina
```c
typedef enum {
    COREHUB_INIT_STATE = 0,           // Inicialização
    COREHUB_CONNECT_MQTT_STATE,       // Verificar conexão MQTT
    COREHUB_IDLE_STATE,               // Estado de espera
    COREHUB_RECONNECT_STATE,          // Tentar reconectar
    COREHUB_ANALYZE_DOOR_STATE,       // Analisar porta/luz
    COREHUB_ANALYZE_TEMP_STATE,       // Analisar temperatura
    COREHUB_AC_ON_STATE,              // Ligar ar condicionado
    COREHUB_AC_OFF_STATE,             // Desligar ar condicionado
    COREHUB_ALARM_LOGIC_STATE,        // Lógica do alarme
    COREHUB_WAIT_TIMER_STATE,         // Aguardar timer
    COREHUB_BUZZER_ON_STATE,          // Ligar buzzer
    COREHUB_BUZZER_OFF_STATE          // Desligar buzzer
} CoreHub_FSM_States;
```

### Transições de Estado
```c
// Transições principais
INIT_STATE → CONNECT_MQTT_STATE
CONNECT_MQTT_STATE → IDLE_STATE (MQTT OK)
CONNECT_MQTT_STATE → RECONNECT_STATE (MQTT Fail)
RECONNECT_STATE → CONNECT_MQTT_STATE

// Transições baseadas em eventos
IDLE_STATE → ANALYZE_DOOR_STATE (Evento porta/luz)
IDLE_STATE → AC_ON_STATE (Temp > 28°C)
IDLE_STATE → AC_OFF_STATE (Temp < 24°C)

// Transições de análise
ANALYZE_DOOR_STATE → AC_ON_STATE (Luz ON + Porta FECHADA)
ANALYZE_DOOR_STATE → AC_OFF_STATE (Luz OFF)
ANALYZE_DOOR_STATE → ALARM_LOGIC_STATE (Luz ON + Porta ABERTA)

// Transições de alarme
ALARM_LOGIC_STATE → WAIT_TIMER_STATE (Condições OK)
WAIT_TIMER_STATE → BUZZER_ON_STATE (Timer esgotado)
WAIT_TIMER_STATE → BUZZER_OFF_STATE (Porta fechou/Luz apagou)

// Transições de ação
AC_ON_STATE → IDLE_STATE
AC_OFF_STATE → IDLE_STATE
BUZZER_ON_STATE → IDLE_STATE
BUZZER_OFF_STATE → ANALYZE_DOOR_STATE
```

---

## 📊 Especificações de Dados

### Estrutura de Dados Principal
```c
typedef struct {
    float temperature;                   // Temperatura atual (°C)
    float humidity;                      // Umidade atual (%)
    uint8_t door_state;                  // Estado da porta (0=CLOSED, 1=OPEN)
    uint8_t light_state;                 // Estado da luz (0=OFF, 1=ON)
    uint8_t ac_state;                    // Estado do AC (0=OFF, 1=ON)
    uint8_t buzzer_state;                // Estado do buzzer (0=OFF, 1=ON)
    uint32_t alarm_start_time;           // Tempo quando alarme iniciou (s)
    uint32_t buzzer_start_time;          // Tempo quando buzzer foi ligado (s)
    uint32_t system_uptime;              // Tempo de funcionamento (s)
    uint8_t mqtt_connected;              // Status da conexão MQTT (0=OFF, 1=ON)
    uint8_t alarm_active;                // Status do alarme (0=INACTIVE, 1=ACTIVE)
} CoreHub_Data_t;
```

### Arrays de Dados
```c
// Dados por ambiente
CoreHub_Data_t corehub_data[NUM_AMBIENTES];
CoreHub_FSM_States current_state[NUM_AMBIENTES];

// Tópicos por ambiente
char topic_smartdoor_door[NUM_AMBIENTES][64];
char topic_smartdoor_light[NUM_AMBIENTES][64];
char topic_smartdoor_buzzer[NUM_AMBIENTES][64];
char topic_senseclima_temp[NUM_AMBIENTES][64];
char topic_senseclima_humidity[NUM_AMBIENTES][64];
char topic_aircontrol_power[NUM_AMBIENTES][64];
char topic_aircontrol_temp[NUM_AMBIENTES][64];
```

---

## 🔍 Especificações de Logging

### Níveis de Log
```c
// Logs de Informação
printf("CoreHub - Conectado ao MQTT Broker!\n");
printf("CoreHub[%s] - Temperatura: %d.%d°C\n", ambiente, temp_int, temp_dec);

// Logs de Debug
printf("CoreHub - DEBUG: Valor da temperatura na struct = %d.%d\n", temp_int, temp_dec);
printf("CoreHub - FSM: %s\n", state_name);

// Logs de Erro
printf("[ERRO] Falha na conexão MQTT (erro: %d)\n", error_code);
printf("[ERRO] Ambiente não identificado para tópico: %s\n", topic);

// Logs de Estado
printf("CoreHub - FSM: Luz ON & Porta FECHADA --> AC_ON_STATE\n");
printf("CoreHub - Publicado: %s = ON\n", topic);
```

### Formato de Logs
```
CoreHub[ambiente] - tipo: mensagem
CoreHub - FSM: estado_atual
[ERRO] descrição_do_erro
[INFO] informação_importante
```

---

## 🛡️ Especificações de Segurança

### Validação de Dados
- **Tópicos**: Validação de formato e ambiente
- **Payloads**: Verificação de tamanho e conteúdo
- **Estados**: Validação de transições permitidas
- **Tempos**: Verificação de overflow e valores negativos

### Tratamento de Erros
```c
// Validação de ambiente
if (ambiente_idx < 0 || ambiente_idx >= NUM_AMBIENTES) {
    printf("[ERRO] Ambiente inválido: %d\n", ambiente_idx);
    return;
}

// Validação de payload
if (payload_len >= sizeof(buffer)) {
    printf("[ERRO] Payload muito grande: %d\n", payload_len);
    return;
}

// Retry com timeout
for (int attempt = 1; attempt <= max_retries; ++attempt) {
    rc = HT_MQTT_Publish(...);
    if (rc == 0) break;
    vTaskDelay(pdMS_TO_TICKS(200));
}
```

---

## 📈 Especificações de Monitoramento

### Métricas de Sistema
- **Uptime**: Tempo de funcionamento contínuo
- **MQTT Status**: Estado da conexão
- **Estado FSM**: Estado atual de cada ambiente
- **Contadores**: Mensagens recebidas/enviadas

### Health Check
```c
// Verificação periódica
void HT_CoreHub_HealthCheck(void) {
    for (int i = 0; i < NUM_AMBIENTES; i++) {
        printf("Ambiente %s: Estado=%d, MQTT=%s, Uptime=%lu\n",
               ambientes[i], current_state[i],
               corehub_data[i].mqtt_connected ? "OK" : "FAIL",
               corehub_data[i].system_uptime);
    }
}
```

---

## 🔄 Especificações de Manutenção

### Atualizações
- **Firmware**: Via UART ou OTA
- **Configurações**: Via MQTT ou recompilação
- **Logs**: Via UART ou MQTT

### Backup e Recuperação
- **Configurações**: Armazenadas em flash
- **Estados**: Recuperação automática após reset
- **Dados**: Persistência via MQTT retain

---

*Especificações técnicas - CoreHub v1.0* 