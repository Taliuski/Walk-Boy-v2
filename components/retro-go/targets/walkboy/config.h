// ─── Target Definition: Walk-Boy V2 (ESP32-S3-DevKitC-1 N16R8) ───
// Hardware consensuado para Andoni y Benja (listado_de_pines_V2.csv)
#pragma once

#include <driver/gpio.h>
#include <driver/spi_master.h>

#define RG_TARGET_NAME              "WALKBOY"

// ─── 1. Selector de Display (ST7789 Benja vs ILI9341 Andoni) ───
#define RG_DRIVER_ILI9341           0
#define RG_DRIVER_ST7789            1

// Por defecto se configura ST7789 (Benja). Para compilar la variante de Andoni,
// descomentar la lnea de ILI9341 o pasar -DRG_SCREEN_DRIVER=0 en la compilacin:
// #define RG_SCREEN_DRIVER         RG_DRIVER_ILI9341
#ifndef RG_SCREEN_DRIVER
#define RG_SCREEN_DRIVER            RG_DRIVER_ST7789
#endif

// ─── 2. Almacenamiento MicroSD (SPI nativo a 3.3V integrado en TFT) ───
// Pines consensuados: SCK=18, MOSI=17, MISO=14 (seguro contra PSRAM/boot), CS=5
#define RG_STORAGE_ROOT             "/sd"
#define RG_STORAGE_SDSPI_HOST       SPI3_HOST
#define RG_STORAGE_SDSPI_SPEED      SDMMC_FREQ_DEFAULT
#define RG_GPIO_SDSPI_CLK           GPIO_NUM_18
#define RG_GPIO_SDSPI_MOSI          GPIO_NUM_17
#define RG_GPIO_SDSPI_MISO          GPIO_NUM_14
#define RG_GPIO_SDSPI_CS            GPIO_NUM_5

// ─── 3. Audio Digital I2S (MAX98357A Clase D) ───
// Pines consensuados: BCLK=6, WS/LRC=7, DATA/DIN=16. DAC interno desactivado.
#define RG_AUDIO_USE_INT_DAC        0
#define RG_AUDIO_USE_EXT_DAC        1
#define RG_GPIO_SND_I2S_BCK         GPIO_NUM_6
#define RG_GPIO_SND_I2S_WS          GPIO_NUM_7
#define RG_GPIO_SND_I2S_DATA        GPIO_NUM_16

// ─── 4. Bus I2C para Lector NFC PN532 (Modo I2C: SW1=ON, SW2=OFF) ───
// Pines consensuados: SDA=21, SCL=47
#define RG_GPIO_I2C_SDA             GPIO_NUM_21
#define RG_GPIO_I2C_SCL             GPIO_NUM_47

// ─── 5. Pantalla TFT SPI (240x320 Landscape) ───
// Pines consensuados: SCK=12, MOSI=11, CS=10, DC=9, RST=4
#define RG_SCREEN_HOST              SPI2_HOST
#define RG_SCREEN_SPEED             SPI_MASTER_FREQ_40M
#define RG_SCREEN_BACKLIGHT         0
#define RG_SCREEN_WIDTH             320
#define RG_SCREEN_HEIGHT            240
#define RG_SCREEN_ROTATE            0
#define RG_SCREEN_VISIBLE_AREA      {0, 0, 0, 0}
#define RG_SCREEN_SAFE_AREA         {0, 0, 0, 0}

#define RG_GPIO_LCD_MISO            GPIO_NUM_NC
#define RG_GPIO_LCD_MOSI            GPIO_NUM_11
#define RG_GPIO_LCD_CLK             GPIO_NUM_12
#define RG_GPIO_LCD_CS              GPIO_NUM_10
#define RG_GPIO_LCD_DC              GPIO_NUM_9
#define RG_GPIO_LCD_RST             GPIO_NUM_4
#define RG_GPIO_LCD_BCKL            GPIO_NUM_NC

