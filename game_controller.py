#!/usr/bin/env python3
"""
Game Controller - Control de juegos STM32 por USART
====================================================

Este script permite controlar los juegos en el STM32 a través del puerto serial.

Comandos disponibles:
  G, 1  - Cambiar a Galaga
  2     - Cambiar a Juego 2 (futuro)
  0     - Detener juego
  S     - Ver estado actual
  R     - Reiniciar juego
  A     - Modo auto-play
  M     - Modo manual (control por sensores)

Requisitos:
  pip install pyserial

Uso:
  python game_controller.py             # Auto-detecta puerto
  python game_controller.py COM3        # Windows
  python game_controller.py /dev/ttyACM0 # Linux
"""

import serial
import serial.tools.list_ports
import sys
import time
import threading

class GameController:
    def __init__(self, port=None, baudrate=115200):
        self.serial = None
        self.baudrate = baudrate
        self.running = False
        
        if port:
            self.connect(port)
        else:
            self.auto_connect()
    
    def auto_connect(self):
        """Intenta conectar automáticamente a un puerto STM32"""
        print("🔍 Buscando dispositivos STM32...")
        ports = serial.tools.list_ports.comports()
        
        for port in ports:
            # Buscar puertos típicos de STM32
            if "STM" in port.description or "ST" in port.manufacturer or \
               "ACM" in port.device or "USB" in port.description:
                print(f"   Encontrado: {port.device} - {port.description}")
                if self.connect(port.device):
                    return True
        
        # Si no se encontró, mostrar todos los puertos disponibles
        print("\n📋 Puertos disponibles:")
        for i, port in enumerate(ports):
            print(f"   {i}: {port.device} - {port.description}")
        
        if ports:
            try:
                idx = int(input("\nSelecciona el número del puerto: "))
                return self.connect(ports[idx].device)
            except (ValueError, IndexError):
                print("❌ Selección inválida")
                return False
        else:
            print("❌ No se encontraron puertos seriales")
            return False
    
    def connect(self, port):
        """Conectar al puerto especificado"""
        try:
            self.serial = serial.Serial(port, self.baudrate, timeout=1)
            time.sleep(2)  # Esperar a que el STM32 se inicialice
            print(f"✅ Conectado a {port} @ {self.baudrate} baud")
            return True
        except serial.SerialException as e:
            print(f"❌ Error al conectar: {e}")
            return False
    
    def send_command(self, cmd):
        """Enviar comando al STM32"""
        if self.serial and self.serial.is_open:
            self.serial.write(cmd.encode())
            print(f"📤 Enviado: {cmd}")
            time.sleep(0.1)
            return True
        else:
            print("❌ No conectado")
            return False
    
    def read_response(self):
        """Leer respuesta del STM32"""
        if self.serial and self.serial.is_open:
            if self.serial.in_waiting > 0:
                response = self.serial.read(self.serial.in_waiting).decode('utf-8', errors='ignore')
                return response
        return None
    
    def reader_thread(self):
        """Hilo para leer respuestas continuamente"""
        while self.running:
            response = self.read_response()
            if response:
                print(f"📥 {response.strip()}")
            time.sleep(0.1)
    
    def interactive_mode(self):
        """Modo interactivo de consola"""
        print("\n" + "="*50)
        print("🎮 GAME CONTROLLER - Modo Interactivo")
        print("="*50)
        print("\nComandos:")
        print("  g, 1  - Galaga")
        print("  2     - Juego 2 (futuro)")
        print("  0     - Detener")
        print("  s     - Estado")
        print("  r     - Reiniciar")
        print("  a     - Auto-play")
        print("  m     - Manual (sensores)")
        print("  q     - Salir")
        print("-"*50)
        
        # Iniciar hilo de lectura
        self.running = True
        reader = threading.Thread(target=self.reader_thread, daemon=True)
        reader.start()
        
        try:
            while True:
                cmd = input("\n> ").strip().upper()
                
                if cmd == 'Q':
                    print("👋 Saliendo...")
                    break
                elif cmd in ['G', '1', '2', '0', 'S', 'R', 'A', 'M']:
                    self.send_command(cmd)
                elif cmd:
                    print("❓ Comando no reconocido. Usa 'q' para salir.")
        except KeyboardInterrupt:
            print("\n👋 Interrumpido por el usuario")
        finally:
            self.running = False
            if self.serial:
                self.serial.close()
                print("🔌 Conexión cerrada")


def main():
    # Determinar el puerto
    port = sys.argv[1] if len(sys.argv) > 1 else None
    
    # Crear controlador
    controller = GameController(port)
    
    if controller.serial and controller.serial.is_open:
        controller.interactive_mode()
    else:
        print("\n❌ No se pudo establecer conexión")
        sys.exit(1)


if __name__ == "__main__":
    main()
