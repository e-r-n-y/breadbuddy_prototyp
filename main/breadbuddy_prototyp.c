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

// für temp Sensor
#include "sensor_temp.h"

// Semaphoren
// SemaphoreHandle_t sema_resistance = NULL;
SemaphoreHandle_t sema_adc = NULL;
// SemaphoreHandle_t sema_co2 = NULL;
// SemaphoreHandle_t sema_ethanol = NULL;

// Settings für Messung
static char *messungsname = "XXXXXX_XXXX_breadbuddy_prototyp";
const int messungsDelay = 5000;
// #define MESSUNGSDELAY 5000 // 900000 entsprechen 15 Minuten

// für SCD41 CO2 Sensor
#include <scd4x.h>
#include <i2cdev.h> // Add this include
static const char *TAG_CO2 = "SCD41-CO2";
#define CONFIG_EXAMPLE_I2C_MASTER_SCL GPIO_NUM_13
#define CONFIG_EXAMPLE_I2C_MASTER_SDA GPIO_NUM_14
// SCD41 I2C address according to datasheet
#define SCD41_I2C_ADDRESS 0x62
static i2c_dev_t dev = {0};
static uint32_t adc_co2_ppm = 0;
#define CO2_I2C_PORT 0

/*
// für elektrischen Widerstand
#include "soc/soc_caps.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#define RES_ADC_ATTEN ADC_ATTEN_DB_12
#define RES_ADC_CHANNEL ADC_CHANNEL_3
#define RES_ADC_WIDTH ADC_BITWIDTH_DEFAULT
#define RESISTOR_VALUE_OHMS 6700 // Value of the known resistor in ohms
static const char *TAG_RES = "ADC-Resistance";
*/

// für Ethanolmessung
// #define ETH_ADC_ATTEN ADC_ATTEN_DB_12
// #define ETH_ADC_CHANNEL ADC_CHANNEL_7 // ist Pin 8
// #define ETH_ADC_WIDTH ADC_BITWIDTH_DEFAULT
// static const char *TAG_ETH = "ADC-Ethanol";

// für Tempsensor
/*
#include "mlx90614.h"
#define TEMP_SCL_PIN GPIO_NUM_36
#define TEMP_SDA_PIN GPIO_NUM_35
#define MASTER_FREQUENCY 100000
#define TEMP_I2C_PORT 1
static const char *TAG_TEMP = "MLX90614-Temperatur";
*/

// FUNKTIONEN FÜR WIDERSTANDSMESSUNG
// =================================
/*
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

    #if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated)
    {
        ESP_LOGI(TAG_RES, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }
    #endif

    #if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated)
    {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }
    #endif

    *out_handle = handle;
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG_RES, "Calibration Success");
    }
    else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated)
    {
        ESP_LOGW(TAG_RES, "eFuse not burnt, skip software calibration");
    }
    else
    {
        ESP_LOGE(TAG_RES, "Invalid arg or no memory");
    }

    return calibrated;
}

static void adc_calibration_deinit(adc_cali_handle_t handle)
{
    #if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG_RES, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

    #elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
    #endif
}

void resistance_task(void *pvParameters)
{
    // GPIO Config for sensor power control
    ESP_ERROR_CHECK(gpio_config(&(gpio_config_t){
        .pin_bit_mask = (1ULL << 6),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    }));
    gpio_dump_io_configuration(stdout, (1ULL << 6));
    ESP_ERROR_CHECK(gpio_set_level(6, 0));

    // ADC1 CONFIG
    // ==============================

    adc_oneshot_chan_cfg_t config = {
        .atten = RES_ADC_ATTEN,
        .bitwidth = RES_ADC_WIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, RES_ADC_CHANNEL, &config));

    //-------------ADC1 Calibration Init---------------//
    adc_cali_handle_t adc1_cali_chan3_handle = NULL;
    bool do_calibration1_chan3 = adc_calibration_init(ADC_UNIT_1, RES_ADC_CHANNEL, RES_ADC_ATTEN, &adc1_cali_chan3_handle);

    while (1)
    {
        if (xSemaphoreTake(sema_adc, (TickType_t)10) == pdTRUE)
        {
            // Stack-Nutzung prüfen
            UBaseType_t stackRemaining = uxTaskGetStackHighWaterMark(NULL);
            ESP_LOGW(TAG_RES, "Stack remaining: %u words", stackRemaining);

            // WIDERSTANDSMESSUNG
            // ==============================
            ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)6, 1));

            vTaskDelay(pdMS_TO_TICKS(5000)); // wait for sensor to stabilize

            int sum = 0;
            for (int i = 0; i < 10; i++)
            {
                ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, RES_ADC_CHANNEL, &adc_res_raw));
                sum += adc_res_raw;
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            adc_res_raw = sum / 10;

            ESP_LOGI(TAG_RES, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, RES_ADC_CHANNEL, adc_res_raw);

            // adc_res_voltage = adc_res_raw * 1750 / 4095; // Convert raw to voltage in mV for 12-bit ADC
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan3_handle, adc_res_raw, &adc_res_voltage));
            adc_res_voltage = (int)(adc_res_voltage * 3300.0 / 3260.0); // adjust for voltage divider
            ESP_LOGI(TAG_RES, "ADC%d Channel[%d] Voltage: %d mV", ADC_UNIT_1 + 1, RES_ADC_CHANNEL, adc_res_voltage);
            // Calculate the resistance of the unknown resistor
            // Using the voltage divider formula: Vout = Vin * (R2 / (R1 + R2))
            // Rearranged to find R2: R2 = R1 * (Vout / (Vin - Vout))
            int vin = 3300; // Assuming a 3.3V supply voltage in mV
            int vout = adc_res_voltage;
            int r1 = RESISTOR_VALUE_OHMS; // Known resistor value in ohms
            float r2 = r1 * vout / (vin - vout);

            if (vout >= vin)
            {
                ESP_LOGW(TAG_RES, "Measured voltage is greater than or equal to supply voltage. Check connections.");
            }
            else
            {
                if (r2 < 1000)
                {
                    ESP_LOGI(TAG_RES, "Calculated Resistance: %.2f ohms\n", r2);
                }
                else
                {
                    ESP_LOGI(TAG_RES, "Calculated Resistance: %.2f kohms\n", r2 / 1000);
                }
            }
            resistance = r2;

            ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)6, 0));
            xSemaphoreGive(sema_adc);
            vTaskDelay(pdMS_TO_TICKS(messungsDelay));
        }
    }

    // Deinitialize ADC Calibration
    if (do_calibration1_chan3)
    {
        adc_calibration_deinit(adc1_cali_chan3_handle);
    }

    // Tear Down
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
}
*/

