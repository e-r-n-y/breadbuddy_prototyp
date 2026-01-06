#include "connect_database.h"

void database_task(void *pvParameters)
{
    // TODO: diese Funktion muss die Daten von den anderen Tasks bekommen???
    // sind aktuell nur globale Variablen
    // INFO: bei Betreuer

    // Warten auf Start der Messung
    xSemaphoreTake(sema_measurement, portMAX_DELAY);
    xSemaphoreGive(sema_measurement);

    // Initialer Delay damit die Datenbank zeitversetzt zur Messung arbeiten kann
    ESP_LOGE("DATABASE", "waiting for initial delay");
    vTaskDelay(pdMS_TO_TICKS(messungsDelay / 2));
    ESP_LOGE("DATABASE", "started working");

    while (1)
    {
        if (xSemaphoreTake(sema_measurement, (TickType_t)100) == pdTRUE)
        {
            http_post_error_t result = http_post_alldata(messungsname, temp_obj, temp_amb_co2, humidity, resistance, co2_ppm, adc_eth_ppm, ph_value, (uint32_t)state.remaining_time, time_ready, baking, &probendaten, notizen);

            // Fehlerbehandlung
            if (result != HTTP_POST_SUCCESS)
            {
                ESP_LOGE("DATABASE", "HTTP POST failed with error code: %d", result);

                // Optional: Retry-Logik für Netzwerkfehler
                if (result == HTTP_POST_ERR_SOCKET_CONNECT || result == HTTP_POST_ERR_DNS_LOOKUP)
                {
                    ESP_LOGW("DATABASE", "Network error, retrying in 2 seconds...");
                    vTaskDelay(pdMS_TO_TICKS(2000));

                    // Zweiter Versuch
                    result = http_post_alldata(messungsname, temp_obj, temp_amb_co2, humidity, resistance, co2_ppm, adc_eth_ppm, ph_value, (uint32_t)state.remaining_time, time_ready, baking, &probendaten, notizen);

                    if (result != HTTP_POST_SUCCESS)
                    {
                        ESP_LOGE("DATABASE", "HTTP POST retry failed with error code: %d", result);
                    }
                    else
                    {
                        ESP_LOGI("DATABASE", "HTTP POST retry succeeded");
                    }
                }
            }
            else
            {
                ESP_LOGI("DATABASE", "HTTP POST successful");
            }

            // CLEANUP
            baking = false;
            ph_value = 0.0;

            xSemaphoreGive(sema_measurement);
            vTaskDelay(pdMS_TO_TICKS(messungsDelay));
        }
    }
}