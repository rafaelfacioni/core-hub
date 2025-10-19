# CoreHub – Central de Decisão e Automação

## 🎯 Objetivo

Desenvolver um dispositivo embarcado capaz de atuar como uma central de automação e controle, responsável por processar dados provenientes de sensores publicados por outros dispositivos via MQTT, tomar decisões com base em regras definidas e publicar comandos para atuadores (como ar-condicionado e buzzer). O projeto utiliza o microcontrolador iMCP HTNB32L e não exige desenvolvimento de PCB personalizada.

## 🧰 Componentes Utilizados

- **Microcontrolador:** iMCP HTNB32L  
- **Conectividade:** NB-IoT (modem interno)  
- **Broker MQTT:**  
  - IP: `131.255.82.115`  
  - Porta: `1883`  
- **PCB personalizada:** não se aplica a este projeto

## ⚙️ Requisitos Funcionais

### Subscrição de dados via MQTT

- Escutar dados publicados pelos projetos:
  - Estado da porta (OPEN / CLOSED) – SmartDoor
  - Temperatura e umidade – SenseClima
  - Estado do ar-condicionado – AirControl

### Lógica de decisão automatizada

- **Controle do buzzer:**
  - Se a porta permanecer aberta por mais de um tempo definido (ex: 60 segundos), publicar `"ON"` no tópico do buzzer.
  - Caso a porta seja fechada antes desse tempo, não acionar o buzzer.
- **Controle do ar-condicionado:**
  - Se a temperatura lida for superior a um limite definido (ex: 28 °C), publicar `"ON"` para ligar o ar-condicionado.
  - Se a temperatura cair abaixo de um limite mínimo (ex: 24 °C), publicar `"OFF"` para desligar o ar-condicionado.
  - Além de ligar e desligar, deverá ser definida e implementada uma lógica de controle para variar a temperatura do ar-condicionado com base na temperatura ambiente desejada.

### Publicação de comandos via MQTT

- Publicar comandos de controle para buzzer (SmartDoor) e ar-condicionado (AirControl) via tópicos padronizados.

### Modo de operação

- Operar continuamente como central de decisão.
- Implementar reconexão automática ao broker MQTT em caso de falha.

## 🛰️ Tópicos MQTT Padronizados

> **IMPORTANTE**: Substituir `<ambiente>`, `<board>` e `<equipamento>` pelos nomes apropriados, todos em letras minúsculas e sem espaços.

| Finalidade        | Tópico MQTT                                               | Direção     | Tipo de dado          |
|-------------------|------------------------------------------------------------|-------------|------------------------|
| Iluminação        | `hana/<ambiente>/smartdoor/light`                         | Assinatura  | `"ON"` / `"OFF"`       |
| Porta             | `hana/<ambiente>/smartdoor/door`                          | Assinatura  | `"OPEN"` / `"CLOSED"`  |
| Buzzer            | `hana/<ambiente>/smartdoor/buzzer`                        | Publicação  | `"ON"` / `"OFF"`       |
| Temp. Amb.        | `hana/<ambiente>/senseclima/<board>/temperature`          | Assinatura  | Ex: `"27.8"`           |
| Umidade           | `hana/<ambiente>/senseclima/<board>/humidity`             | Assinatura  | Ex: `"64.2"`           |
| Estado Ar         | `hana/<ambiente>/aircontrol/<equipamento>/power`          | Publicação  | `"ON"` / `"OFF"`       |
| Temp. Ar          | `hana/<ambiente>/aircontrol/<equipamento>/temperature`    | Publicação  | `"22"`, `"24"`, `"26"` |

## 🖨️ Desenvolvimento da PCB

- **Não aplicável.** Este projeto não exige o desenvolvimento de uma placa personalizada.

## 🔍 Observações Técnicas

- O tempo de porta aberta deve ser monitorado com timers no firmware.
- A lógica de controle deve considerar atualizações periódicas dos dados.
- O sistema deve funcionar corretamente mesmo com desconexões temporárias da rede.
- É obrigatória a reconexão automática ao broker MQTT.

## 📋 Critérios de Avaliação

- Funcionamento correto da lógica de decisão baseada nos dados recebidos via MQTT.
- Publicação funcional de comandos para buzzer e ar-condicionado.
- Subscrição correta e eficiente aos tópicos MQTT.
- Documentação completa, na wiki do github, com evolução do projeto e dificuldades encontradas durante o desenvolvimento.  
  - Exemplo de documentação: [Hands-On Linux Wiki](https://github.com/rafaelfacioni/Hands-On-Linux/wiki)  
- Apresentação prática do projeto final..
- *(Opcional)* Adicione na Wiki do repositório um registro pessoal com os principais aprendizados adquiridos ao longo do curso.

---

> Este projeto faz parte do Módulo 4 do Curso de Capacitação em Sistemas Embarcados com o iMCP HTNB32L.