// Secuencia de inicializacin para ILI9341 (Andoni)
#define RG_SCREEN_INIT_ILI9341()                                                                                 \
    ILI9341_CMD(0xCF, 0x00, 0xc3, 0x30);                                                                         \
    ILI9341_CMD(0xED, 0x64, 0x03, 0x12, 0x81);                                                                   \
    ILI9341_CMD(0xE8, 0x85, 0x00, 0x78);                                                                         \
    ILI9341_CMD(0xCB, 0x39, 0x2c, 0x00, 0x34, 0x02);                                                             \
    ILI9341_CMD(0xF7, 0x20);                                                                                     \
    ILI9341_CMD(0xEA, 0x00, 0x00);                                                                               \
    ILI9341_CMD(0xC0, 0x1B);                 /* Power control //VRH[5:0] */                                      \
    ILI9341_CMD(0xC1, 0x12);                 /* Power control //SAP[2:0];BT[3:0] */                              \
    ILI9341_CMD(0xC5, 0x32, 0x3C);           /* VCM control */                                                   \
    ILI9341_CMD(0xC7, 0x91);                 /* VCM control2 */                                                  \
    ILI9341_CMD(0x36, 0x28);                 /* MADCTL: MV=1, BGR=1 (Landscape 320x240) */                       \
    ILI9341_CMD(0xB1, 0x00, 0x10);           /* Frame Rate Control (70Hz) */                                     \
    ILI9341_CMD(0xB6, 0x0A, 0xA2);           /* Display Function Control */                                      \
    ILI9341_CMD(0xF6, 0x01, 0x30);                                                                               \
    ILI9341_CMD(0xF2, 0x00);                 /* 3Gamma Function Disable */                                       \
    ILI9341_CMD(0x26, 0x01);                 /* Gamma curve selected */                                          \
    ILI9341_CMD(0xE0, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00); \
    ILI9341_CMD(0xE1, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F);

// Secuencia de inicializacin para ST7789V (Benja)
#define RG_SCREEN_INIT_ST7789()                                                                                  \
    ILI9341_CMD(0x36, 0x70);                 /* MADCTL: MX, MY, MV (Landscape 320x240) */                        \
    ILI9341_CMD(0x3A, 0x05);                 /* Interface Pixel Format: 16 bit/pixel */                          \
    ILI9341_CMD(0xB2, 0x0C, 0x0C, 0x00, 0x33, 0x33); /* Porch setting */                                        \
    ILI9341_CMD(0xB7, 0x35);                 /* Gate Control */                                                  \
    ILI9341_CMD(0xBB, 0x19);                 /* VCOM Setting */                                                  \
    ILI9341_CMD(0xC0, 0x2C);                 /* LCM Control */                                                   \
    ILI9341_CMD(0xC2, 0x01);                 /* VDV and VRH Command Enable */                                    \
    ILI9341_CMD(0xC3, 0x12);                 /* VRH Set */                                                       \
    ILI9341_CMD(0xC4, 0x20);                 /* VDV Set */                                                       \
    ILI9341_CMD(0xC6, 0x0F);                 /* Frame Rate Control in Normal Mode (60Hz) */                      \
    ILI9341_CMD(0xD0, 0xA4, 0xA1);           /* Power Control 1 */                                               \
    ILI9341_CMD(0xE0, 0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23);     \
    ILI9341_CMD(0xE1, 0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23);     \
    ILI9341_CMD(0x21);                       /* Display Inversion ON (requerido en ST7789) */

#if RG_SCREEN_DRIVER == RG_DRIVER_ST7789
#define RG_SCREEN_INIT()            RG_SCREEN_INIT_ST7789()
#else
#define RG_SCREEN_INIT()            RG_SCREEN_INIT_ILI9341()
#endif

// ─── 6. Botonera Gamepad (Active LOW con Pull-ups internos) ───
// Pines consensuados: UP=13, DOWN=3, LEFT=1, RIGHT=2, A=41, B=42, SELECT=39, START=40, MENU=38
#define RG_GAMEPAD_GPIO_MAP {                                              \
    {RG_KEY_UP,     .num = GPIO_NUM_13, .pullup = 1, .level = 0},          \
    {RG_KEY_DOWN,   .num = GPIO_NUM_3,  .pullup = 1, .level = 0},          \
    {RG_KEY_LEFT,   .num = GPIO_NUM_1,  .pullup = 1, .level = 0},          \
    {RG_KEY_RIGHT,  .num = GPIO_NUM_2,  .pullup = 1, .level = 0},          \
    {RG_KEY_A,      .num = GPIO_NUM_41, .pullup = 1, .level = 0},          \
    {RG_KEY_B,      .num = GPIO_NUM_42, .pullup = 1, .level = 0},          \
    {RG_KEY_SELECT, .num = GPIO_NUM_39, .pullup = 1, .level = 0},          \
    {RG_KEY_START,  .num = GPIO_NUM_40, .pullup = 1, .level = 0},          \
    {RG_KEY_MENU,   .num = GPIO_NUM_38, .pullup = 1, .level = 0},          \
}

#define RG_RECOVERY_BTN             RG_KEY_MENU

// ─── 7. Batera / LED de Estado ───
#define RG_BATTERY_DRIVER           0   // Desactivado en banco inicial (se puede activar con ADC_UNIT_1)
#define RG_BATTERY_CALC_PERCENT(raw) (100)
#define RG_BATTERY_CALC_VOLTAGE(raw) (4.2f)
