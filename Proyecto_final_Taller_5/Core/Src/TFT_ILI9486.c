#include "TFT_ILI9486.h"
#include "main.h"
#include <math.h>   // For sqrtf()
#include <stdlib.h> // For abs()
#include <string.h>

// Macros for Control Pins (Updated based on main.h)
#define RS_PORT RS_GPIO_Port
#define WR_PORT WR_GPIO_Port
#define CS_PORT CS_GPIO_Port
#define RST_PORT RST_GPIO_Port

// Optimization: Direct BSRR Macros for Control Pins (All on GPIOB)
#define RS_L GPIOB->BSRR = (uint32_t)RS_Pin << 16
#define RS_H GPIOB->BSRR = RS_Pin

#define WR_L GPIOB->BSRR = (uint32_t)WR_Pin << 16
#define WR_H GPIOB->BSRR = WR_Pin

#define CS_L GPIOB->BSRR = (uint32_t)CS_Pin << 16
#define CS_H GPIOB->BSRR = CS_Pin

#define RST_L GPIOB->BSRR = (uint32_t)RST_Pin << 16
#define RST_H GPIOB->BSRR = RST_Pin

// Optimization: Direct BSRR Access for Speed
// We define these as inline for maximum performance
static inline void ILI_Write16(uint16_t data) {
  uint32_t pa = 0; // GPIOA BSRR value
  uint32_t pb = 0; // GPIOB BSRR value
  uint32_t pc = 0; // GPIOC BSRR value

  // Map Data Bits to Ports using ternary operators for branchless-ish assembly
  // Set bits go to lower 16 bits of BSRR, Reset bits go to upper 16 bits

  // DB0 -> PB13
  if (data & 0x0001)
    pb |= DB0_Pin;
  else
    pb |= (DB0_Pin << 16);
  // DB1 -> PC4
  if (data & 0x0002)
    pc |= DB1_Pin;
  else
    pc |= (DB1_Pin << 16);
  // DB2 -> PA10
  if (data & 0x0004)
    pa |= DB2_Pin;
  else
    pa |= (DB2_Pin << 16);
  // DB3 -> PB5
  if (data & 0x0008)
    pb |= DB3_Pin;
  else
    pb |= (DB3_Pin << 16);
  // DB4 -> PB10
  if (data & 0x0010)
    pb |= DB4_Pin;
  else
    pb |= (DB4_Pin << 16);
  // DB5 -> PA8
  if (data & 0x0020)
    pa |= DB5_Pin;
  else
    pa |= (DB5_Pin << 16);
  // DB6 -> PA9
  if (data & 0x0040)
    pa |= DB6_Pin;
  else
    pa |= (DB6_Pin << 16);
  // DB7 -> PC7
  if (data & 0x0080)
    pc |= DB7_Pin;
  else
    pc |= (DB7_Pin << 16);
  // DB8 -> PC5
  if (data & 0x0100)
    pc |= DB8_Pin;
  else
    pc |= (DB8_Pin << 16);
  // DB9 -> PC6
  if (data & 0x0200)
    pc |= DB9_Pin;
  else
    pc |= (DB9_Pin << 16);
  // DB10 -> PC8
  if (data & 0x0400)
    pc |= DB10_Pin;
  else
    pc |= (DB10_Pin << 16);
  // DB11 -> PC9
  if (data & 0x0800)
    pc |= DB11_Pin;
  else
    pc |= (DB11_Pin << 16);
  // DB12 -> PA5
  if (data & 0x1000)
    pa |= DB12_Pin;
  else
    pa |= (DB12_Pin << 16);
  // DB13 -> PA6
  if (data & 0x2000)
    pa |= DB13_Pin;
  else
    pa |= (DB13_Pin << 16);
  // DB14 -> PA7
  if (data & 0x4000)
    pa |= DB14_Pin;
  else
    pa |= (DB14_Pin << 16);
  // DB15 -> PB6
  if (data & 0x8000)
    pb |= DB15_Pin;
  else
    pb |= (DB15_Pin << 16);

  // Apply to ports atomically
  GPIOA->BSRR = pa;
  GPIOB->BSRR = pb;
  GPIOC->BSRR = pc;

  // TURBO STROBE: Minimal NOPs for max speed (approx 30-40ns)
  // WR Low
  GPIOB->BSRR = (uint32_t)WR_Pin << 16;
  __NOP();
  __NOP();
  __NOP();
  // WR High
  GPIOB->BSRR = WR_Pin;
  __NOP();
  __NOP();
  __NOP();
}

