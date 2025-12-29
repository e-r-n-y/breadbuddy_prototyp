#include "sensor_co2.h"
#include <string.h> // for memset
const char *TAG_CO2 = "SCD41-CO2";

void init_co2_sensor()
{
    // Initialize i2cdev library - MUST be called before any I2C operations
    // ESP_ERROR_CHECK(i2cdev_init());

    // Initialize I2C device descriptor WITHOUT reconfiguring the bus
    // Initialize I2C device descriptor
    ESP_ERROR_CHECK(scd4x_init_desc(&co2_dev, CO2_I2C_PORT, CO2_I2C_MASTER_SDA, CO2_I2C_MASTER_SCL));

    co2_dev.cfg.sda_pullup_en = true;
    co2_dev.cfg.scl_pullup_en = true;
    // The bus is already configured in main()
    /*
    memset(&co2_dev, 0, sizeof(i2c_dev_t));
    co2_dev.port = CO2_I2C_PORT;
    co2_dev.addr = SCD41_I2C_ADDRESS;
    co2_dev.cfg.sda_io_num = CO2_I2C_MASTER_SDA;
    co2_dev.cfg.scl_io_num = CO2_I2C_MASTER_SCL;
    #if HELPER_TARGET_IS_ESP32
    co2_dev.cfg.master.clk_speed = 100000; // 100kHz for SCD41
    #endif
    ESP_ERROR_CHECK(i2c_dev_create_mutex(&co2_dev));
    */

    ESP_LOGI(TAG_CO2, "SCD41 device descriptor initialized at address 0x%02X", SCD41_I2C_ADDRESS);

    // Give I2C bus time to stabilize
    vTaskDelay(pdMS_TO_TICKS(500));
}

void co2_task(void *pvParameters)
{
    // Warte bis I2C vollständig initialisiert ist
    vTaskDelay(pdMS_TO_TICKS(3000));

    init_co2_sensor();

    /*
    // Try to wake up sensor with error handling
    esp_err_t ret = scd4x_wake_up(&co2_dev);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_CO2, "Failed to wake up sensor: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG_CO2, "Check I2C connections and pull-up resistors!");
        vTaskDelete(NULL);
        return;
    }

    ret = scd4x_stop_periodic_measurement(&co2_dev);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_CO2, "Failed to stop periodic measurement: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }
    */

    if (xSemaphoreTake(sema_measurement, portMAX_DELAY) == pdTRUE)
    {
        /*
        esp_err_t ret = scd4x_reinit(&co2_dev);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG_CO2, "Failed to reinit sensor: %s", esp_err_to_name(ret));
            vTaskDelete(NULL);
            return;
            }
            ESP_LOGI(TAG_CO2, "Sensor initialized");
            */

        printf("DEBUG TAG\n");
        ESP_ERROR_CHECK(scd4x_start_periodic_measurement(&co2_dev));
        printf("DEBUG TAG 2\n");
        ESP_LOGI(TAG_CO2, "Periodic measurements started");
        xSemaphoreGive(sema_measurement);
    }
    uint16_t co2;
    float temperature, current_humidity;
    vTaskDelay(pdMS_TO_TICKS(5000));

    while (1)
    {
        if (xSemaphoreTake(sema_measurement, (TickType_t)100) == pdTRUE)
        {
            // Stack-Nutzung prüfen
            UBaseType_t stackRemaining = uxTaskGetStackHighWaterMark(NULL);
            ESP_LOGW(TAG_CO2, "Stack remaining: %u words", stackRemaining);

            esp_err_t res = scd4x_read_measurement(&co2_dev, &co2, &temperature, &current_humidity);
            if (res != ESP_OK)
            {
                ESP_LOGE(TAG_CO2, "Error reading results %d (%s)", res, esp_err_to_name(res));
                xSemaphoreGive(sema_measurement);
                vTaskDelay(pdMS_TO_TICKS(messungsDelay));
                continue;
            }

            if (co2 == 0)
            {
                ESP_LOGW(TAG_CO2, "Invalid sample detected, skipping");
                xSemaphoreGive(sema_measurement);
                vTaskDelay(pdMS_TO_TICKS(messungsDelay));
                continue;
            }

            ESP_LOGI(TAG_CO2, "CO2: %u ppm", co2);
            ESP_LOGI(TAG_CO2, "Temperature: %.2f °C", temperature);
            ESP_LOGI(TAG_CO2, "Humidity: %.2f %%\n", current_humidity);

            co2_ppm = co2;
            humidity = (int)(current_humidity * 100) / 100.0f; // Um auf 2 Nachkommastellen zu runden
            temp_amb_co2 = (int)(temperature * 100) / 100.0f;
            xSemaphoreGive(sema_measurement);
            vTaskDelay(pdMS_TO_TICKS(messungsDelay));
        }
    }
}