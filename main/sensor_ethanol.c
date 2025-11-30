#include "sensor_ethanol.h"

const char *TAG_ETH = "ADC-Ethanol";

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