// Optimized Scaline Based Darkfield Demo
void ILI_Demo_Darkfield(uint16_t r_in, uint16_t r_out) {
  ILI_FillScreen(BLACK);
  int16_t cx = ILI9486_SCREEN_WIDTH / 2;
  int16_t cy = ILI9486_SCREEN_HEIGHT / 2;

  // Scanline algorithm from Conversación.md
  for (int y = -r_out; y <= r_out; y++) {
    int16_t half_width_out =
        (int16_t)sqrtf((float)(r_out * r_out) - (float)(y * y));
    int16_t half_width_in = 0;

    if (abs(y) < r_in) {
      half_width_in = (int16_t)sqrtf((float)(r_in * r_in) - (float)(y * y));
    }

    if ((half_width_out - half_width_in) > 0) {
      // Left segment
      ILI_FillRect(cx - half_width_out, cy + y, half_width_out - half_width_in,
                   1, WHITE);
      // Right segment
      ILI_FillRect(cx + half_width_in, cy + y, half_width_out - half_width_in,
                   1, WHITE);
    }
  }
}

static void ILI_WriteCmd(uint16_t cmd) {
  // RS Low (PB14)
  GPIOB->BSRR = (uint32_t)RS_Pin << 16;
  ILI_Write16(cmd);
}

static void ILI_WriteData(uint16_t data) {
  // RS High (PB14)
  GPIOB->BSRR = RS_Pin;
  ILI_Write16(data);
}

void ILI_Init(void) {
  // Toggle specific pins to ensure known state using direct BSRR for
  // consistency
  GPIOB->BSRR = CS_Pin | WR_Pin | RS_Pin | RST_Pin; // All High
  HAL_Delay(10);

  // Hardware Reset Sequence
  RST_L;
  HAL_Delay(100);
  RST_H;
  HAL_Delay(100);

  CS_L; // Select Chip

  // Unlock Internal Timing Registers (Magic for ILI9486)
  ILI_WriteCmd(0xF1);
  ILI_WriteData(0x36);
  ILI_WriteData(0x04);
  ILI_WriteData(0x00);
  ILI_WriteData(0x3C);
  ILI_WriteData(0x0F);
  ILI_WriteData(0x8F);

  ILI_WriteCmd(0xF2);
  ILI_WriteData(0x18);
  ILI_WriteData(0xA3);
  ILI_WriteData(0x12);
  ILI_WriteData(0x02);
  ILI_WriteData(0xB2);
  ILI_WriteData(0x12);
  ILI_WriteData(0xFF);
  ILI_WriteData(0x10);
  ILI_WriteData(0x00);

  ILI_WriteCmd(0xF8);
  ILI_WriteData(0x21);
  ILI_WriteData(0x04);

  ILI_WriteCmd(0xF9);
  ILI_WriteData(0x00);
  ILI_WriteData(0x08);

  // Standard Init
  ILI_WriteCmd(0x36);  // Memory Access Control
  ILI_WriteData(0x48); // BGR Order

  ILI_WriteCmd(0x3A);  // Interface Pixel Format
  ILI_WriteData(0x55); // 16-bit

  // Power Control
  ILI_WriteCmd(0xC0);
  ILI_WriteData(0x0d);
  ILI_WriteData(0x0d);

  ILI_WriteCmd(0xC1);
  ILI_WriteData(0x43);
  ILI_WriteData(0x00);

  ILI_WriteCmd(0xC2);
  ILI_WriteData(0x00);

  ILI_WriteCmd(0xC5);
  ILI_WriteData(0x00);
  ILI_WriteData(0x48);

  // Gamma
  ILI_WriteCmd(0xE0);
  ILI_WriteData(0x0f);
  ILI_WriteData(0x24);
  ILI_WriteData(0x1c);
  ILI_WriteData(0x0a);
  ILI_WriteData(0x0f);
  ILI_WriteData(0x08);
  ILI_WriteData(0x43);
  ILI_WriteData(0x88);
  ILI_WriteData(0x32);
  ILI_WriteData(0x0f);
  ILI_WriteData(0x10);
  ILI_WriteData(0x06);
  ILI_WriteData(0x0F);
  ILI_WriteData(0x07);
  ILI_WriteData(0x00);

  ILI_WriteCmd(0xE1);
  ILI_WriteData(0x0F);
  ILI_WriteData(0x38);
  ILI_WriteData(0x2E);
  ILI_WriteData(0x0A);
  ILI_WriteData(0x0D);
  ILI_WriteData(0x02);
  ILI_WriteData(0x43);
  ILI_WriteData(0x78);
  ILI_WriteData(0x3F);
  ILI_WriteData(0x07);
  ILI_WriteData(0x13);
  ILI_WriteData(0x0A);
  ILI_WriteData(0x25);
  ILI_WriteData(0x11);
  ILI_WriteData(0x00);

  ILI_WriteCmd(0x11); // Sleep Out
  HAL_Delay(150);

  ILI_WriteCmd(0x29); // Display ON
  HAL_Delay(100);

  // Force a clear screen to RED to verify life immediately
  ILI_FillScreen(0xF800); // RAW RED

  CS_H; // Deselect
}

