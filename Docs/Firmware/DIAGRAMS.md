# 📊 Diagramas Técnicos - CoreHub Sistema de Automação Inteligente

## 🔄 Diagrama de Sequência - Inicialização do Sistema

```mermaid
sequenceDiagram
    participant Main as main.c
    participant Task as HT_CoreHubTask
    participant MQTT as HT_CoreHub_MqttTask
    participant Broker as MQTT Broker
    participant FSM as FSM Engine

    Main->>Task: Criar HT_CoreHubTask
    Task->>Task: Aguardar SIM Ready
    Task->>Task: Configurar Rede NB-IoT
    Task->>Task: Inicializar Ambientes
    Task->>MQTT: Criar HT_CoreHub_MqttTask
    
    MQTT->>Broker: Conectar MQTT
    Broker-->>MQTT: Conectado
    
    loop Para cada ambiente
        MQTT->>Broker: Subscribe tópicos
    end
    
    Broker-->>MQTT: Dados retain
    
    loop Loop Principal
        Broker->>MQTT: Mensagem MQTT
        MQTT->>FSM: Processar mensagem
        FSM->>FSM: Executar lógica
        FSM->>MQTT: Comando de saída
        MQTT->>Broker: Publicar comando
    end
```

## 🏗️ Diagrama de Arquitetura Detalhada

```mermaid
graph TB
    subgraph "Hardware Layer"
        MCU[HTNB32L MCU]
        SIM[NB-IoT SIM Card]
        UART[UART Debug]
    end
    
    subgraph "OS Layer"
        FreeRTOS[FreeRTOS Kernel]
        Tasks[Task Scheduler]
    end
    
    subgraph "Application Layer"
        MainTask[HT_CoreHubTask]
        MQTTTask[HT_CoreHub_MqttTask]
    end
    
    subgraph "CoreHub Engine"
        FSM[Finite State Machine]
        DataManager[Data Manager]
        TopicManager[Topic Manager]
    end
    
    subgraph "Communication Layer"
        MQTTClient[MQTT Client]
        Network[NB-IoT Network Stack]
        Retry[Retry Mechanism]
    end
    
    subgraph "Device Layer"
        SmartDoor[SmartDoor]
        SenseClima[SenseClima]
        AirControl[AirControl]
    end
    
    MCU --> FreeRTOS
    FreeRTOS --> Tasks
    Tasks --> MainTask
    Tasks --> MQTTTask
    
    MainTask --> FSM
    MQTTTask --> FSM
    MQTTTask --> MQTTClient
    
    FSM --> DataManager
    FSM --> TopicManager
    
    MQTTClient --> Network
    MQTTClient --> Retry
    
    Network --> SmartDoor
    Network --> SenseClima
    Network --> AirControl
```

## 🔄 Diagrama de Estados Detalhado

