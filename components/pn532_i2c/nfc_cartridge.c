// ─── NFC Cartridge Logic Implementation (Walk-Boy V2) ───
// Maneja el bucle de espera de cartuchos fsicos al arranque y el mapeo a Retro-Go.

#include "nfc_cartridge.h"
#include "pn532_i2c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

// Cabeceras de Retro-Go
#include <rg_system.h>
#include <rg_display.h>
#include <rg_gui.h>
#include <rg_input.h>

static const char *TAG = "NFC_CARTRIDGE";

// ─── Inicializacin ───

bool nfc_cartridge_init(void)
{
    ESP_LOGI(TAG, "Inicializando transceptor PN532 en bus I2C (SDA=21, SCL=47)...");
    esp_err_t err = pn532_i2c_init(PN532_DEFAULT_SDA_PIN, PN532_DEFAULT_SCL_PIN);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "No se pudo iniciar el bus I2C de PN532 (err=%d)", err);
        return false;
    }

    uint32_t fw_version = 0;
    err = pn532_get_firmware_version(&fw_version);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "PN532 no responde al comando de version. Compruebe SW1=ON, SW2=OFF.");
        return false;
    }

    err = pn532_sam_configuration();
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Fallo al configurar SAM en PN532.");
        return false;
    }

    ESP_LOGI(TAG, "Subsistema NFC inicializado y listo para detectar cartuchos.");
    return true;
}

// ─── Escaneo de Tarjetas ───

bool nfc_cartridge_scan_card(char *out_uid, size_t max_len, uint32_t timeout_ms)
{
    if (!out_uid || max_len < 9)
        return false;

    uint8_t uid_bytes[8];
    uint8_t uid_len = 0;

    bool found = pn532_read_passive_target_id(PN532_MIFARE_ISO14443A, uid_bytes, &uid_len, timeout_ms);
    if (found && uid_len > 0)
    {
        pn532_format_uid_string(uid_bytes, uid_len, out_uid, max_len);
        return true;
    }

    return false;
}

// ─── Bsqueda en Catlogo MicroSD (/sd/nfc_catalog.csv) ───

static void trim_whitespace(char *str)
{
    if (!str) return;
    char *end = str + strlen(str) - 1;
    while (end >= str && (*end == '\r' || *end == '\n' || isspace((unsigned char)*end)))
    {
        *end = '\0';
        end--;
    }
}

bool nfc_cartridge_lookup(const char *uid, nfc_cartridge_entry_t *entry)
{
    if (!uid || !entry)
        return false;

    FILE *fp = fopen(NFC_CATALOG_SD_PATH_1, "r");
    if (!fp)
    {
        fp = fopen(NFC_CATALOG_SD_PATH_2, "r");
    }

    if (!fp)
    {
        ESP_LOGW(TAG, "No se encontro el archivo de catalogo en /sd/nfc_catalog.csv ni /sdcard/nfc_catalog.csv");
        return false;
    }

    char line[384];
    bool found = false;

    while (fgets(line, sizeof(line), fp))
    {
        trim_whitespace(line);

        // Omitir lineas vacias o comentarios
        if (line[0] == '\0' || line[0] == '#')
            continue;

        // Omitir cabecera
        if (strncasecmp(line, "UID;", 4) == 0)
            continue;

        // Separar campos por punto y coma (UID;TIPO;RUTA)
        char *token_uid = strtok(line, ";");
        char *token_tipo = strtok(NULL, ";");
        char *token_ruta = strtok(NULL, ";");

        if (!token_uid || !token_tipo || !token_ruta)
            continue;

        if (strcasecmp(token_uid, uid) == 0)
        {
            strncpy(entry->uid, token_uid, sizeof(entry->uid) - 1);
            entry->uid[sizeof(entry->uid) - 1] = '\0';

            strncpy(entry->tipo, token_tipo, sizeof(entry->tipo) - 1);
            entry->tipo[sizeof(entry->tipo) - 1] = '\0';

            strncpy(entry->ruta, token_ruta, sizeof(entry->ruta) - 1);
            entry->ruta[sizeof(entry->ruta) - 1] = '\0';

            found = true;
            break;
        }
    }

    fclose(fp);
    return found;
}

