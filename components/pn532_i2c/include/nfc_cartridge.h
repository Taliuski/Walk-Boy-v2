// ─── NFC Cartridge Subsystem (Walk-Boy V2) ───
// Responsabilidad: Detección y arranque de cartuchos/cassettes NFC físicos para Retro-Go.
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NFC_CATALOG_SD_PATH_1   "/sd/nfc_catalog.csv"
#define NFC_CATALOG_SD_PATH_2   "/sdcard/nfc_catalog.csv"

/**
 * @brief Estructura que representa una entrada del catálogo de cartuchos NFC.
 */
typedef struct {
    char uid[32];       // Identificador único del tag (Hexadecimal, ej: "04A1B2C3D4E5F6")
    char tipo[16];      // Sistema emulador o medio (NES, GB, GBC, SMS, PCE, DOOM, MUSIC, etc.)
    char ruta[256];     // Ruta absoluta al archivo ROM o lista en la tarjeta MicroSD
} nfc_cartridge_entry_t;

/**
 * @brief Inicializa el subsistema NFC y comprueba la comunicación con el PN532.
 * @return true si el hardware PN532 respondió correctamente, false en caso contrario.
 */
bool nfc_cartridge_init(void);

/**
 * @brief Escanea la presencia de un cartucho NFC con un tiempo de espera no bloqueante.
 * @param out_uid Buffer donde se almacenará la cadena UID en mayúsculas.
 * @param max_len Tamaño máximo del buffer out_uid.
 * @param timeout_ms Milisegundos de espera para la detección.
 * @return true si se detectó un cartucho y se leyó su UID, false en caso contrario.
 */
bool nfc_cartridge_scan_card(char *out_uid, size_t max_len, uint32_t timeout_ms);

/**
 * @brief Busca un UID en el archivo de catálogo /sd/nfc_catalog.csv.
 * @param uid Cadena UID a buscar (coincidencia insensible a mayúsculas/minúsculas).
 * @param entry Puntero a la estructura donde se volcarán los datos encontrados.
 * @return true si se encontró coincidencia en el catálogo, false si no existe o hay error de lectura.
 */
bool nfc_cartridge_lookup(const char *uid, nfc_cartridge_entry_t *entry);

/**
 * @brief Inicia la ejecución de una ROM o elemento multimedia mapeando su extensión al core Retro-Go correspondiente.
 * @param path Ruta absoluta del archivo ROM en la tarjeta MicroSD (ej: "/sd/roms/nes/mario.nes").
 * @return true si se pudo transferir el control al emulador, false si la ruta es inválida.
 */
bool rg_launcher_start_game(const char *path);

/**
 * @brief Bucle principal de arranque (Boot Cartridge Loop).
 * Muestra el cartel retro "WALK-BOY: POR FAVOR INSERTA UN CARTUCHO / CASSETTE" y
 * entra en escaneo continuo hasta detectar un cartucho físico o pulsar MENU para ir al launcher.
 */
void nfc_cartridge_boot_loop(void);

#ifdef __cplusplus
}
#endif
