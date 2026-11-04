// ─── PN532 I2C Driver for ESP-IDF / ESP32-S3 (Walk-Boy V2) ───
// Responsabilidad: Control por bajo nivel del transceptor NFC PN532 mediante bus I2C.
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <esp_err.h>
#include <driver/gpio.h>
#include <driver/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

// Direccin I2C por defecto de PN532 (7 bits)
#define PN532_I2C_ADDRESS               (0x24)

// Pines I2C fijados en el consenso de Walk-Boy V2
#define PN532_DEFAULT_SDA_PIN           GPIO_NUM_21
#define PN532_DEFAULT_SCL_PIN           GPIO_NUM_47
#define PN532_I2C_PORT                  I2C_NUM_0
#define PN532_I2C_FREQ_HZ               100000  // 100 kHz estándar modo I2C PN532

// Códigos de Comando del PN532
#define PN532_COMMAND_DIAGNOSE          (0x00)
#define PN532_COMMAND_GETFIRMWAREVERSION (0x02)
#define PN532_COMMAND_SAMCONFIGURATION  (0x14)
#define PN532_COMMAND_INLISTPASSIVETARGET (0x4A)
#define PN532_COMMAND_INDATAEXCHANGE    (0x40)

// Tipos de modulación de tarjeta para InListPassiveTarget
#define PN532_MIFARE_ISO14443A          (0x00)

/**
 * @brief Inicializa el bus I2C y el controlador PN532.
 * @param sda_pin Pin GPIO para SDA (por defecto GPIO 21)
 * @param scl_pin Pin GPIO para SCL (por defecto GPIO 47)
 * @return ESP_OK si la inicialización fue exitosa, código de error en caso contrario.
 */
esp_err_t pn532_i2c_init(gpio_num_t sda_pin, gpio_num_t scl_pin);

/**
 * @brief Desinicializa el bus I2C y libera los recursos del PN532.
 * @return ESP_OK si se desinicializó correctamente.
 */
esp_err_t pn532_i2c_deinit(void);

/**
 * @brief Configura el Security Access Module (SAM) en modo normal.
 * Es requerido para habilitar la detección de tarjetas pasivas.
 * @return ESP_OK si el comando SAM fue reconocido con éxito.
 */
esp_err_t pn532_sam_configuration(void);

/**
 * @brief Obtiene el número de versión de firmware del chip PN532.
 * @param version Puntero a entero donde se almacenará el firmware version raw.
 * @return ESP_OK si la lectura fue exitosa.
 */
esp_err_t pn532_get_firmware_version(uint32_t *version);

/**
 * @brief Escanea la presencia de una tarjeta/tag pasivo ISO14443A (Mifare / NTAG).
 * @param card_baudrate Baudrate/tipo de modulación (PN532_MIFARE_ISO14443A).
 * @param uid Buffer donde se copiarán los bytes del UID (mínimo 7 bytes).
 * @param uid_len Puntero donde se almacenará la longitud del UID (4 o 7 bytes).
 * @param timeout_ms Tiempo máximo de espera en milisegundos.
 * @return true si se detectó y leyó una tarjeta, false si expiró el tiempo o hubo error.
 */
bool pn532_read_passive_target_id(uint8_t card_baudrate, uint8_t *uid, uint8_t *uid_len, uint32_t timeout_ms);

/**
 * @brief Convierte un buffer de bytes de UID en una cadena hexadecimal en mayúsculas sin separadores.
 * @param uid Puntero al buffer con bytes del UID.
 * @param uid_len Longitud del UID en bytes.
 * @param out_str Buffer de caracteres de destino.
 * @param out_len Tamaño máximo del buffer de destino.
 */
void pn532_format_uid_string(const uint8_t *uid, uint8_t uid_len, char *out_str, size_t out_len);

#ifdef __cplusplus
}
#endif