// ─── Lanzador de Juegos Retro-Go ───

bool rg_launcher_start_game(const char *path)
{
    if (!path || strlen(path) == 0)
    {
        ESP_LOGE(TAG, "Ruta de ROM nula o vacia");
        return false;
    }

    ESP_LOGI(TAG, "Lanzando cartucho desde ruta: %s", path);

    // Obtener extensin del archivo
    const char *dot = strrchr(path, '.');
    char ext[16] = {0};
    if (dot)
    {
        size_t i = 0;
        for (const char *p = dot + 1; *p && i < (sizeof(ext) - 1); p++, i++)
        {
            ext[i] = tolower((unsigned char)*p);
        }
        ext[i] = '\0';
    }

    const char *partition = "retro-core";
    const char *app_name = "nes";

    if (strcmp(ext, "nes") == 0 || strcmp(ext, "fc") == 0 || strcmp(ext, "fds") == 0)
    {
        partition = "retro-core";
        app_name = "nes";
    }
    else if (strcmp(ext, "gb") == 0)
    {
        partition = "retro-core";
        app_name = "gb";
    }
    else if (strcmp(ext, "gbc") == 0)
    {
        partition = "retro-core";
        app_name = "gbc";
    }
    else if (strcmp(ext, "sms") == 0 || strcmp(ext, "sg") == 0)
    {
        partition = "retro-core";
        app_name = "sms";
    }
    else if (strcmp(ext, "gg") == 0)
    {
        partition = "retro-core";
        app_name = "gg";
    }
    else if (strcmp(ext, "pce") == 0)
    {
        partition = "retro-core";
        app_name = "pce";
    }
    else if (strcmp(ext, "wad") == 0)
    {
        partition = "prboom-go";
        app_name = "doom";
    }
    else if (strcmp(ext, "md") == 0 || strcmp(ext, "gen") == 0 || strcmp(ext, "bin") == 0)
    {
        partition = "gwenesis";
        app_name = "md";
    }
    else if (strcmp(ext, "rom") == 0 || strcmp(ext, "mx1") == 0 || strcmp(ext, "mx2") == 0)
    {
        partition = "fmsx";
        app_name = "msx";
    }

    // Efectuar el cambio de aplicacin en Retro-Go
    rg_system_switch_app(partition, app_name, path, 0);
    return true;
}

// ─── Bucle de Arranque (Boot Cartridge Loop) ───

static void draw_boot_ui(void)
{
    rg_display_clear(C_BLACK);

    // Marco exterior retro estilo Walk-Boy
    rg_gui_draw_rect(10, 10, 300, 220, 2, C_DODGER_BLUE, C_MIDNIGHT_BLUE);
    rg_gui_draw_rect(14, 14, 292, 212, 1, C_STEEL_BLUE, C_MIDNIGHT_BLUE);

    // Encabezado
    rg_gui_draw_text(20, 25, 280, "WALK-BOY V2", C_GOLD, C_MIDNIGHT_BLUE,
                     RG_TEXT_ALIGN_CENTER | RG_TEXT_BIGGER);

    // Separador
    rg_gui_draw_rect(30, 48, 260, 2, 1, C_DEEP_SKY_BLUE, C_DEEP_SKY_BLUE);

    // Aviso principal consensuado
    rg_gui_draw_text(20, 65, 280, "POR FAVOR INSERTA UN\nCARTUCHO / CASSETTE", C_WHITE, C_MIDNIGHT_BLUE,
                     RG_TEXT_ALIGN_CENTER | RG_TEXT_MULTILINE);

    // Grfico representativo de cartucho retro / cassette
    rg_gui_draw_rect(100, 115, 120, 50, 2, C_GRAY, C_BLACK);
    rg_gui_draw_rect(115, 128, 90, 24, 1, C_DARK_GRAY, C_DIM_GRAY);
    rg_gui_draw_text(115, 133, 90, "NFC TAG", C_CYAN, C_DIM_GRAY, RG_TEXT_ALIGN_CENTER);

    // Instruccin inferior para saltar al launcher
    rg_gui_draw_text(20, 185, 280, "Acerque su tag NFC...", C_SPRING_GREEN, C_MIDNIGHT_BLUE,
                     RG_TEXT_ALIGN_CENTER);
    rg_gui_draw_text(20, 203, 280, "Pulsa [MENU] para Launcher Clasico", C_SILVER, C_MIDNIGHT_BLUE,
                     RG_TEXT_ALIGN_CENTER);
}

