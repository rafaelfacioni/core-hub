# Simulador de Dispositivos AirControl para o Projeto CoreHub

Este repositório contém o script Python para simular o comportamento de um ou mais dispositivos **AirControl**. O objetivo principal é fornecer um "atuador" virtual para ser comandado pelo **CoreHub**, permitindo testar a lógica de controle de climatização do projeto.

O simulador é puramente reativo: ele se conecta a um broker MQTT e aguarda por comandos para ligar/desligar e ajustar a temperatura de equipamentos de ar-condicionado virtuais.

---

## 🚀 Visão Geral

Este script simula o comportamento de um controlador de ar-condicionado que recebe ordens via MQTT. Suas principais funções são:

* **Controle de Energia**: Ouve um tópico para receber comandos `"ON"` ou `"OFF"`.
* **Controle de Temperatura**: Ouve um tópico para receber um valor numérico de temperatura (ex: `"22"`).
* **Feedback Visual**: Imprime no console o estado atual de cada equipamento simulado após receber um comando.

## ✨ Funcionalidades

* **Simulação de Múltiplos Equipamentos**: Um único script pode simular vários controladores de ar-condicionado, cada um com seu próprio identificador e estado.
* **Comunicação MQTT**: Totalmente integrado com um broker MQTT para subscrever tópicos de comando.
* **Ambiente Configurável**: O script pode ser iniciado para operar num "ambiente" específico (ex: `lab1`, `salaaula`) e com identificadores de equipamentos definidos pelo usuário.
* **Reativo e Leve**: O simulador apenas escuta mensagens, utilizando o loop `loop_forever()` do Paho-MQTT para eficiência.

---

## 🔧 Pré-requisitos

Antes de começar, garanta que tem os seguintes softwares instalados:

* **Python**: A versão exata utilizada no desenvolvimento é a **3.9.13**. É altamente recomendável usar um gerenciador de versões como o `pyenv` para garantir a compatibilidade.
* **pip**: O gerenciador de pacotes do Python.

## ⚙️ Instalação e Configuração

Siga os passos abaixo para preparar o seu ambiente de desenvolvimento:

1.  **Clone o repositório (exemplo):**
    ```bash
    git clone [https://github.com/seu-usuario/aircontrol-simulator.git](https://github.com/seu-usuario/aircontrol-simulator.git)
    cd aircontrol-simulator
    ```

2.  **Configure a versão do Python (opcional, recomendado):**
    Se utiliza o `pyenv`, ele lerá automaticamente o ficheiro `.python-version` e configurará a versão correta do interpretador para este projeto.
    ```bash
    pyenv install 3.9.13 # Caso ainda não a tenha instalado
    pyenv local 3.9.13
    ```

3.  **Instale as dependências:**
    A biblioteca necessária está listada no ficheiro `requirements.txt`. Para a instalar, execute:
    ```bash
    pip install -r requirements.txt
    ```

## ▶️ Como Usar

O simulador é executado a partir da linha de comando. Os argumentos `--ambiente` e `--equipamentos` são obrigatórios.

**Exemplo de execução para simular duas unidades de AC (`ac_principal` e `ac_janela`) no ambiente `escritorio`:**
```bash
python aircontrol_simulator.py --ambiente escritorio --equipamentos ac_principal ac_janela
```

O terminal exibirá os logs de conexão e ficará aguardando por mensagens. Para testar, use um cliente MQTT (como o MQTT Explorer) para publicar mensagens nos tópicos correspondentes:

* **Ligar o AC principal:**
    * Tópico: `hana/escritorio/aircontrol/ac_principal/power`
    * Mensagem: `ON`
* **Ajustar temperatura do AC da janela para 22°C:**
    * Tópico: `hana/escritorio/aircontrol/ac_janela/temperature`
    * Mensagem: `22`

## 📡 Configuração do Broker MQTT

O simulador está pré-configurado para se conectar ao seguinte broker:

* **Endereço (IP)**: `131.255.82.115`
* **Porta**: `1883`

Certifique-se de que a sua máquina tem acesso a este endereço para que o simulador funcione corretamente.

## 📂 Estrutura do Projeto

```
.
├── .python-version           # Define a versão do Python para o pyenv
├── requirements.txt          # Lista de dependências Python do projeto
├── README.md                 # Este ficheiro de documentação
└── aircontrol_simulator.py   # Script de simulação para o AirControl
```