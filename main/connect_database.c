#include "connect_database.h"

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