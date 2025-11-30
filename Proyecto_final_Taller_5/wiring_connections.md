# Guía de Conexión: Pantalla ILI9486 (Modo Paralelo 8-Bits)

Esta guía documenta cómo conectar tu pantalla al STM32F411 (Nucleo) según la configuración actual del código (`main.h` y `ili9486.h`).

## 1. Pines de Datos (Puerto C)
Estos pines envían los colores y comandos a la pantalla. Debes conectarlos en orden.

| Etiqueta Pantalla | Pin STM32 (Morpho) | Ubicación Física (CN7/CN10) |
| :--- | :--- | :--- |
| **D0** / DB0 | **PC0** | CN7 Pin 37 |
| **D1** / DB1 | **PC1** | CN7 Pin 35 |
| **D2** / DB2 | **PC2** | CN7 Pin 34 |
| **D3** / DB3 | **PC3** | CN7 Pin 36 |
| **D4** / DB4 | **PC4** | CN7 Pin 34 (Revisar) |
| **D5** / DB5 | **PC5** | CN7 Pin 6 |
| **D6** / DB6 | **PC6** | CN7 Pin 4 |
| **D7** / DB7 | **PC7** | CN7 Pin 19 |

*Nota: Busca los pines marcados como `PCx` en la placa blanca.*

## 2. Pines de Control (Puerto B)
Estos pines controlan cuándo se envían los datos.

| Etiqueta Pantalla | Función | Pin STM32 |
| :--- | :--- | :--- |
| **RS** / DC | Comando/Dato | **PB0** |
| **WR** | Escritura (Write) | **PB1** |
| **CS** | Chip Select | **PB2** |
| **RST** / RESET | Reset | **PB10** |
| **RD** | Lectura | **3.3V** (Opcional) |

## 3. Sensores

### Acelerómetro (GY-521)
| Pin Sensor | Pin STM32 |
| :--- | :--- |
| **SCL** | **PB6** |
| **SDA** | **PB7** |
| **VCC** | 5V |
| **GND** | GND |

### Joystick
| Pin Joystick | Pin STM32 |
| :--- | :--- |
| **VRx** / VRy | **PA0** |
| **VCC** | 3.3V |
| **GND** | GND |
