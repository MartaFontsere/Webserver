import socket
import threading
import time

HOST = "127.0.0.1"   # dirección del servidor
PORT = 8080          # cambia este valor si tu servidor usa otro puerto
NUM_CLIENTS = 20     # número de clientes simultáneos

def client(id):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
        msg = f"Hola desde cliente {id}\n"
        s.sendall(msg.encode())
        time.sleep(0.1)  # pequeño retardo para simular actividad
        data = s.recv(1024)
        print(f"[Cliente {id}] Recibido: {data.decode().strip()}")
    except Exception as e:
        print(f"[Cliente {id}] Error: {e}")
    finally:
        s.close()

threads = [threading.Thread(target=client, args=(i,)) for i in range(NUM_CLIENTS)]

print(f"🔹 Lanzando {NUM_CLIENTS} clientes concurrentes...\n")
for t in threads:
    t.start()

for t in threads:
    t.join()

print("\n✅ Prueba terminada.")
