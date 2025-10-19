# Simulador do Dispositivo SmartDoor para o Projeto CoreHub

Este repositório contém o script Python para simular o comportamento de um dispositivo **SmartDoor**. O objetivo principal é fornecer um ambiente de teste robusto para o desenvolvimento do **CoreHub**, a unidade central que irá interagir com este dispositivo.

O simulador comunica-se via protocolo MQTT, publicando dados de sensores (porta e luz) e respondendo a comandos (buzzer), replicando o comportamento do dispositivo de hardware real descrito na documentação do projeto.

---

## 🚀 Visão Geral

Para desenvolver o CoreHub de forma eficaz, é crucial simular os dados que seriam gerados pelo SmartDoor. Este script simula:

* **Estado da Porta**: Publica se a porta está `"OPEN"` ou `"CLOSED"`.
* **Estado da Iluminação**: Publica se a luz está `"ON"` ou `"OFF"`.
* **Atuador (Buzzer)**: Ouve um tópico MQTT e simula a ativação de um buzzer ao receber o comando `"ON"`.

## ✨ Funcionalidades

* **Simulação Realista**: Opera com intervalos de tempo que mimetizam o uso real de uma porta e de um interruptor de luz num ambiente como um escritório ou laboratório.
* **Comunicação MQTT**: Totalmente integrado com um broker MQTT para publicar e subscrever tópicos.
* **Ambiente Configurável**: O script pode ser iniciado para operar num "ambiente" específico (ex: `lab1`, `salaaula`), permitindo testar o CoreHub em diferentes cenários.
* **Fácil de Usar**: Um único script pronto para ser executado com dependências mínimas.

---

## 🔧 Pré-requisitos

Antes de começar, garanta que tem os seguintes softwares instalados:

* **Python**: A versão exata utilizada no desenvolvimento é a **3.9.13**. É altamente recomendável usar um gerenciador de versões como o `pyenv` para garantir a compatibilidade.
* **pip**: O gerenciador de pacotes do Python.

## ⚙️ Instalação e Configuração

Siga os passos abaixo para preparar o seu ambiente de desenvolvimento:

1.  **Clone o repositório (exemplo):**
    ```bash
    git clone [https://github.com/seu-usuario/smartdoor-simulator.git](https://github.com/seu-usuario/smartdoor-simulator.git)
    cd smartdoor-simulator
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

O simulador é executado a partir da linha de comando. O argumento `--ambiente` é obrigatório para definir em qual local o dispositivo simulado está a operar.

**Exemplo de execução para o ambiente `lab1`:**
```bash
python smartdoor_simulator.py --ambiente lab1
```

O terminal exibirá os logs de conexão, as publicações de mudança de estado e as mensagens recebidas no tópico do buzzer.

## 📡 Configuração do Broker MQTT

O simulador está pré-configurado para se conectar ao seguinte broker:

* **Endereço (IP)**: `131.255.82.115`
* **Porta**: `1883`

Certifique-se de que a sua máquina tem acesso a este endereço para que o simulador funcione corretamente.

## 📂 Estrutura do Projeto

```
.
├── .python-version         # Define a versão do Python para o pyenv
├── requirements.txt        # Lista de dependências Python do projeto
├── README.md               # Este ficheiro de documentação
└── smartdoor_simulator.py  # Script de simulação para o SmartDoor
```