void ILI_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  CS_L;
  ILI_WriteCmd(0x2A); // Column Addr Set
  ILI_WriteData(x0 >> 8);
  ILI_WriteData(x0 & 0xFF);
  ILI_WriteData(x1 >> 8);
  ILI_WriteData(x1 & 0xFF);

  ILI_WriteCmd(0x2B); // Page Addr Set
  ILI_WriteData(y0 >> 8);
  ILI_WriteData(y0 & 0xFF);
  ILI_WriteData(y1 >> 8);
  ILI_WriteData(y1 & 0xFF);

  ILI_WriteCmd(0x2C); // Memory Write
  CS_H;
}

// Optimization: Pre-calculate BSRR values for the color to avoid per-pixel
// mapping overhead.
void ILI_FillRect(int x, int y, int w, int h, uint16_t color) {
  if ((x >= ILI9486_SCREEN_WIDTH) || (y >= ILI9486_SCREEN_HEIGHT))
    return;
  if ((x + w - 1) >= ILI9486_SCREEN_WIDTH)
    w = ILI9486_SCREEN_WIDTH - x;
  if ((y + h - 1) >= ILI9486_SCREEN_HEIGHT)
    h = ILI9486_SCREEN_HEIGHT - y;

  ILI_SetAddressWindow(x, y, x + w - 1, y + h - 1);

  // Pre-calculate BSRR values for the color
  uint32_t pa_set = 0, pa_reset = 0;
  uint32_t pb_set = 0, pb_reset = 0;
  uint32_t pc_set = 0, pc_reset = 0;

  // Map Data Bits to Ports (Same logic as ILI_Write16 but pre-calculated)
  // DB0 -> PB13
  if (color & 0x0001)
    pb_set |= DB0_Pin;
  else
    pb_reset |= DB0_Pin;
  // DB1 -> PC4
  if (color & 0x0002)
    pc_set |= DB1_Pin;
  else
    pc_reset |= DB1_Pin;
  // DB2 -> PA10
  if (color & 0x0004)
    pa_set |= DB2_Pin;
  else
    pa_reset |= DB2_Pin;
  // DB3 -> PB5
  if (color & 0x0008)
    pb_set |= DB3_Pin;
  else
    pb_reset |= DB3_Pin;
  // DB4 -> PB10
  if (color & 0x0010)
    pb_set |= DB4_Pin;
  else
    pb_reset |= DB4_Pin;
  // DB5 -> PA8
  if (color & 0x0020)
    pa_set |= DB5_Pin;
  else
    pa_reset |= DB5_Pin;
  // DB6 -> PA9
  if (color & 0x0040)
    pa_set |= DB6_Pin;
  else
    pa_reset |= DB6_Pin;
  // DB7 -> PC7
  if (color & 0x0080)
    pc_set |= DB7_Pin;
  else
    pc_reset |= DB7_Pin;
  // DB8 -> PC5
  if (color & 0x0100)
    pc_set |= DB8_Pin;
  else
    pc_reset |= DB8_Pin;
  // DB9 -> PC6
  if (color & 0x0200)
    pc_set |= DB9_Pin;
  else
    pc_reset |= DB9_Pin;
  // DB10 -> PC8
  if (color & 0x0400)
    pc_set |= DB10_Pin;
  else
    pc_reset |= DB10_Pin;
  // DB11 -> PC9
  if (color & 0x0800)
    pc_set |= DB11_Pin;
  else
    pc_reset |= DB11_Pin;
  // DB12 -> PA5
  if (color & 0x1000)
    pa_set |= DB12_Pin;
  else
    pa_reset |= DB12_Pin;
  // DB13 -> PA6
  if (color & 0x2000)
    pa_set |= DB13_Pin;
  else
    pa_reset |= DB13_Pin;
  // DB14 -> PA7
  if (color & 0x4000)
    pa_set |= DB14_Pin;
  else
    pa_reset |= DB14_Pin;
  // DB15 -> PB6
  if (color & 0x8000)
    pb_set |= DB15_Pin;
  else
    pb_reset |= DB15_Pin;

  // Construct final BSRR values: High 16 for reset, Low 16 for set
  uint32_t pa_bsrr = (pa_reset << 16) | pa_set;
  uint32_t pb_bsrr = (pb_reset << 16) | pb_set;
  uint32_t pc_bsrr = (pc_reset << 16) | pc_set;

  // Cache WR Pin BSRR values
  uint32_t wr_lo = (uint32_t)WR_Pin << 16;
  uint32_t wr_hi = WR_Pin;

  // Prepare for transfer
  CS_L;
  RS_H;

  // Set Data Lines ONCE
  GPIOA->BSRR = pa_bsrr;
  GPIOB->BSRR = pb_bsrr;
  GPIOC->BSRR = pc_bsrr;

  // Since we are writing the SAME color repeatedly, we don't need to rewrite
  // data ports! We only need to toggle WR. HOWEVER, caution: if any other
  // interrupt touches these ports, we might have issues. But for raw speed in a
  // blocking function, we assume exclusive access or that interrupts restore
  // state. Safe approach: Rewrite data every time? No, standard GPIO doesn't
  // change on its own. BUT: bit-banging WR is on GPIOB. Writing to GPIOB->BSRR
  // for WR *might* affect other pins if we are not careful? No, BSRR write only
  // affects bits set to 1.

  // So we can set the data bus ONCE, and then just toggle WR!
  // That is a HUGE optimization.
  // WR is on GPIOB (PB1).
  // Data bits on GPIOB are: PB13(DB0), PB5(DB3), PB10(DB4), PB6(DB15).
  // If we write to GPIOB->BSRR for WR, we must ensure we don't touch data pins.
  // wr_lo has only bit (1<<1) set in high half. wr_hi has only bit (1<<1) set
  // in low half. They don't overlap with data pins unless data pins are also
  // PB1 (which isn't).

  // Let's verify PB1 is not a data pin.
  // DB0->PB13, DB3->PB5, DB4->PB10, DB15->PB6.
  // PB1 is safe.

  uint32_t total_pixels = w * h;

  // Loop Unrolling (x16)
  uint32_t loops = total_pixels >> 4; // /16
  uint32_t rem = total_pixels & 0xF;  // %16

  for (uint32_t i = 0; i < loops; i++) {
    // 1
    GPIOB->BSRR = wr_lo;
    __NOP();
    __NOP();
    GPIOB->BSRR = wr_hi;
    __NOP();
    __NOP();
    // 2
    GPIOB->BSRR = wr_lo;
    __NOP();
    __NOP();
    GPIOB->BSRR = wr_hi;
    __NOP();
    __NOP();
    // 3
    GPIOB->BSRR = wr_lo;
    __NOP();
    __NOP();
    GPIOB->BSRR = wr_hi;
    __NOP();
    __NOP();
    // 4
    GPIOB->BSRR = wr_lo;
    __NOP();
    __NOP();
    GPIOB->BSRR = wr_hi;
    __NOP();
    __NOP();
    // 5
    GPIOB->BSRR = wr_lo;
    __NOP();
    __NOP();
    GPIOB->BSRR = wr_hi;
    __NOP();
    __NOP();
    // 6
    GPIOB->BSRR = wr_lo;
    __NOP();
    __NOP();
    GPIOB->BSRR = wr_hi;
    __NOP();
    __NOP();
    // 7
    GPIOB->BSRR = wr_lo;
    __NOP();
    __NOP();
    GPIOB->BSRR = wr_hi;
    __NOP();
    __NOP();
    // 8
    GPIOB->BSRR = wr_lo;
    __NOP();
    __NOP();
    GPIOB->BSRR = wr_hi;
    __NOP();
    __NOP();
    // 9
    GPIOB->BSRR = wr_lo;
    __NOP();
    __NOP();
    GPIOB->BSRR = wr_hi;
    __NOP();
    __NOP();
    // 10
    GPIOB->BSRR = wr_lo;
    __NOP();
    __NOP();
    GPIOB->BSRR = wr_hi;
    __NOP();
    __NOP();
    // 11
    GPIOB->BSRR = wr_lo;
    __NOP();
    __NOP();
    GPIOB->BSRR = wr_hi;
    __NOP();
    __NOP();
    // 12
    GPIOB->BSRR = wr_lo;
    __NOP();
    __NOP();
    GPIOB->BSRR = wr_hi;
    __NOP();
    __NOP();
    // 13
    GPIOB->BSRR = wr_lo;
    __NOP();
    __NOP();
    GPIOB->BSRR = wr_hi;
    __NOP();
    __NOP();
    // 14
    GPIOB->BSRR = wr_lo;
    __NOP();
    __NOP();
    GPIOB->BSRR = wr_hi;
    __NOP();
    __NOP();
    // 15
    GPIOB->BSRR = wr_lo;
    __NOP();
    __NOP();
    GPIOB->BSRR = wr_hi;
    __NOP();
    __NOP();
    // 16
    GPIOB->BSRR = wr_lo;
    __NOP();
    __NOP();
    GPIOB->BSRR = wr_hi;
    __NOP();
    __NOP();
  }

  for (uint32_t i = 0; i < rem; i++) {
    GPIOB->BSRR = wr_lo;
    __NOP();
    __NOP();
    GPIOB->BSRR = wr_hi;
    __NOP();
    __NOP();
  }

  CS_H;
}

