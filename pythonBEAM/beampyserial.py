import socket, struct
import serial  # Adicionado para comunicação serial

# Porta do OutGauge definida nas opções do jogo
PORT = 4444

# Formato básico do pacote OutGauge (parcial, suficiente pra pegar marcha)
# ref: gear é um char onde: 0=R, 1=N, 2=1ª, 3=2ª, ...
# Vamos ler só os primeiros campos relevantes.
FMT = "<I4sHbbf"  # time, car[4], flags, gear(char), plid(char), speed(float)
SIZE = struct.calcsize(FMT)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("127.0.0.1", PORT))
print(f"Ouvindo OutGauge em 127.0.0.1:{PORT} ... Ctrl+C para sair.")

def gear_to_str(g):
    mapping = {0:"R", 1:"N", 2:"1", 3:"2", 4:"3", 5:"4", 6:"5", 7:"6", 8:"7", 9:"8"}
    return mapping.get(g, str(g))

# Configuração da porta serial para o Pico (ajuste o caminho se necessário)
SERIAL_PORT = '/dev/ttyACM0'  # Exemplo para Linux; verifique com ls /dev/tty*
BAUD_RATE = 115200  # Taxa de transmissão comum para Pico

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Conectado ao Pico via {SERIAL_PORT}")
except serial.SerialException as e:
    print(f"Erro ao conectar à porta serial: {e}")
    exit(1)

try:
    while True:
        data, _ = sock.recvfrom(512)
        if len(data) >= SIZE:
            _, _, _, gear_char, _, _ = struct.unpack(FMT, data[:SIZE])
            gear_str = gear_to_str(gear_char)
            # Envia a marcha via serial para o Pico
            ser.write(f"{gear_str}\n".encode())
            print(f"Enviado para Pico: {gear_str}")  # Opcional: mantém log local
except KeyboardInterrupt:
    pass
finally:
    sock.close()
    ser.close()