/**
 * @file ili9486.c
 * @brief Implementación del driver para pantalla ILI9486 en MODO PARALELO 8-BITS.
 * @author Jules (Asistente de IA)
 */

#include "ili9486.h"

// Acceso directo a registros para máxima velocidad
// ODR (Output Data Register) offsets
// GPIOC Base: 0x40020800
// GPIOB Base: 0x40020400

// Macros para escribir en el bus de datos (PC0-PC7)
// Se asume que ILI_DATA_PORT es GPIOC (o el que se defina en .h)
#define ILI_WRITE_BUS(data)     (ILI_DATA_PORT->ODR = (ILI_DATA_PORT->ODR & 0xFF00) | (data))

// Macros para señales de control
// Usamos los puertos definidos en el .h (que vienen del main.h)

#define ILI_RS_SET      (ILI_RS_PORT->BSRR = ILI_RS_PIN)
#define ILI_RS_RESET    (ILI_RS_PORT->BSRR = (uint32_t)ILI_RS_PIN << 16)

#define ILI_WR_SET      (ILI_WR_PORT->BSRR = ILI_WR_PIN)
#define ILI_WR_RESET    (ILI_WR_PORT->BSRR = (uint32_t)ILI_WR_PIN << 16)

#define ILI_CS_SET      (ILI_CS_PORT->BSRR = ILI_CS_PIN)
#define ILI_CS_RESET    (ILI_CS_PORT->BSRR = (uint32_t)ILI_CS_PIN << 16)

#define ILI_RST_SET     (ILI_RST_PORT->BSRR = ILI_RST_PIN)
#define ILI_RST_RESET   (ILI_RST_PORT->BSRR = (uint32_t)ILI_RST_PIN << 16)

// Strobe de escritura: WR Low -> WR High
#define ILI_WR_STROBE   { ILI_WR_RESET; __asm("nop"); ILI_WR_SET; }

static void Delay(volatile uint32_t count) {
    while (count--) {
        __asm("nop");
    }
}

static void ILI_WriteCmd(uint8_t cmd) {
    ILI_RS_RESET;   // Comando
    ILI_WRITE_BUS(cmd);
    ILI_WR_STROBE;
}

static void ILI_WriteData(uint8_t data) {
    ILI_RS_SET;     // Dato
    ILI_WRITE_BUS(data);
    ILI_WR_STROBE;
}

void ILI9486_Init(void) {
    // NOTA: La inicialización de GPIOs (Relojes, Modo Salida, Velocidad)
    // se delega a main.c (generado por STM32CubeMX).
    // Asegúrate de configurar los pines con velocidad "Very High".

    // Estado inicial
    ILI_CS_SET;
    ILI_WR_SET;
    ILI_RS_SET;
    ILI_RST_SET;

    // 4. Secuencia de Reset
    ILI_CS_RESET;
    ILI_RST_SET;
    Delay(50000);
    ILI_RST_RESET;
    Delay(50000);
    ILI_RST_SET;
    Delay(200000);

    // 5. Comandos de Inicialización
    ILI_WriteCmd(0xB0); // Interface Mode Control
    ILI_WriteData(0x00);

    ILI_WriteCmd(0x11); // Sleep OUT
    Delay(150000);

    ILI_WriteCmd(0x3A); // Interface Pixel Format
    ILI_WriteData(0x55); // 16 bits/pixel (5-6-5)

    ILI_WriteCmd(0x36); // Memory Access Control
    ILI_WriteData(0x28); // BGR Order

    ILI_WriteCmd(0xC2); // Power Control 3
    ILI_WriteData(0x44);

    ILI_WriteCmd(0xC5); // VCOM Control
    ILI_WriteData(0x00);
    ILI_WriteData(0x00);
    ILI_WriteData(0x00);
    ILI_WriteData(0x00);

    ILI_WriteCmd(0xE0); // PGAMCTRL
    uint8_t gammaP[] = {0x0F, 0x1F, 0x1C, 0x0C, 0x0F, 0x08, 0x48, 0x98, 0x37, 0x0A, 0x13, 0x04, 0x11, 0x0D, 0x00};
    for(int i=0; i<15; i++) ILI_WriteData(gammaP[i]);

    ILI_WriteCmd(0xE1); // NGAMCTRL
    uint8_t gammaN[] = {0x0F, 0x32, 0x2E, 0x0B, 0x0D, 0x05, 0x47, 0x75, 0x37, 0x06, 0x10, 0x03, 0x24, 0x20, 0x00};
    for(int i=0; i<15; i++) ILI_WriteData(gammaN[i]);

    ILI_WriteCmd(0x29); // Display ON
    Delay(150000);

    ILI_CS_SET;
}

void ILI9486_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    ILI_CS_RESET;

    // Column Address Set
    ILI_WriteCmd(0x2A);
    ILI_WriteData(x0 >> 8);
    ILI_WriteData(x0 & 0xFF);
    ILI_WriteData(x1 >> 8);
    ILI_WriteData(x1 & 0xFF);

    // Page Address Set
    ILI_WriteCmd(0x2B);
    ILI_WriteData(y0 >> 8);
    ILI_WriteData(y0 & 0xFF);
    ILI_WriteData(y1 >> 8);
    ILI_WriteData(y1 & 0xFF);

    // Write to RAM
    ILI_WriteCmd(0x2C);

    ILI_CS_SET;
}

void ILI9486_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if ((x >= ILI9486_ANCHO) || (y >= ILI9486_ALTO)) return;

    ILI9486_SetWindow(x, y, x, y);

    ILI_CS_RESET;
    ILI_RS_SET; // Data

    // MSB
    ILI_WRITE_BUS(color >> 8);
    ILI_WR_STROBE;
    // LSB
    ILI_WRITE_BUS(color & 0xFF);
    ILI_WR_STROBE;

    ILI_CS_SET;
}

void ILI9486_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if ((x >= ILI9486_ANCHO) || (y >= ILI9486_ALTO)) return;
    if ((x + w - 1) >= ILI9486_ANCHO) w = ILI9486_ANCHO - x;
    if ((y + h - 1) >= ILI9486_ALTO) h = ILI9486_ALTO - y;

    ILI9486_SetWindow(x, y, x + w - 1, y + h - 1);

    ILI_CS_RESET;
    ILI_RS_SET; // Data Mode

    uint32_t total_pixels = (uint32_t)w * h;
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    // Loop optimizado
    while (total_pixels--) {
        ILI_WRITE_BUS(hi);
        ILI_WR_STROBE;
        ILI_WRITE_BUS(lo);
        ILI_WR_STROBE;
    }

    ILI_CS_SET;
}

void ILI9486_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data) {
    if ((x >= ILI9486_ANCHO) || (y >= ILI9486_ALTO)) return;
    if ((x + w - 1) >= ILI9486_ANCHO) w = ILI9486_ANCHO - x;
    if ((y + h - 1) >= ILI9486_ALTO) h = ILI9486_ALTO - y;

    ILI9486_SetWindow(x, y, x + w - 1, y + h - 1);

    ILI_CS_RESET;
    ILI_RS_SET;

    uint32_t total_pixels = (uint32_t)w * h;
    const uint16_t* p = data;

    while (total_pixels--) {
        uint16_t color = *p++;
        ILI_WRITE_BUS(color >> 8);
        ILI_WR_STROBE;
        ILI_WRITE_BUS(color & 0xFF);
        ILI_WR_STROBE;
    }

    ILI_CS_SET;
}
