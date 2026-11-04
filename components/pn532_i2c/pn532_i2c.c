// ─── PN532 I2C Driver Implementation (Walk-Boy V2) ───
// Maneja la comunicacin de paquetes por I2C con el transceptor NFC PN532.

#include "pn532_i2c.h"
#include <string.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

static const char *TAG = "PN532_I2C";

// ─── Constantes del Protocolo PN532 ───
#define PN532_PREAMBLE                  (0x00)
#define PN532_STARTCODE1                (0x00)
#define PN532_STARTCODE2                (0xFF)
#define PN532_POSTAMBLE                 (0x00)
#define PN532_HOSTTOPN532               (0xD4)
#define PN532_PN532TOHOST               (0xD5)
#define PN532_ACK_WAIT_TIME_MS          (50)

static const uint8_t pn532_ack[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
static bool s_i2c_driver_installed = false;
static i2c_port_t s_i2c_port = PN532_I2C_PORT;

// ─── Funciones Auxiliares de Bajo Nivel I2C ───

/**
 * @brief Lee el byte de estado de preparación del PN532 (0x01 = listo para transmitir datos).
 */
static bool pn532_is_ready(void)
{
    uint8_t status = 0;
    esp_err_t err = i2c_master_read_from_device(s_i2c_port, PN532_I2C_ADDRESS, &status, 1, pdMS_TO_TICKS(10));
    return (err == ESP_OK && (status & 0x01) == 0x01);
}

/**
 * @brief Espera a que el PN532 devuelva el estado Ready (1) dentro del tiempo límite.
 */
static bool pn532_wait_ready(uint32_t timeout_ms)
{
    TickType_t start = xTaskGetTickCount();
    TickType_t wait_ticks = pdMS_TO_TICKS(timeout_ms);

    while ((xTaskGetTickCount() - start) < wait_ticks)
    {
        if (pn532_is_ready())
            return true;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return false;
}

/**
 * @brief Envía un paquete con el formato estándar PN532 (Preamble, Start, Len, LCS, Data, DCS, Postamble).
 */
static esp_err_t pn532_write_command(const uint8_t *cmd, size_t cmd_len)
{
    // Tamaño total = Preamble(1) + Start(2) + LEN(1) + LCS(1) + TFI(1) + cmd_len + DCS(1) + Postamble(1)
    uint8_t buffer[cmd_len + 8];
    size_t idx = 0;

    buffer[idx++] = PN532_PREAMBLE;
    buffer[idx++] = PN532_STARTCODE1;
    buffer[idx++] = PN532_STARTCODE2;

    uint8_t length = cmd_len + 1; // cmd_len + TFI
    buffer[idx++] = length;
    buffer[idx++] = (~length + 1) & 0xFF; // LCS Checksum

    buffer[idx++] = PN532_HOSTTOPN532;
    uint8_t sum = PN532_HOSTTOPN532;

    for (size_t i = 0; i < cmd_len; i++)
    {
        buffer[idx++] = cmd[i];
        sum += cmd[i];
    }

    buffer[idx++] = (~sum + 1) & 0xFF; // DCS Checksum
    buffer[idx++] = PN532_POSTAMBLE;

    esp_err_t err = i2c_master_write_to_device(s_i2c_port, PN532_I2C_ADDRESS, buffer, idx, pdMS_TO_TICKS(100));
    if (err != ESP_OK)
    {
        ESP_LOGD(TAG, "Fallo al escribir comando a PN532: %d", err);
    }
    return err;
}

/**
 * @brief Lee y verifica el paquete ACK (00 00 FF 00 FF 00) del PN532.
 */
static bool pn532_read_ack(void)
{
    if (!pn532_wait_ready(PN532_ACK_WAIT_TIME_MS))
        return false;

    // Se leen 7 bytes: byte 0 = estado (0x01), bytes 1..6 = ACK frame
    uint8_t ack_buff[7];
    esp_err_t err = i2c_master_read_from_device(s_i2c_port, PN532_I2C_ADDRESS, ack_buff, sizeof(ack_buff), pdMS_TO_TICKS(50));
    if (err != ESP_OK)
        return false;

    return (memcmp(&ack_buff[1], pn532_ack, sizeof(pn532_ack)) == 0);
}

/**
 * @brief Lee la trama de respuesta de datos del PN532 y verifica sus sumas de comprobación.
 */
static int pn532_read_response(uint8_t *response, size_t max_len, uint32_t timeout_ms)
{
    if (!pn532_wait_ready(timeout_ms))
        return -1;

    // Buffer temporal para leer el encabezado: 1(ready) + 3(preamble/start) + 2(len/lcs) + 1(TFI)
    uint8_t raw_header[7];
    esp_err_t err = i2c_master_read_from_device(s_i2c_port, PN532_I2C_ADDRESS, raw_header, sizeof(raw_header), pdMS_TO_TICKS(50));
    if (err != ESP_OK)
        return -2;

    if (raw_header[1] != PN532_PREAMBLE || raw_header[2] != PN532_STARTCODE1 || raw_header[3] != PN532_STARTCODE2)
    {
        ESP_LOGD(TAG, "Encabezado PN532 invalido");
        return -3;
    }

    uint8_t length = raw_header[4];
    uint8_t lcs = raw_header[5];
    if (((length + lcs) & 0xFF) != 0)
    {
        ESP_LOGD(TAG, "LCS checksum incorrecto");
        return -4;
    }

    if (raw_header[6] != PN532_PN532TOHOST)
    {
        ESP_LOGD(TAG, "TFI no corresponde a PN532TOHOST");
        return -5;
    }

    // Datos restantes a leer = length - 1 (TFI ya ledo) + 1(DCS) + 1(Postamble)
    uint8_t payload_len = length - 1;
    if (payload_len > max_len)
        payload_len = max_len;

    uint8_t trailing_bytes[payload_len + 2];
    err = i2c_master_read_from_device(s_i2c_port, PN532_I2C_ADDRESS, trailing_bytes, sizeof(trailing_bytes), pdMS_TO_TICKS(50));
    if (err != ESP_OK)
        return -6;

    memcpy(response, trailing_bytes, payload_len);
    return (int)payload_len;
}

// ─── Funciones Públicas Exportadas ───

esp_err_t pn532_i2c_init(gpio_num_t sda_pin, gpio_num_t scl_pin)
{
    if (s_i2c_driver_installed)
        return ESP_OK;

    const i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = PN532_I2C_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(s_i2c_port, &conf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error configurando parametros I2C: %d", err);
        return err;
    }

    err = i2c_driver_install(s_i2c_port, I2C_MODE_MASTER, 0, 0, 0);
    if (err == ESP_OK)
    {
        s_i2c_driver_installed = true;
        ESP_LOGI(TAG, "Bus I2C inicializado en SDA=%d, SCL=%d", sda_pin, scl_pin);
    }
    else if (err == ESP_ERR_INVALID_STATE)
    {
        // El driver ya estaba instalado previamente por el sistema base
        s_i2c_driver_installed = true;
        err = ESP_OK;
    }

    return err;
}

esp_err_t pn532_i2c_deinit(void)
{
    if (!s_i2c_driver_installed)
        return ESP_OK;

    esp_err_t err = i2c_driver_delete(s_i2c_port);
    if (err == ESP_OK)
        s_i2c_driver_installed = false;

    return err;
}

esp_err_t pn532_sam_configuration(void)
{
    // Comando SAMConfiguration: 0x14, Modo Normal: 0x01, Timeout: 0x14 (1s), Use IRQ: 0x01
    const uint8_t cmd[] = {PN532_COMMAND_SAMCONFIGURATION, 0x01, 0x14, 0x01};

    esp_err_t err = pn532_write_command(cmd, sizeof(cmd));
    if (err != ESP_OK)
        return err;

    if (!pn532_read_ack())
    {
        ESP_LOGW(TAG, "No se recibio ACK al configurar SAM");
        return ESP_FAIL;
    }

    uint8_t resp[8];
    int len = pn532_read_response(resp, sizeof(resp), 100);
    if (len < 1 || resp[0] != (PN532_COMMAND_SAMCONFIGURATION + 1))
    {
        ESP_LOGW(TAG, "Respuesta SAMConfiguration invalida");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "PN532 SAM configurado en Modo Normal con exito");
    return ESP_OK;
}

esp_err_t pn532_get_firmware_version(uint32_t *version)
{
    const uint8_t cmd[] = {PN532_COMMAND_GETFIRMWAREVERSION};

    esp_err_t err = pn532_write_command(cmd, sizeof(cmd));
    if (err != ESP_OK)
        return err;

    if (!pn532_read_ack())
        return ESP_FAIL;

    uint8_t resp[8];
    int len = pn532_read_response(resp, sizeof(resp), 100);
    if (len < 5 || resp[0] != (PN532_COMMAND_GETFIRMWAREVERSION + 1))
        return ESP_FAIL;

    if (version)
    {
        *version = ((uint32_t)resp[1] << 24) |
                   ((uint32_t)resp[2] << 16) |
                   ((uint32_t)resp[3] << 8)  |
                   ((uint32_t)resp[4]);
    }

    ESP_LOGI(TAG, "PN532 detectado - Chip IC: 0x%02X, Firmware Ver: %d.%d", resp[1], resp[2], resp[3]);
    return ESP_OK;
}

bool pn532_read_passive_target_id(uint8_t card_baudrate, uint8_t *uid, uint8_t *uid_len, uint32_t timeout_ms)
{
    // Comando InListPassiveTarget: MaxTargets=1, BaudRate=card_baudrate
    const uint8_t cmd[] = {PN532_COMMAND_INLISTPASSIVETARGET, 0x01, card_baudrate};

    esp_err_t err = pn532_write_command(cmd, sizeof(cmd));
    if (err != ESP_OK)
        return false;

    if (!pn532_read_ack())
        return false;

    // Buffer de respuesta para InListPassiveTarget
    uint8_t resp[32];
    int len = pn532_read_response(resp, sizeof(resp), timeout_ms);
    if (len < 7)
        return false;

    // resp[0] = CMD+1 (0x4B)
    // resp[1] = NbTg (Numero de tarjetas encontradas)
    if (resp[0] != (PN532_COMMAND_INLISTPASSIVETARGET + 1) || resp[1] != 1)
        return false;

    // Estructura de respuesta para ISO14443A:
    // resp[2] = Tg (1)
    // resp[3,4] = SENS_RES
    // resp[5] = SEL_RES
    // resp[6] = NFCIDLength
    uint8_t id_len = resp[6];
    if (id_len > 7 || (len < (7 + id_len)))
        return false;

    if (uid && uid_len)
    {
        *uid_len = id_len;
        memcpy(uid, &resp[7], id_len);
    }

    return true;
}

void pn532_format_uid_string(const uint8_t *uid, uint8_t uid_len, char *out_str, size_t out_len)
{
    if (!uid || !out_str || out_len == 0)
        return;

    size_t written = 0;
    for (uint8_t i = 0; i < uid_len && (written + 3) <= out_len; i++)
    {
        written += snprintf(&out_str[written], out_len - written, "%02X", uid[i]);
    }
    out_str[written] = '\0';
}
