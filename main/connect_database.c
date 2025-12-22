#include "connect_database.h"

void database_task(void *pvParameters)
{
    // TODO: diese Funktion muss die Daten von den anderen Tasks bekommen
    // sind aktuell nur globale Variablen

    while (1)
    {
        if (xSemaphoreTake(sema_measurement, (TickType_t)100) == pdTRUE)
        {
            // http_post_mostdata(messungsname, temp_obj, temp_amb_co2, humidity, resistance, co2_ppm, adc_eth_ppm);

            http_post_alldata(messungsname, temp_obj, temp_amb_co2, humidity, resistance, co2_ppm, adc_eth_ppm, ph_value, time_remain, time_ready, baking, &probendaten, notizen);

            // CLEANUP
            baking = false;
            ph_value = 0.0;

            xSemaphoreGive(sema_measurement);
            vTaskDelay(pdMS_TO_TICKS(messungsDelay));
        }
    }
}