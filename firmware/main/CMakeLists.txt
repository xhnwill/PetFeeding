idf_component_register(SRCS "main.c" "pet.c" "board.c" "radio.c" "network.c"
 INCLUDE_DIRS "." REQUIRES driver esp_adc esp_wifi esp_event esp_netif esp_http_server app_update esp_https_ota nvs_flash bt json esp_timer esp_driver_gpio esp_driver_spi mbedtls
 EMBED_TXTFILES "../../web/index.html" "../../web/app.js" "../../web/style.css")
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wno-unused-parameter)