void co2_task(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(5000));

    ESP_ERROR_CHECK(scd4x_start_periodic_measurement(&dev));
    ESP_LOGI(TAG_CO2, "Periodic measurements started");

    uint16_t co2;
    float temperature, humidity;
    vTaskDelay(pdMS_TO_TICKS(5000));

    while (1)
    {
        // Stack-Nutzung prüfen
        UBaseType_t stackRemaining = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGW(TAG_CO2, "Stack remaining: %u words", stackRemaining);

        esp_err_t res = scd4x_read_measurement(&dev, &co2, &temperature, &humidity);
        if (res != ESP_OK)
        {
            ESP_LOGE(TAG_CO2, "Error reading results %d (%s)", res, esp_err_to_name(res));
            continue;
        }

        if (co2 == 0)
        {
            ESP_LOGW(TAG_CO2, "Invalid sample detected, skipping");
            continue;
        }

        ESP_LOGI(TAG_CO2, "CO2: %u ppm", co2);
        ESP_LOGI(TAG_CO2, "Temperature: %.2f °C", temperature);
        ESP_LOGI(TAG_CO2, "Humidity: %.2f %%\n", humidity);

        adc_co2_ppm = co2;
        vTaskDelay(pdMS_TO_TICKS(messungsDelay));
    }
}

/*
void ethanol_task(void *pvParameters)
{
    // ADC1 CONFIG
    // ==============================

    adc_oneshot_chan_cfg_t config = {
        .atten = ETH_ADC_ATTEN,
        .bitwidth = ETH_ADC_WIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ETH_ADC_CHANNEL, &config));

    int sum = 0;

    while (1)
    {
        // Ethanolmessung
        // ==============================
        if (xSemaphoreTake(sema_adc, (TickType_t)10) == pdTRUE)
        {
            // Stack-Nutzung prüfen
            UBaseType_t stackRemaining = uxTaskGetStackHighWaterMark(NULL);
            ESP_LOGW(TAG_ETH, "Stack remaining: %u words", stackRemaining);

            vTaskDelay(pdMS_TO_TICKS(5000)); // wait for sensor to stabilize

            sum = 0;
            for (int i = 0; i < 10; i++)
            {
                ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ETH_ADC_CHANNEL, &adc_eth_raw));
                sum += adc_eth_raw;
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            adc_eth_raw = sum / 10;

            ESP_LOGI(TAG_ETH, "Ethanol Raw Data: %d", adc_eth_raw);

            adc_eth_voltage = (adc_eth_raw * 3300) / 4095;
            ESP_LOGI(TAG_ETH, "Ethanol Voltage: %.2f", adc_eth_voltage / 1000);

            adc_eth_ppm = (adc_eth_raw * 500) / 4095; // 12-bit ADC: 0-4095 (2^12 - 1)
            ESP_LOGI(TAG_ETH, "Ethanol PPM: %d\n", adc_eth_ppm);

            xSemaphoreGive(sema_adc);
            vTaskDelay(pdMS_TO_TICKS(messungsDelay));
        }
    }
}
*/

