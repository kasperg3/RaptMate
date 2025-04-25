idf_component_register(SRCS "main.cpp" "src/RaptMateServer.cpp" "src/RaptPillBLE.cpp" INCLUDE_DIRS "." "src" REQUIRES bt nvs_flash spiffs esp_http_server json)
set(CONFIG_BT_NIMBLE_ENABLED 1)  # Enable NimBLE stack

set(COMPONENT_REQUIRES bt nvs_flash spiffs esp_http_server json)

# Build react spiffs image
add_custom_target(dep
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/rapt-mate
    COMMAND npm run build
    COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_SOURCE_DIR}/rapt-mate/build/static/js/*LICENSE.txt # Remove the license files, they are too long for the spiffs
)

set(PARTITION_TABLE ${CMAKE_SOURCE_DIR}/partitions.csv)

spiffs_create_partition_image(
    storage 
    ${CMAKE_SOURCE_DIR}/rapt-mate/build/ 
    FLASH_IN_PROJECT 
    PARTITION_TABLE_FILE ${PARTITION_TABLE}
    DEPENDS dep
)

spiffs_create_partition_image(
    data 
    ${CMAKE_SOURCE_DIR}/rapt-mate/build/ 
    FLASH_IN_PROJECT 
    PARTITION_TABLE_FILE ${PARTITION_TABLE}
    DEPENDS dep
)