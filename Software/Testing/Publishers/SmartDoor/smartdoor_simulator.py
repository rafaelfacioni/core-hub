import paho.mqtt.client as mqtt
import time
import random
import argparse

# --- Configurações do Broker MQTT ---
BROKER_IP = "131.255.82.115"
BROKER_PORT = 1883
CLIENT_ID_PREFIX = "smartdoor_sim_real"

# --- Intervalos de tempo realistas (em segundos) ---
# A porta pode mudar de estado a cada 2 a 10 minutos
DOOR_INTERVAL_MIN = 120  # 2 minutos
DOOR_INTERVAL_MAX = 600  # 10 minutos

# A luz muda de estado com muito menos frequência, a cada 30 a 90 minutos
LIGHT_INTERVAL_MIN = 1800  # 30 minutos
LIGHT_INTERVAL_MAX = 5400  # 90 minutos


# --- Funções de Callback do MQTT (sem alterações) ---


def on_connect(client, userdata, flags, rc):
    """Callback executado quando o cliente se conecta ao broker."""
    if rc == 0:
        print(f"Cliente '{userdata['client_id']}' conectado com sucesso ao broker.")
        buzzer_topic = userdata["topics"]["buzzer"]
        client.subscribe(buzzer_topic)
        print(f"Inscrito no tópico: {buzzer_topic}")
    else:
        print(f"Falha na conexão. Código de retorno: {rc}")


def on_message(client, userdata, msg):
    """Callback executado quando uma mensagem é recebida de um tópico assinado."""
    payload = msg.payload.decode()
    print(f"\nMensagem recebida no tópico '{msg.topic}': {payload}")

    if payload.upper() == "ON":
        print(f"[AÇÃO] Buzzer ativado no ambiente '{userdata['ambiente']}'!")
    elif payload.upper() == "OFF":
        print(f"[AÇÃO] Buzzer desativado no ambiente '{userdata['ambiente']}'!")


def on_publish(client, userdata, mid):
    """Callback executado após uma publicação ser enviada."""
    pass


# --- Função Principal de Simulação Aprimorada ---


def simulate_smartdoor(ambiente):
    """Inicia o cliente MQTT e o loop de simulação com temporizadores independentes."""

    topics = {
        "light": f"test/hana/{ambiente}/smartdoor/light",
        "door": f"test/hana/{ambiente}/smartdoor/door",
        "buzzer": f"test/hana/{ambiente}/smartdoor/buzzer",
    }

    client_id = f"{CLIENT_ID_PREFIX}-{ambiente}-{random.randint(0, 1000)}"
    userdata = {"topics": topics, "ambiente": ambiente, "client_id": client_id}

    client = mqtt.Client(client_id=client_id, userdata=userdata)
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_publish = on_publish

    try:
        client.connect(BROKER_IP, BROKER_PORT, 60)
    except Exception as e:
        print(
            f"Não foi possível conectar ao broker em {BROKER_IP}:{BROKER_PORT}. Erro: {e}"
        )
        return

    client.loop_start()

    print("\n--- Iniciando Simulação Realista do SmartDoor ---")
    print(f"Ambiente: {ambiente}")
    print(f"Intervalo da porta: {DOOR_INTERVAL_MIN}-{DOOR_INTERVAL_MAX}s")
    print(f"Intervalo da luz: {LIGHT_INTERVAL_MIN}-{LIGHT_INTERVAL_MAX}s")
    print("Pressione CTRL+C para parar.\n")

    # Variáveis para controlar os temporizadores e estados
    last_door_state = "CLOSED"
    last_light_state = "OFF"

    # Publica os estados iniciais para garantir que o broker os tenha
    client.publish(topics["door"], last_door_state, retain=True)
    client.publish(topics["light"], last_light_state, retain=True)
    print(
        f"Estados iniciais publicados: Porta='{last_door_state}', Luz='{last_light_state}'"
    )

    # Agenda a primeira mudança
    next_door_update_time = time.time() + random.randint(
        DOOR_INTERVAL_MIN, DOOR_INTERVAL_MAX
    )
    next_light_update_time = time.time() + random.randint(
        LIGHT_INTERVAL_MIN, LIGHT_INTERVAL_MAX
    )

    try:
        while True:
            current_time = time.time()

            # --- Lógica de Simulação da Porta ---
            if current_time >= next_door_update_time:
                # Inverte o estado da porta
                new_door_state = "OPEN" if last_door_state == "CLOSED" else "CLOSED"
                client.publish(topics["door"], new_door_state, retain=True)
                print(f"\n[EVENTO] Mudança de estado da porta -> '{new_door_state}'")
                last_door_state = new_door_state

                # Agenda a próxima mudança da porta
                next_door_update_time = time.time() + random.randint(
                    DOOR_INTERVAL_MIN, DOOR_INTERVAL_MAX
                )
                print(
                    f"Próxima verificação da porta em {int(next_door_update_time - current_time)} segundos."
                )

            # --- Lógica de Simulação da Luz ---
            if current_time >= next_light_update_time:
                # Inverte o estado da luz
                new_light_state = "ON" if last_light_state == "OFF" else "OFF"
                client.publish(topics["light"], new_light_state, retain=True)
                print(f"\n[EVENTO] Mudança de estado da luz -> '{new_light_state}'")
                last_light_state = new_light_state

                # Agenda a próxima mudança da luz
                next_light_update_time = time.time() + random.randint(
                    LIGHT_INTERVAL_MIN, LIGHT_INTERVAL_MAX
                )
                print(
                    f"Próxima verificação da luz em {int(next_light_update_time - current_time)} segundos."
                )

            time.sleep(
                1
            )  # Loop principal roda a cada segundo para não sobrecarregar a CPU

    except KeyboardInterrupt:
        print("\n--- Encerrando Simulação ---")
    finally:
        client.loop_stop()
        client.disconnect()
        print("Cliente desconectado.")


# --- Execução do Script ---

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Simulador de dispositivo SmartDoor com intervalos realistas."
    )
    parser.add_argument(
        "--ambiente",
        type=str,
        required=True,
        help="Nome do ambiente a ser monitorado (ex: lab1, salaaula).",
    )

    args = parser.parse_args()
    simulate_smartdoor(args.ambiente.lower())