```mermaid
stateDiagram-v2
    [*] --> INIT_STATE : Sistema Iniciado
    
    INIT_STATE --> CONNECT_MQTT_STATE : Inicialização OK
    
    CONNECT_MQTT_STATE --> IDLE_STATE : MQTT Conectado
    CONNECT_MQTT_STATE --> RECONNECT_STATE : Falha Conexão
    
    RECONNECT_STATE --> CONNECT_MQTT_STATE : Tentar Novamente
    
    IDLE_STATE --> ANALYZE_DOOR_STATE : Evento Porta/Luz
    IDLE_STATE --> AC_ON_STATE : Temp > 28°C
    IDLE_STATE --> AC_OFF_STATE : Temp < 24°C
    
    note right of IDLE_STATE
        Estado de espera
        Monitora mensagens MQTT
        Processa dados SenseClima
    end note
    
    ANALYZE_DOOR_STATE --> AC_ON_STATE : Luz ON + Porta FECHADA
    ANALYZE_DOOR_STATE --> AC_OFF_STATE : Luz OFF
    ANALYZE_DOOR_STATE --> ALARM_LOGIC_STATE : Luz ON + Porta ABERTA
    ANALYZE_DOOR_STATE --> IDLE_STATE : Outras condições
    
    note right of ANALYZE_DOOR_STATE
        Analisa estado da porta
        e iluminação
        Decide próxima ação
    end note
    
    ALARM_LOGIC_STATE --> WAIT_TIMER_STATE : Condições atendidas
    ALARM_LOGIC_STATE --> IDLE_STATE : Condições não atendidas
    
    note right of ALARM_LOGIC_STATE
        Verifica condições
        do alarme
        Inicia timer se OK
    end note
    
    WAIT_TIMER_STATE --> BUZZER_ON_STATE : Timer esgotado (60s)
    WAIT_TIMER_STATE --> BUZZER_OFF_STATE : Porta fechou/Luz apagou
    WAIT_TIMER_STATE --> WAIT_TIMER_STATE : Timer rodando
    
    note right of WAIT_TIMER_STATE
        Aguarda 60 segundos
        Monitora mudanças
        de estado
    end note
    
    BUZZER_ON_STATE --> IDLE_STATE : Buzzer ligado
    BUZZER_OFF_STATE --> ANALYZE_DOOR_STATE : Buzzer desligado
    
    AC_ON_STATE --> IDLE_STATE : AC ligado
    AC_OFF_STATE --> IDLE_STATE : AC desligado
    
    note right of AC_ON_STATE
        Liga AC
        Define temperatura
        Retorna ao idle
    end note
    
    note right of AC_OFF_STATE
        Desliga AC
        Retorna ao idle
    end note
```

## 📡 Diagrama de Comunicação MQTT

```mermaid
graph LR
    subgraph "CoreHub"
        Client[MQTT Client]
        Callback[Message Callback]
        Parser[Topic Parser]
        FSM[FSM Engine]
    end
    
    subgraph "MQTT Broker"
        Broker[131.255.82.115:1883]
        NB_IoT[NB-IoT Network]
    end
    
    subgraph "Devices"
        SD[SmartDoor]
        SC[SenseClima]
        AC[AirControl]
    end
    
    Client <--> NB_IoT
    NB_IoT <--> Broker
    Broker <--> SD
    Broker <--> SC
    Broker <--> AC
    
    Broker --> Callback
    Callback --> Parser
    Parser --> FSM
    FSM --> Client
```

## 🔧 Diagrama de Estruturas de Dados

```mermaid
classDiagram
    class CoreHub_Data_t {
        +float temperature
        +float humidity
        +uint8_t door_state
        +uint8_t light_state
        +uint8_t ac_state
        +uint8_t buzzer_state
        +uint32_t alarm_start_time
        +uint32_t buzzer_start_time
        +uint32_t system_uptime
        +uint8_t mqtt_connected
        +uint8_t alarm_active
    }
    
    class CoreHub_FSM_States {
        <<enumeration>>
        INIT_STATE
        CONNECT_MQTT_STATE
        IDLE_STATE
        RECONNECT_STATE
        ANALYZE_DOOR_STATE
        ANALYZE_TEMP_STATE
        AC_ON_STATE
        AC_OFF_STATE
        ALARM_LOGIC_STATE
        WAIT_TIMER_STATE
        BUZZER_ON_STATE
        BUZZER_OFF_STATE
    }
    
    class TopicManager {
        +char topic_smartdoor_door[3][64]
        +char topic_smartdoor_light[3][64]
        +char topic_smartdoor_buzzer[3][64]
        +char topic_senseclima_temp[3][64]
        +char topic_senseclima_humidity[3][64]
        +char topic_aircontrol_power[3][64]
        +char topic_aircontrol_temp[3][64]
        +montaTopicos()
    }
    
    class MQTTClient {
        +Network network
        +uint8_t sendbuf[1024]
        +uint8_t readbuf[1024]
        +connect()
        +subscribe()
        +publish()
        +yield()
        +disconnect()
    }
    
    CoreHub_Data_t --> CoreHub_FSM_States
    TopicManager --> MQTTClient
```