void ILI_FillScreen(uint16_t color) {
  ILI_FillRect(0, 0, ILI9486_SCREEN_WIDTH, ILI9486_SCREEN_HEIGHT, color);
}

void ILI_DrawPixel(int x, int y, uint16_t color) {
  if ((x < 0) || (x >= ILI9486_SCREEN_WIDTH) || (y < 0) ||
      (y >= ILI9486_SCREEN_HEIGHT))
    return;
  ILI_SetAddressWindow(x, y, x, y);
  CS_L;
  RS_H;
  ILI_Write16(color);
  CS_H;
}

// External handle for UART (assumed defined in main.c)
extern UART_HandleTypeDef huart2;
#include <stdio.h> // For sprintf

// Improved FPS Test & Benchmark
void ILI_TestFPS(void) {
  // Setup variables for FPS counting
  uint32_t start_time = HAL_GetTick();
  uint32_t frame_count = 0;
  char msg[64];

// Visuals: Multiple Bouncing Rects instead of one
#define NUM_RECTS 10
  struct {
    int x, y, dx, dy, size;
    uint16_t color;
  } rects[NUM_RECTS];

  // Initialize random rects
  for (int i = 0; i < NUM_RECTS; i++) {
    rects[i].size = 20 + (rand() % 40); // 20-60
    rects[i].x = rand() % (ILI9486_SCREEN_WIDTH - rects[i].size);
    rects[i].y = rand() % (ILI9486_SCREEN_HEIGHT - rects[i].size);
    rects[i].dx = (rand() % 5) + 2; // 2-6 speed
    rects[i].dy = (rand() % 5) + 2;
    if (rand() % 2)
      rects[i].dx = -rects[i].dx;
    if (rand() % 2)
      rects[i].dy = -rects[i].dy;
    rects[i].color = (rand() % 0xFFFF);
  }

  // Clear screen once
  ILI_FillScreen(BLACK);

  // Run the loop indefinitely (user can reset board to stop)
  // Or run for a fixed duration. User said "maximize refresh", so let's run a
  // stress loop. We will run for 1000 frames to get a good measurement.
  for (int f = 0; f < 2000; f++) {

    // 1. Clear previous positions (Draw Black)
    for (int i = 0; i < NUM_RECTS; i++) {
      ILI_FillRect(rects[i].x, rects[i].y, rects[i].size, rects[i].size, BLACK);
    }

    // 2. Update and Draw New
    for (int i = 0; i < NUM_RECTS; i++) {
      // Update Position
      rects[i].x += rects[i].dx;
      rects[i].y += rects[i].dy;

      // Bounce
      if (rects[i].x <= 0 || rects[i].x >= ILI9486_SCREEN_WIDTH - rects[i].size)
        rects[i].dx = -rects[i].dx;
      if (rects[i].y <= 0 ||
          rects[i].y >= ILI9486_SCREEN_HEIGHT - rects[i].size)
        rects[i].dy = -rects[i].dy;

      // Clamp to screen (safety)
      if (rects[i].x < 0)
        rects[i].x = 0;
      if (rects[i].y < 0)
        rects[i].y = 0;

      // Draw Color
      ILI_FillRect(rects[i].x, rects[i].y, rects[i].size, rects[i].size,
                   rects[i].color);
    }

    frame_count++;

    // Every 100 frames, estimate FPS
    if (frame_count % 100 == 0) {
      uint32_t elapsed = HAL_GetTick() - start_time;
      if (elapsed > 0) {
        float fps = (float)frame_count / (elapsed / 1000.0f);
        sprintf(msg, "Frames: %lu, Time: %lu ms, FPS: %.2f\r\n", frame_count,
                elapsed, fps);
        HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 100);
      }
    }
  }
}

