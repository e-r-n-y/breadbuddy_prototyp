#include "sensor_temp.h"

const char *TAG_TEMP = "MLX90614-Temperatur";

esp_err_t mlx90614_read_temperature(i2c_dev_t *dev, uint8_t reg, float *temperature)
{
    uint8_t data[3];

    printf("DEBUG TAG\n");
    // Read 3 bytes: LSB, MSB, PEC (we ignore PEC for simplicity)
    esp_err_t res = i2c_dev_read_reg(dev, reg, data, 3);
    if (res != ESP_OK)
    {
        ESP_LOGE(TAG_TEMP, "Failed to read from register 0x%02X: %s", reg, esp_err_to_name(res));
        return res;
    }

    // Combine LSB and MSB
    uint16_t raw_temp = (data[1] << 8) | data[0];

    // Check if error flag is set (bit 15)
    if (raw_temp & 0x8000)
    {
        ESP_LOGE(TAG_TEMP, "Sensor error flag set");
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Convert to Celsius: raw_temp * 0.02 - 273.15
    *temperature = (raw_temp * 0.02f) - 273.15f;

    return ESP_OK;
}

void init_temp_sensor()
{
    // Initialize i2cdev library - MUST be called before any I2C operations
    // ESP_ERROR_CHECK(i2cdev_init());

    ESP_LOGI(TAG_TEMP, "Initializing MLX90614 sensor on shared I2C bus...");

    // Initialize device descriptor
    // i2cdev_init() wurde bereits in app_main() aufgerufen
    memset(&temp_dev, 0, sizeof(i2c_dev_t));
    temp_dev.port = TEMP_I2C_PORT;
    temp_dev.addr = MLX90614_I2C_ADDRESS;
    temp_dev.cfg.sda_io_num = TEMP_I2C_MASTER_SDA;
    temp_dev.cfg.scl_io_num = TEMP_I2C_MASTER_SCL;
    temp_dev.cfg.sda_pullup_en = true;
    temp_dev.cfg.scl_pullup_en = true;
#if HELPER_TARGET_IS_ESP32
    temp_dev.cfg.master.clk_speed = 100000; // 100kHz
#endif
    ESP_ERROR_CHECK(i2c_dev_create_mutex(&temp_dev));

    ESP_LOGI(TAG_TEMP, "MLX90614 initialized at address 0x%02X", MLX90614_I2C_ADDRESS);
}

void temp_task(void *pvParameter)
{
    // Warte auf I2C-Initialisierung
    vTaskDelay(pdMS_TO_TICKS(3000));

    // Initialisiere den Sensor
    init_temp_sensor();

    vTaskDelay(pdMS_TO_TICKS(1000));

    float temp_object = 0.0;
    float temp_ambient = 0.0;

    while (true)
    {
        if (xSemaphoreTake(sema_measurement, (TickType_t)100) == pdTRUE)
        {

            // Stack-Nutzung prüfen
            UBaseType_t stackRemaining = uxTaskGetStackHighWaterMark(NULL);
            ESP_LOGW(TAG_TEMP, "Stack remaining: %u words", stackRemaining);

            // vTaskDelay(pdMS_TO_TICKS(10000));
            // i2c_dev_probe(&temp_dev, I2C_DEV_READ);
            // printf("DEBUG TAG 2\n");

            // Lese Objekttemperatur
            esp_err_t res_obj = mlx90614_read_temperature(&temp_dev, MLX90614_TOBJ1, &temp_object);

            // Lese Umgebungstemperatur
            esp_err_t res_amb = mlx90614_read_temperature(&temp_dev, MLX90614_TA, &temp_ambient);

            if (res_obj == ESP_OK && res_amb == ESP_OK)
            {
                ESP_LOGI(TAG_TEMP, "Object Temperature: %.2f °C", temp_object);
                ESP_LOGI(TAG_TEMP, "Ambient Temperature: %.2f °C", temp_ambient);

                // Update globale Variablen
                temp_obj = temp_object;
                temp_amb = temp_ambient;
            }
            else
            {
                ESP_LOGE(TAG_TEMP, "Failed to read temperatures");
            }

            xSemaphoreGive(sema_measurement);
            vTaskDelay(pdMS_TO_TICKS(messungsDelay));
        }
    }
}