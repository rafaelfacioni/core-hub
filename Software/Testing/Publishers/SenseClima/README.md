# Simulador de Dispositivos SenseClima para o Projeto CoreHub

Este repositório contém o script Python para simular o comportamento de um ou mais dispositivos **SenseClima**. O objetivo principal é fornecer um ambiente de teste robusto para o desenvolvimento do **CoreHub**, a unidade central que irá consumir os dados de temperatura e umidade.

O simulador comunica-se via protocolo MQTT, publicando dados de sensores (temperatura e umidade) e permitindo que seu intervalo de publicação seja configurado remotamente, replicando o comportamento dos dispositivos de hardware reais.

---

## 🚀 Visão Geral

Para desenvolver o CoreHub de forma eficaz, é crucial simular os dados que seriam gerados pelos sensores de ambiente. Este script simula:

* **Temperatura**: Publica um valor de temperatura (ex: `"25.5"`) em seu tópico MQTT.
* **Umidade**: Publica um valor de umidade relativa (ex: `"58.2"`) em seu tópico MQTT.
* **Intervalo Configurável**: Ouve um tópico MQTT para receber um novo intervalo (em segundos) e ajustar sua frequência de publicação.

## ✨ Funcionalidades

* **Simulação de Múltiplos Dispositivos**: Um único script pode simular vários sensores SenseClima, cada um com seu próprio identificador e estado.
* **Dados Realistas**: Gera valores de temperatura e umidade dentro de uma faixa plausível.
* **Comunicação MQTT**: Totalmente integrado com um broker MQTT para publicar dados e subscrever comandos de configuração.
* **Ambiente Configurável**: O script pode ser iniciado para operar num "ambiente" específico (ex: `lab1`, `salaaula`) e com identificadores de placas definidos pelo usuário.

---

## 🔧 Pré-requisitos

Antes de começar, garanta que tem os seguintes softwares instalados:

* **Python**: A versão exata utilizada no desenvolvimento é a **3.9.13**. É altamente recomendável usar um gerenciador de versões como o `pyenv` para garantir a compatibilidade.
* **pip**: O gerenciador de pacotes do Python.

## ⚙️ Instalação e Configuração

Siga os passos abaixo para preparar o seu ambiente de desenvolvimento:

1.  **Clone o repositório (exemplo):**
    ```bash
    git clone [https://github.com/seu-usuario/senseclima-simulator.git](https://github.com/seu-usuario/senseclima-simulator.git)
    cd senseclima-simulator
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

O simulador é executado a partir da linha de comando. Os argumentos `--ambiente` e `--boards` são obrigatórios.

**Exemplo de execução para simular duas placas (`sensor01` e `sensor02`) no ambiente `lab1`:**
```bash
python senseclima_simulator.py --ambiente lab1 --boards sensor01 sensor02
```

O terminal exibirá os logs de conexão e os dados publicados por cada placa simulada.

Para alterar o intervalo de uma placa, você pode usar um cliente MQTT (como o MQTT Explorer) para publicar um novo valor no tópico correspondente. Por exemplo, para que a placa `sensor01` publique a cada 10 segundos, envie a mensagem `"10"` para o tópico:
`hana/lab1/senseclima/sensor01/interval`

## 📡 Configuração do Broker MQTT

O simulador está pré-configurado para se conectar ao seguinte broker:

* **Endereço (IP)**: `131.255.82.115`
* **Porta**: `1883`

Certifique-se de que a sua máquina tem acesso a este endereço para que o simulador funcione corretamente.

## 📂 Estrutura do Projeto

```
.
├── .python-version          # Define a versão do Python para o pyenv
├── requirements.txt         # Lista de dependências Python do projeto
├── README.md                # Este ficheiro de documentação
└── senseclima_simulator.py  # Script de simulação para o SenseClima
```