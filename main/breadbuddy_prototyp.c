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
uint32_t last_resistance = 0;
time_t knickpunkt; // besseren Name finden
bool knickpunkt_erreicht = false;

// für ethanol Messung
#include "sensor_ethanol.h"
int adc_eth_raw = 0;
float adc_eth_voltage = 0.0;
uint32_t adc_eth_ppm = 0;

// für ADC - weil beide nutzen den ADC1
adc_oneshot_unit_handle_t adc1_handle;

// für co2 Sensor
#include "sensor_co2.h"
i2c_dev_t co2_dev = {0};
uint32_t co2_ppm = 0;
float temp_amb_co2 = 0;
float humidity = 0.0;
#define I2C_MASTER_SCL GPIO_NUM_1
#define I2C_MASTER_SDA GPIO_NUM_2
#define I2C_PORT 0
i2c_config_t i2c_cfg;

// für temp Sensor
#include "sensor_temp.h"
i2c_dev_t temp_dev = {0};
float temp_obj = 0.0;
float temp_amb = 0.0;

// für PH-Werte
float ph_value = 0.0;
float last_ph_value = 0.0;
ph_t ph_array[32] = {0};

// datenbank verbindung
#include "connect_database.h"
#include "probe_data.h"
probe_data_t probendaten;
char notizen[1024];

// webserver
#include "webserver.h"
char bake_rest_time[64] = "---";
uint32_t time_remain = 25200; // 7 Stunden ab Knickpunkt (wo Teig beginnt sich auszudehnen)
char bake_time[64] = "---";
time_t time_ready = 0;
char last_bake_time[64] = "---";
bool baking = false;

// timeserver
#include "current_time.h"
char measurement_id[64] = "---";
char measurement_start_time[64] = "---";
char measurement_start_date[64] = "---";
// TODO: Beendung der Messung müssen diese Werte befüllen!!
char measurement_stop_time[64] = "---";
char measurement_stop_date[64] = "---";
char measurement_duration[64] = "---";
char elapsed_time_str[32] = "---";
time_t start_time;
time_t stop_time;
time_t duration;

// Dataanalysis
#include "analysis.h"
Dataset_int dataset_resistance;
Dataset_int dataset_co2;
Dataset_float dataset_temperature;
Phase phase = INITIAL;
Measurement_state state;

// Semaphoren
SemaphoreHandle_t sema_measurement = NULL;

// Settings für Messung
char messungsname[64];
const int messungsDelay = 900000; // 15 * 60 * 1000 = 900.000

void app_main(void)
{
    // Semaphoren
    sema_measurement = xSemaphoreCreateBinary();

    if (sema_measurement == NULL)
    {
        ESP_LOGE("MAIN", "Failed to create semaphore for measurement");
        return;
    }

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
        initialize_sntp();
    }

    // initialisierung des ADC
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    /*
    // *** I2C BUS RESET - 9 CLOCK PULSES ***
    ESP_LOGI("MAIN", "Resetting I2C bus with 9 clock pulses...");

    // Vor i2c_driver_install() aufrufen:
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << I2C_MASTER_SCL),
        .pull_up_en = GPIO_PULLUP_ENABLE};
        gpio_config(&io_conf);

        for (int i = 0; i < 9; i++)
        {
            gpio_set_level(I2C_MASTER_SCL, 0);
            esp_rom_delay_us(10);
            gpio_set_level(I2C_MASTER_SCL, 1);
            esp_rom_delay_us(10);
        }

        ESP_LOGI("MAIN", "I2C bus reset completed, SDA level: %d", gpio_get_level(I2C_MASTER_SDA));
        vTaskDelay(pdMS_TO_TICKS(100));
    */

    // *** ZENTRALE I2C-INITIALISIERUNG ***
    ESP_LOGI("MAIN", "Initializing I2C bus...");
    ESP_ERROR_CHECK(i2cdev_init());
    ESP_LOGI("MAIN", "i2cdev library initialized");

    xTaskCreate(resistance_task, "resistance", 3072, NULL, 5, NULL);
    xTaskCreate(co2_task, "co2", 3072, NULL, 5, NULL);
    xTaskCreate(ethanol_task, "ethanol", 3072, NULL, 5, NULL);
    //   xTaskCreate(temp_task, "temp", 3072, NULL, 5, NULL);
    xTaskCreate(database_task, "database", 4096, NULL, 5, NULL);
    xTaskCreate(webserver_task, "webserver", 16384, NULL, 5, NULL);
    xTaskCreate(dataanalysis_task, "analysis", 4096, NULL, 5, NULL);
}
