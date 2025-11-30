#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// allgemeine Includes
#include <esp_log.h>
#include <esp_err.h>
#include <hal/gpio_types.h>
#include <driver/gpio.h>
#include "connect_wifi.h"
#include "httprequest.h"

// für elektrischen Widerstand
#include "sensor_resistance.h"
int adc_res_raw = 0;
int adc_res_voltage = 0;
uint32_t resistance = 0;

// für ethanol Messung
#include "sensor_ethanol.h"
int adc_eth_raw = 0;
float adc_eth_voltage = 0.0;
uint32_t adc_eth_ppm = 0;

// für ADC - weil beide nutzen den ADC1
adc_oneshot_unit_handle_t adc1_handle;

// für co2 Sensor
#include "sensor_co2.h"
i2c_dev_t dev = {0};
uint32_t adc_co2_ppm = 0;

// für temp Sensor
#include "sensor_temp.h"
uint32_t temp_obj = 0;
uint32_t temp_amb = 0;

// datenbank verbindung
#include "connect_database.h"

// Semaphoren
SemaphoreHandle_t sema_adc = NULL;

// Settings für Messung
char *messungsname = "XXXXXX_XXXX_breadbuddy_prototyp";
const int messungsDelay = 5000;

void app_main(void)
{
    // Semaphoren
    sema_adc = xSemaphoreCreateBinary();

    if (sema_adc == NULL)
    {
        ESP_LOGE("MAIN", "Failed to create ADC semaphore");
        return;
    }

    // Binary semaphore starts empty, must give it once to make it available
    xSemaphoreGive(sema_adc);

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // TODO: das ist da Test-Wifi Paket
    connect_wifi();

    if (wifi_connect_status)
    {
    }

    // initialisierung des ADC
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // i2c init
    init_co2_sensor();

    xTaskCreate(resistance_task, "resistance", 3072, NULL, 5, NULL);
    xTaskCreate(co2_task, "co2", 3072, NULL, 5, NULL);
    xTaskCreate(ethanol_task, "ethanol", 3072, NULL, 5, NULL);
    xTaskCreate(temp_task, "temp", 3072, NULL, 5, NULL);
    // xTaskCreate(database_task, "database", 4096, NULL, 5, NULL);
}
