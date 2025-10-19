import paho.mqtt.client as mqtt
import time
import random
import argparse
import sys

# --- Configurações do Broker MQTT ---
BROKER_IP = "131.255.82.115"
BROKER_PORT = 1883
CLIENT_ID_PREFIX = "aircontrol_sim"

# Dicionário para armazenar o estado de cada equipamento de AC simulado
# Estrutura: { 'id_equipamento': {'power': 'OFF', 'temp': 24} }
ac_units_state = {}

# --- Funções de Callback do MQTT ---


def on_connect(client, userdata, flags, rc):
    """Callback executado quando o cliente se conecta ao broker."""
    if rc == 0:
        print(f"Cliente '{userdata['client_id']}' conectado com sucesso ao broker.")
        # Para cada equipamento, inscreve-se nos seus tópicos de controle
        for ac_id in userdata["equipamentos"]:
            power_topic = f"test/hana/{userdata['ambiente']}/aircontrol/{ac_id}/power"
            temp_topic = (
                f"test/hana/{userdata['ambiente']}/aircontrol/{ac_id}/temperature"
            )
            client.subscribe(power_topic)
            client.subscribe(temp_topic)
            print(f"Inscrito nos tópicos para o equipamento '{ac_id}':")
            print(f"  - {power_topic}")
            print(f"  - {temp_topic}")
    else:
        print(f"Falha na conexão. Código de retorno: {rc}")
        sys.exit(1)  # Encerra o script se não conseguir conectar


def on_message(client, userdata, msg):
    """Callback executado quando uma mensagem é recebida de um tópico assinado."""
    try:
        payload = msg.payload.decode().upper()
        topic_parts = msg.topic.split("/")
        ac_id = topic_parts[-2]  # O penúltimo elemento é o ID do equipamento
        command_type = topic_parts[
            -1
        ]  # O último é o tipo de comando (power/temperature)

        if ac_id not in ac_units_state:
            return  # Ignora mensagens para equipamentos não gerenciados

        print(
            f"\n[COMANDO RECEBIDO] Equipamento: '{ac_id}' | Tópico: '{msg.topic}' | Payload: '{payload}'"
        )

        if command_type == "power":
            if payload in ["ON", "OFF"]:
                ac_units_state[ac_id]["power"] = payload
                print(
                    f"  -> [AÇÃO] Estado de energia do '{ac_id}' alterado para {payload}."
                )
            else:
                print(
                    f"  -> [AVISO] Payload de energia inválido: '{payload}'. Esperado 'ON' ou 'OFF'."
                )

        elif command_type == "temperature":
            try:
                temp_value = int(payload)
                ac_units_state[ac_id]["temp"] = temp_value
                print(
                    f"  -> [AÇÃO] Temperatura do '{ac_id}' ajustada para {temp_value}°C."
                )
            except ValueError:
                print(
                    f"  -> [AVISO] Payload de temperatura inválido: '{payload}'. Esperado um número inteiro."
                )

        # Mostra o estado atual do equipamento
        current_state = ac_units_state[ac_id]
        print(
            f"  -> [ESTADO ATUAL de '{ac_id}'] Energia: {current_state['power']} | Temperatura: {current_state['temp']}°C"
        )

    except (ValueError, IndexError) as e:
        print(f"Erro ao processar mensagem no tópico '{msg.topic}': {e}")


def main(ambiente, equipamentos):
    """Função principal que configura o cliente e inicia o loop de escuta."""

    client_id = f"{CLIENT_ID_PREFIX}-{ambiente}-{random.randint(0, 1000)}"
    userdata = {
        "ambiente": ambiente,
        "equipamentos": equipamentos,
        "client_id": client_id,
    }

    # Inicializa o estado para cada equipamento
    for ac_id in equipamentos:
        ac_units_state[ac_id] = {"power": "OFF", "temp": 24}  # Estado inicial padrão

    # Inicializa o cliente MQTT
    client = mqtt.Client(client_id=client_id, userdata=userdata)
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(BROKER_IP, BROKER_PORT, 60)
    except Exception as e:
        print(
            f"Não foi possível conectar ao broker em {BROKER_IP}:{BROKER_PORT}. Erro: {e}"
        )
        return

    print("\n--- Iniciando Simulação do AirControl ---")
    print(f"Ambiente: {ambiente}")
    print(f"Equipamentos simulados: {', '.join(equipamentos)}")
    print("Aguardando comandos MQTT. Pressione CTRL+C para parar.\n")

    # client.loop_forever() é um loop bloqueante que processa o tráfego de rede
    # e despacha os callbacks automaticamente. Ideal para scripts que só escutam.
    try:
        client.loop_forever()
    except KeyboardInterrupt:
        print("\n--- Encerrando Simulação ---")
    finally:
        client.disconnect()
        print("Cliente desconectado.")


# --- Execução do Script ---
if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Simulador de dispositivo(s) AirControl."
    )
    parser.add_argument(
        "--ambiente",
        type=str,
        required=True,
        help="Nome do ambiente a ser monitorado (ex: lab1, salaaula).",
    )
    parser.add_argument(
        "--equipamentos",
        nargs="+",
        required=True,
        help="Um ou mais IDs para os equipamentos de AC a serem simulados (ex: ac1 ac_sala).",
    )

    args = parser.parse_args()

    main(args.ambiente.lower(), [e.lower() for e in args.equipamentos])