// Helpers for main.c compatibility
void ILI_TestColors(void) {
  ILI_FillScreen(RED);
  HAL_Delay(250); // Faster colors
  ILI_FillScreen(GREEN);
  HAL_Delay(250);
  ILI_FillScreen(BLUE);
  HAL_Delay(250);
  ILI_FillScreen(WHITE);
  HAL_Delay(250);
  ILI_FillScreen(BLACK);
  HAL_Delay(250);
}

static void swap(uint16_t *a, uint16_t *b) {
  uint16_t t = *a;
  *a = *b;
  *b = t;
}

void ILI_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                  uint16_t color) {
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(&x0, &y0);
    swap(&x1, &y1);
  }
  if (x0 > x1) {
    swap(&x0, &x1);
    swap(&y0, &y1);
  }
  int16_t dx = x1 - x0;
  int16_t dy = abs(y1 - y0);
  int16_t err = dx / 2;
  int16_t ystep;
  if (y0 < y1)
    ystep = 1;
  else
    ystep = -1;

  for (; x0 <= x1; x0++) {
    if (steep)
      ILI_DrawPixel(y0, x0, color);
    else
      ILI_DrawPixel(x0, y0, color);
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

void ILI_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                  uint16_t color) {
  ILI_DrawLine(x, y, x + w - 1, y, color);
  ILI_DrawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
  ILI_DrawLine(x, y, x, y + h - 1, color);
  ILI_DrawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void ILI_DrawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;
  ILI_DrawPixel(x0, y0 + r, color);
  ILI_DrawPixel(x0, y0 - r, color);
  ILI_DrawPixel(x0 + r, y0, color);
  ILI_DrawPixel(x0 - r, y0, color);
  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    ILI_DrawPixel(x0 + x, y0 + y, color);
    ILI_DrawPixel(x0 - x, y0 + y, color);
    ILI_DrawPixel(x0 + x, y0 - y, color);
    ILI_DrawPixel(x0 - x, y0 - y, color);
    ILI_DrawPixel(x0 + y, y0 + x, color);
    ILI_DrawPixel(x0 - y, y0 + x, color);
    ILI_DrawPixel(x0 + y, y0 - x, color);
    ILI_DrawPixel(x0 - y, y0 - x, color);
  }
}
