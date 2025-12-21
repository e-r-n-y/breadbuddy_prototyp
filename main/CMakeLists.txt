idf_component_register(
    SRCS
        "breadbuddy_prototyp.c"
        "measurement_timing.c"
        "sensor_resistance.c"
        "sensor_ethanol.c"
        "sensor_temp.c"
        "sensor_co2.c"
        "connect_database.c"  
        "httprequest.c"
        "connect_wifi.c"
        "webserver.c"
        "current_time.c"
        "probe_data.c"
    INCLUDE_DIRS 
        "."     
    PRIV_REQUIRES 
        esp_adc
        esp_driver_gpio
        nvs_flash
        esp_wifi
        driver

        esp_http_server
        spi_flash
        spiffs

        json
)

spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)
                       
