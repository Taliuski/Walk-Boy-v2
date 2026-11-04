# Component CMakeLists.txt for PN532 I2C & NFC Cartridge Engine
# Walk-Boy V2 Subsystem

idf_component_register(
    SRCS
        "pn532_i2c.c"
        "nfc_cartridge.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        driver
        retro-go
        fatfs
)
