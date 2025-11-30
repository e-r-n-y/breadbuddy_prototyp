#include "sensor_temp.h"

const char *TAG_TEMP = "MLX90614-Temperatur";

void temp_task(void *pvParameter)
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