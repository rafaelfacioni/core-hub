import paho.mqtt.client as mqtt
import time
import random
import argparse
import threading
from datetime import datetime, timedelta

# --- Configurações do Broker MQTT ---
BROKER_IP = "131.255.82.115"
BROKER_PORT = 1883
CLIENT_ID_PREFIX = "senseclima_sim_dyn"
DEFAULT_INTERVAL = 10  # Intervalo de publicação em segundos (tempo real)

# --- Configurações da Simulação de Tempo ---
# Fator de aceleração: 3600x significa que 1 segundo real = 1 hora simulada.
# Um dia de 24h será simulado em 24 segundos.
SIMULATION_SPEED_FACTOR = 3600
# Inicia a simulação às 07:00 da manhã.
simulated_start_time = datetime.now().replace(hour=7, minute=0, second=0, microsecond=0)
real_start_time = time.time()

# Dicionário para armazenar o estado de cada placa simulada
boards_state = {}

# --- Funções de Cálculo de Curva Diária ---


def get_dynamic_readings(current_hour):
    """
    Calcula a temperatura e umidade com base na hora do dia simulada,
    usando interpolação linear entre os pontos-chave.
    """
    # Pontos-chave da curva de temperatura
    # (Hora, Temperatura em °C)
    temp_points = [(0, 14), (8, 18), (12, 30), (18, 22), (24, 14)]

    # Pontos-chave da umidade (inversamente proporcional à temperatura)
    # (Temperatura, Umidade em %)
    humidity_points = [
        (14, 85),
        (30, 40),  # Mínima temp -> Máxima umidade | Máxima temp -> Mínima umidade
    ]

    # Encontra os dois pontos de temperatura para fazer a interpolação
    start_point = temp_points[0]
    end_point = temp_points[1]
    for i in range(len(temp_points) - 1):
        if temp_points[i][0] <= current_hour < temp_points[i + 1][0]:
            start_point = temp_points[i]
            end_point = temp_points[i + 1]
            break

    # Interpolação Linear da Temperatura
    start_hour, start_temp = start_point
    end_hour, end_temp = end_point
    hour_range = end_hour - start_hour
    temp_range = end_temp - start_temp

    progress = (current_hour - start_hour) / hour_range
    temperature = start_temp + (temp_range * progress)

    # Interpolação Linear da Umidade
    min_temp, max_humidity = humidity_points[0]
    max_temp, min_humidity = humidity_points[1]

    temp_total_range = max_temp - min_temp
    humidity_total_range = max_humidity - min_humidity

    temp_progress = (temperature - min_temp) / temp_total_range
    # A umidade diminui conforme a temperatura aumenta
    humidity = max_humidity - (humidity_total_range * temp_progress)

    # Adiciona uma pequena variação aleatória para mais realismo
    temperature += random.uniform(-0.3, 0.3)
    humidity += random.uniform(-2.0, 2.0)

    return round(temperature, 1), round(humidity, 1)


# --- Funções de Callback do MQTT (sem alterações) ---


def on_connect(client, userdata, flags, rc):
    """Callback executado quando o cliente se conecta ao broker."""
    if rc == 0:
        print(f"Cliente '{userdata['client_id']}' conectado com sucesso ao broker.")
        for board_id in userdata["boards"]:
            interval_topic = (
                f"hana/{userdata['ambiente']}/senseclima/{board_id}/interval"
            )
            client.subscribe(interval_topic)
            print(f"Inscrito no tópico: {interval_topic}")
    else:
        print(f"Falha na conexão. Código de retorno: {rc}")


def on_message(client, userdata, msg):
    """Callback executado quando uma mensagem é recebida de um tópico assinado."""
    try:
        payload = msg.payload.decode()
        topic_parts = msg.topic.split("/")
        board_id = topic_parts[-2]
        print(
            f"\nMensagem recebida para a placa '{board_id}': alterando intervalo para {payload}s"
        )
        if board_id in boards_state:
            boards_state[board_id]["interval"] = int(payload)
            print(
                f"Intervalo da placa '{board_id}' atualizado para {boards_state[board_id]['interval']} segundos."
            )
    except (ValueError, IndexError) as e:
        print(f"Erro ao processar mensagem no tópico '{msg.topic}': {e}")


# --- Função de Simulação ---


def simulate_board(board_id, ambiente, client):
    """Função executada em uma thread para simular uma única placa SenseClima."""
    temp_topic = f"hana/{ambiente}/senseclima/{board_id}/temperature"
    humidity_topic = f"hana/{ambiente}/senseclima/{board_id}/humidity"

    while True:
        # Pega o tempo simulado atual
        elapsed_real_seconds = time.time() - real_start_time
        elapsed_simulated_seconds = elapsed_real_seconds * SIMULATION_SPEED_FACTOR
        current_simulated_time = simulated_start_time + timedelta(
            seconds=elapsed_simulated_seconds
        )

        # Converte a hora para um float (ex: 13.5 para 13:30)
        current_hour_float = current_simulated_time.hour + (
            current_simulated_time.minute / 60.0
        )

        # Obtém os valores da curva diária
        temp, humidity = get_dynamic_readings(current_hour_float)

        # Verifica se é hora de publicar
        current_time = time.time()
        if current_time >= boards_state[board_id]["next_update"]:
            client.publish(temp_topic, str(temp), retain=True)
            client.publish(humidity_topic, str(humidity), retain=True)

            # Imprime o estado atual
            sim_time_str = current_simulated_time.strftime("%H:%M:%S")
            print(
                f"[{board_id} | {sim_time_str}] Publicado: Temp={temp}°C, Umidade={humidity}%"
            )

            # Agenda a próxima atualização com base no tempo real
            interval = boards_state[board_id]["interval"]
            boards_state[board_id]["next_update"] = current_time + interval

        time.sleep(1)


def main(ambiente, boards):
    """Função principal que configura o cliente e inicia as threads de simulação."""
    client_id = f"{CLIENT_ID_PREFIX}-{ambiente}-{random.randint(0, 1000)}"
    userdata = {"ambiente": ambiente, "boards": boards, "client_id": client_id}

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

    client.loop_start()

    print("\n--- Iniciando Simulação Dinâmica do SenseClima ---")
    print(f"Ambiente: {ambiente}")
    print(f"Placas simuladas: {', '.join(boards)}")
    print(f"Velocidade da simulação: {SIMULATION_SPEED_FACTOR}x")
    print("Pressione CTRL+C para parar.\n")

    threads = []
    for board_id in boards:
        boards_state[board_id] = {
            "interval": DEFAULT_INTERVAL,
            "next_update": time.time(),
        }
        thread = threading.Thread(
            target=simulate_board, args=(board_id, ambiente, client), daemon=True
        )
        threads.append(thread)
        thread.start()

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n--- Encerrando Simulação ---")
    finally:
        client.loop_stop()
        client.disconnect()
        print("Cliente desconectado.")


# --- Execução do Script ---
if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Simulador de dispositivo(s) SenseClima com curva diária de temperatura."
    )
    parser.add_argument(
        "--ambiente",
        type=str,
        required=True,
        help="Nome do ambiente a ser monitorado (ex: lab1, salaaula).",
    )
    parser.add_argument(
        "--boards",
        nargs="+",
        required=True,
        help="Um ou mais IDs para as placas a serem simuladas (ex: sensor01 sensor02).",
    )

    args = parser.parse_args()

    main(args.ambiente.lower(), [b.lower() for b in args.boards])