void nfc_cartridge_boot_loop(void)
{
    ESP_LOGI(TAG, "Iniciando Boot Cartridge Loop...");

    // Inicializar lector PN532
    bool nfc_ok = nfc_cartridge_init();
    if (!nfc_ok)
    {
        ESP_LOGW(TAG, "Lector PN532 no disponible en este arranque. Saltando al launcher clasico...");
        return;
    }

    draw_boot_ui();

    char uid_str[32];
    nfc_cartridge_entry_t entry;
    uint32_t loop_count = 0;

    while (1)
    {
        // 1. Comprobar pulsacin del botn MENU para salir al launcher visual tradicional
        uint32_t keys = rg_input_read_gamepad();
        if (keys & RG_KEY_MENU)
        {
            ESP_LOGI(TAG, "Boton MENU pulsado por el usuario. Saliendo al Launcher clasico...");
            rg_gui_draw_rect(30, 175, 260, 40, 1, C_ORANGE, C_BLACK);
            rg_gui_draw_text(30, 185, 260, "Abriendo Launcher clasico...", C_YELLOW, C_BLACK, RG_TEXT_ALIGN_CENTER);
            vTaskDelay(pdMS_TO_TICKS(250));
            break;
        }

        // 2. Indicador dinmico de escaneo (parpadeo sutil cada segundo)
        loop_count++;
        if ((loop_count % 8) == 0)
        {
            rg_gui_draw_text(20, 185, 280, "Acerque su tag NFC...",
                             (loop_count % 16 == 0) ? C_SPRING_GREEN : C_DARK_GREEN,
                             C_MIDNIGHT_BLUE, RG_TEXT_ALIGN_CENTER);
        }

        // 3. Escaneo no bloqueante del PN532
        if (nfc_cartridge_scan_card(uid_str, sizeof(uid_str), 120))
        {
            ESP_LOGI(TAG, "Cartucho detectado con UID: %s", uid_str);

            // Mostrar retroalimentacin inmediata en pantalla
            rg_gui_draw_rect(20, 110, 280, 75, 2, C_SPRING_GREEN, C_BLACK);
            rg_gui_draw_text(25, 118, 270, "CARTUCHO DETECTADO!", C_SPRING_GREEN, C_BLACK, RG_TEXT_ALIGN_CENTER);

            char msg_uid[64];
            snprintf(msg_uid, sizeof(msg_uid), "UID: %s", uid_str);
            rg_gui_draw_text(25, 138, 270, msg_uid, C_YELLOW, C_BLACK, RG_TEXT_ALIGN_CENTER);

            // 4. Buscar coincidencia en el catlogo de la tarjeta MicroSD
            if (nfc_cartridge_lookup(uid_str, &entry))
            {
                char msg_game[96];
                snprintf(msg_game, sizeof(msg_game), "Cargando: [%s]", entry.tipo);
                rg_gui_draw_text(25, 158, 270, msg_game, C_WHITE, C_BLACK, RG_TEXT_ALIGN_CENTER);

                vTaskDelay(pdMS_TO_TICKS(600));

                // 5. Arrancar el juego mediante rg_launcher_start_game
                rg_launcher_start_game(entry.ruta);
                // Si la ejecucin retorna por algn error en la ruta:
                ESP_LOGE(TAG, "No se pudo transferir control a la ROM: %s", entry.ruta);
            }
            else
            {
                rg_gui_draw_text(25, 158, 270, "No registrado en nfc_catalog.csv", C_RED, C_BLACK, RG_TEXT_ALIGN_CENTER);
                ESP_LOGW(TAG, "El UID %s no existe en nfc_catalog.csv", uid_str);
                vTaskDelay(pdMS_TO_TICKS(1800));
                draw_boot_ui();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