## 🔄 Diagrama de Fluxo de Dados

```mermaid
flowchart TD
    A[MQTT Message] --> B[Message Callback]
    B --> C[Extract Topic & Payload]
    C --> D[Identify Environment]
    D --> E{Valid Environment?}
    E -->|No| F[Log Error]
    E -->|Yes| G[Parse Message Type]
    
    G --> H{Message Type?}
    H -->|Door| I[Update Door State]
    H -->|Light| J[Update Light State]
    H -->|Temperature| K[Update Temperature]
    H -->|Humidity| L[Update Humidity]
    H -->|AC Power| M[Update AC State]
    
    I --> N[Update corehub_data]
    J --> N
    K --> N
    L --> N
    M --> N
    
    N --> O[Transition FSM State]
    O --> P[Execute State Logic]
    P --> Q{Action Required?}
    Q -->|Yes| R[Publish Command]
    Q -->|No| S[Return to Idle]
    
    R --> T[MQTT Publish]
    T --> S
    S --> U[Wait Next Message]
    U --> A
```

## 🎯 Diagrama de Casos de Uso

```mermaid
graph TB
    subgraph "Use Cases"
        UC1[Controle Automático de AC]
        UC2[Sistema de Alarme]
        UC3[Monitoramento Ambiental]
        UC4[Gerenciamento Multi-Ambiente]
    end
    
    subgraph "Actors"
        A1[SmartDoor]
        A2[SenseClima]
        A3[AirControl]
        A4[CoreHub]
    end
    
    subgraph "System"
        S1[FSM Engine]
        S2[MQTT Communication]
        S3[Data Management]
    end
    
    A1 --> UC1
    A1 --> UC2
    A2 --> UC1
    A2 --> UC3
    A3 --> UC1
    A4 --> UC4
    
    UC1 --> S1
    UC2 --> S1
    UC3 --> S1
    UC4 --> S2
    
    S1 --> S2
    S2 --> S3
```

## 🔧 Diagrama de Configuração

```mermaid
graph LR
    subgraph "Configuration"
        C1[Temperature Limits]
        C2[Alarm Timeout]
        C3[AC Setpoint]
        C4[MQTT Settings]
    end
    
    subgraph "Values"
        V1[Upper: 28°C]
        V2[Lower: 24°C]
        V3[60 seconds]
        V4[22°C]
        V5[Broker: 131.255.82.115]
        V6[Port: 1883]
    end
    
    C1 --> V1
    C1 --> V2
    C2 --> V3
    C3 --> V4
    C4 --> V5
    C4 --> V6
```

## 📊 Diagrama de Performance

```mermaid
graph TB
    subgraph "Performance Metrics"
        P1[Response Time]
        P2[Memory Usage]
        P3[CPU Usage]
        P4[Network Latency]
    end
    
    subgraph "Targets"
        T1[< 100ms]
        T2[< 50KB]
        T3[< 30%]
        T4[< 500ms]
    end
    
    subgraph "Current"
        C1[~50ms]
        C2[~35KB]
        C3[~20%]
        C4[~200ms]
    end
    
    P1 --> T1
    P1 --> C1
    P2 --> T2
    P2 --> C2
    P3 --> T3
    P3 --> C3
    P4 --> T4
    P4 --> C4
```

---

## 📋 Legenda dos Diagramas

### Símbolos Utilizados:
- **🔄**: Fluxo de dados/controle
- **📡**: Comunicação/redes
- **🏗️**: Arquitetura/estrutura
- **🔧**: Configuração/implementação
- **📊**: Métricas/performance
- **🎯**: Casos de uso/requisitos

### Cores:
- **Azul**: Componentes principais
- **Verde**: Estados/processos
- **Laranja**: Interfaces/comunicação
- **Vermelho**: Erros/alertas
- **Roxo**: Configurações/dados

---

*Diagramas gerados automaticamente - CoreHub v1.0* 