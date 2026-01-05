#include "sensor_temp.h"

const char *TAG_TEMP = "D6T-1A-01_Temperatur";

/**
 * @brief CRC-8 Berechnung basierend auf dem D6T Handbuch [cite: 405]
 * Polynom: X^8 + X^2 + X^1 + 1 (SMBus Standard)
 */
unsigned char calc_crc(unsigned char data)
{
    int index;
    unsigned char temp;
    for (index = 0; index < 8; index++)
    {
        temp = data;
        data <<= 1;
        if (temp & 0x80)
        {
            data ^= 0x07;
        }
    }
    return data;
}

/**
 * @brief Überprüfung der PEC (Packet Error Check) Daten
 * Die Berechnung bezieht Write-Adresse, Command und Read-Adresse mit ein.
 */
bool d6t_check_pec(uint8_t *buffer, int length)
{
    unsigned char crc;
    int i;

    // Berechnung startet mit Schreib-Adresse (0x14 = 0x0A << 1 | 0)
    crc = calc_crc(0x14);
    // XOR mit Command (0x4C)
    crc = calc_crc(0x4C ^ crc);
    // XOR mit Lese-Adresse (0x15 = 0x0A << 1 | 1)
    crc = calc_crc(0x15 ^ crc);

    // Iteration über die empfangenen Datenbytes (ohne das letzte PEC Byte)
    for (i = 0; i < length; i++)
    {
        crc = calc_crc(buffer[i] ^ crc);
    }

    // Vergleich mit dem empfangenen PEC Byte (letztes Byte im Buffer)
    return (crc == buffer[length]);
}

/**
 * @brief Auslesen der Temperaturdaten mit i2cdev
 */
esp_err_t D6T_1A_01_read_temperatur(i2c_dev_t *dev, float *temperature)
{
    uint8_t data[5];

    /*
    // Mutex nehmen
    I2C_DEV_TAKE_MUTEX(dev);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, D6T_CMD_READ, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 4, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &data[4], I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(dev->port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    // Mutex freigeben
    I2C_DEV_GIVE_MUTEX(dev);
    */

    // Mit i2cdev: Command schreiben (0x4C) und dann 5 Bytes lesen
    esp_err_t ret = i2c_dev_read_reg(dev, D6T_CMD_READ, data, 5);

    if (ret == ESP_OK)
    {
        // PEC Check durchführen (letztes Byte ist PEC, daher 4 Datenbytes)
        if (d6t_check_pec(data, 4))
        {
            // Umrechnung der Daten
            // Daten sind signed 16-bit integer, multipliziert mit 10
            int16_t tPTAT_raw = (data[1] << 8) | data[0];
            int16_t tP0_raw = (data[3] << 8) | data[2];

            float tPTAT = (float)tPTAT_raw / 10.0;
            float tP0 = (float)tP0_raw / 10.0;
            *temperature = tP0;

            ESP_LOGI(TAG_TEMP, "Ref Temp (PTAT): %.1f °C | Object Temp (P0): %.1f °C", tPTAT, tP0);
        }
        else
        {
            ESP_LOGE(TAG_TEMP, "PEC Check Fehler! Daten korrupt.");
            return ESP_ERR_INVALID_CRC;
        }
    }
    else
    {
        ESP_LOGE(TAG_TEMP, "I2C Read Failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

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

    ESP_LOGI(TAG_TEMP, "Initializing temperature sensor on shared I2C bus...");

    // Initialize device descriptor
    // i2cdev_init() wurde bereits in app_main() aufgerufen
    memset(&temp_dev, 0, sizeof(i2c_dev_t));
    temp_dev.port = TEMP_I2C_PORT;
    temp_dev.addr = D6T_I2C_ADDRESS;
    temp_dev.cfg.sda_io_num = TEMP_I2C_MASTER_SDA;
    temp_dev.cfg.scl_io_num = TEMP_I2C_MASTER_SCL;
    temp_dev.cfg.sda_pullup_en = true;
    temp_dev.cfg.scl_pullup_en = true;
#if HELPER_TARGET_IS_ESP32
    temp_dev.cfg.master.clk_speed = 100000; // 100kHz
#endif
    ESP_ERROR_CHECK(i2c_dev_create_mutex(&temp_dev));

    ESP_LOGI(TAG_TEMP, "D6T-1A-01 initialized at address 0x%02X", D6T_I2C_ADDRESS);
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
            // esp_err_t res_obj = mlx90614_read_temperature(&temp_dev, MLX90614_TOBJ1, &temp_object);
            esp_err_t res_obj = D6T_1A_01_read_temperatur(&temp_dev, &temp_object);

            // Lese Umgebungstemperatur
            // esp_err_t res_amb = mlx90614_read_temperature(&temp_dev, MLX90614_TA, &temp_ambient);

            if (res_obj == ESP_OK) // && res_amb == ESP_OK)
            {
                ESP_LOGI(TAG_TEMP, "Object Temperature: %.2f °C", temp_object);
                // ESP_LOGI(TAG_TEMP, "Ambient Temperature: %.2f °C", temp_ambient);

                // Update globale Variablen
                temp_obj = temp_object;
                // temp_amb = temp_ambient;
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