/*
static void temp_task(void *pvParameter)
{
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = TEMP_I2C_PORT,
        .scl_io_num = TEMP_SCL_PIN,
        .sda_io_num = TEMP_SDA_PIN,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;

    i2c_new_master_bus(&i2c_bus_config, &bus_handle);

    mlx90614_config_t mlx90614_config = {
        .mlx90614_device.scl_speed_hz = MASTER_FREQUENCY,
        .mlx90614_device.device_address = 0x5A,
    };

    mlx90614_handle_t mlx90614_handle;
    mlx90614_init(bus_handle, &mlx90614_config, &mlx90614_handle);

    float temp_object = 0.0;
    float temp_ambient = 0.0;

    while (true)
    {
        // Stack-Nutzung prüfen
        UBaseType_t stackRemaining = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGW(TAG_TEMP, "Stack remaining: %u words", stackRemaining);

        mlx90614_get_to(mlx90614_handle, &temp_object);
        mlx90614_get_ta(mlx90614_handle, &temp_ambient);

        ESP_LOGI(TAG_TEMP, "Object Temperature: %.2f C\n", temp_object);
        ESP_LOGI(TAG_TEMP, "Ambient Temperature: %.2f C\n\n", temp_ambient);

        vTaskDelay(pdMS_TO_TICKS(messungsDelay));
    }
}
*/

void database_task(void *pvParameters)
{
    // TODO: diese Funktion muss die Daten von den anderen Tasks bekommen
    // sind aktuell nur globale Variablen

    while (1)
    {
        http_post_res_co2_eth(messungsname, resistance, adc_co2_ppm, adc_eth_ppm);
        vTaskDelay(pdMS_TO_TICKS(messungsDelay));
    }
}

void app_main(void)
{
    // Semaphoren
    // sema_resistance = xSemaphoreCreateBinary();
    // sema_co2 = xSemaphoreCreateBinary();
    // sema_ethanol = xSemaphoreCreateBinary();
    sema_adc = xSemaphoreCreateBinary();

    if (sema_adc == NULL)
    {
        ESP_LOGE("MAIN", "Failed to create ADC semaphore");
        return;
    }

    // Binary semaphore starts empty, must give it once to make it available
    xSemaphoreGive(sema_adc);

    // Initialize i2cdev library - MUST be called before any I2C operations
    ESP_ERROR_CHECK(i2cdev_init());
    ESP_LOGI(TAG_CO2, "I2C device library initialized");

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

    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // i2c init
    // Configure GPIO pins with internal pull-ups before I2C initialization
    ESP_LOGI(TAG_CO2, "Configuring GPIO pins with pull-ups...");
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_EXAMPLE_I2C_MASTER_SDA) | (1ULL << CONFIG_EXAMPLE_I2C_MASTER_SCL),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD, // Open drain mode for I2C
        .pull_up_en = GPIO_PULLUP_ENABLE,  // Enable internal pull-ups
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Initialize I2C device descriptor
    ESP_ERROR_CHECK(scd4x_init_desc(&dev, 0, CONFIG_EXAMPLE_I2C_MASTER_SDA, CONFIG_EXAMPLE_I2C_MASTER_SCL));

    // Give I2C bus time to stabilize
    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG_CO2, "Initializing SCD41 sensor at address 0x%02X...", SCD41_I2C_ADDRESS);

    // Verify that our main device descriptor is set to the correct address
    ESP_LOGI(TAG_CO2, "Device descriptor address: 0x%02X", dev.addr);
    if (dev.addr != SCD41_I2C_ADDRESS)
    {
        ESP_LOGI(TAG_CO2, "Setting device address to 0x%02X", SCD41_I2C_ADDRESS);
        dev.addr = SCD41_I2C_ADDRESS;
    }

    /* Der Teil mit der configMinimal ist viel zu viel Speicher!!
    xTaskCreate(resistance_task, "resistance", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL);
    xTaskCreate(co2_task, "co2", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL);
    xTaskCreate(ethanol_task, "ethanol", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL);
    xTaskCreate(temp_task, "temp", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL);
    xTaskCreate(database_task, "database", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL);
    */

    xTaskCreate(resistance_task, "resistance", 3072, NULL, 5, NULL);
    xTaskCreate(co2_task, "co2", 3072, NULL, 5, NULL);
    xTaskCreate(ethanol_task, "ethanol", 3072, NULL, 5, NULL);
    xTaskCreate(temp_task, "temp", 3072, NULL, 5, NULL);
    // xTaskCreate(database_task, "database", 4096, NULL, 5, NULL);
}
