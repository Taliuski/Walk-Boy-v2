# Target definition for Walk-Boy V2 (ESP32-S3-DevKitC-1 N16R8)
# Retro-Go Engine Custom Target

cmake_minimum_required(VERSION 3.16)

message(STATUS "Configuring Walk-Boy V2 Target (ESP32-S3-N16R8)")

# Export target include directory and build definition
target_compile_definitions(${COMPONENT_LIB} PUBLIC
    -DRG_TARGET_WALKBOY=1
)

target_include_directories(${COMPONENT_LIB} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
