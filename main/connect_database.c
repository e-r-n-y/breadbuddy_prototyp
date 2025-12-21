#include "connect_database.h"

void database_task(void *pvParameters)
{
    // TODO: diese Funktion muss die Daten von den anderen Tasks bekommen
    // sind aktuell nur globale Variablen

    while (1)
    {
        if (xSemaphoreTake(sema_measurement, (TickType_t)100) == pdTRUE)
        {
            http_post_mostdata(messungsname, temp_obj, temp_amb_co2, humidity, resistance, co2_ppm, adc_eth_ppm);

            xSemaphoreGive(sema_measurement);
            vTaskDelay(pdMS_TO_TICKS(messungsDelay));
        }
    }
}