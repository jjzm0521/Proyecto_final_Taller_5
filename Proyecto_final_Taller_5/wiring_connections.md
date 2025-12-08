# Conexiones del Display ILI9486 (16-bit Paralelo)

Tabla de conexiones entre el microcontrolador STM32F411 y la pantalla LCD ILI9486.

## 1. Bus de Datos (16-bit)

| Bit de Datos (DB) | Pin STM32 | Puerto |
| :--- | :--- | :--- |
| **DB0** | PB13 | GPIOB |
| **DB1** | PC4 | GPIOC |
| **DB2** | PA10 | GPIOA |
| **DB3** | PB5 | GPIOB |
| **DB4** | PB10 | GPIOB |
| **DB5** | PA8 | GPIOA |
| **DB6** | PA9 | GPIOA |
| **DB7** | PC7 | GPIOC |
| **DB8** | PC5 | GPIOC |
| **DB9** | PC6 | GPIOC |
| **DB10** | PC8 | GPIOC |
| **DB11** | PC9 | GPIOC |
| **DB12** | PA5 | GPIOA |
| **DB13** | PA6 | GPIOA |
| **DB14** | PA7 | GPIOA |
| **DB15** | PB6 | GPIOB |

## 2. Señales de Control

| Señal | Pin STM32 | Puerto | Función |
| :--- | :--- | :--- | :--- |
| **CS** | PB1 | GPIOB | Chip Select (Active Low) |
| **RST** | PB2 | GPIOB | Reset (Active Low) |
| **RS / DC** | PB14 | GPIOB | Register Select (Dat/Cmd) |
| **WR** | PB15 | GPIOB | Write Strobe (Active Low) |
| **RD** | - | - | Read Strobe (3.3V fijo) |

## 3. Otros

| Señal | Pin STM32 | Descripción |
| :--- | :--- | :--- |
| **LED** | 3.3V | Retroiluminación (Directo a VCC) |
| **VCC** | 5V / 3.3V| Alimentación Lógica |
| **GND** | GND | Tierra |

## 4. Otros Periféricos

### Acelerómetro GY-521 (I2C1)

| Pin | STM32 |
|-----|-------|
| SCL | PB8 |
| SDA | PB9 |

### LED Blinky (Debug)

| Función | Pin |
|---------|-----|
| LED | **PH1** |

*Parpadea con Timer 11 para verificar que el sistema sigue corriendo.*
