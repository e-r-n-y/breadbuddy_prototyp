#include "sensor_co2.h"

const char *TAG_CO2 = "SCD41-CO2";

void init_co2_sensor()
{
    // Initialize i2cdev library - MUST be called before any I2C operations
    ESP_ERROR_CHECK(i2cdev_init());
    ESP_LOGI(TAG_CO2, "I2C device library initialized");

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
}

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