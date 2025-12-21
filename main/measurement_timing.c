#include "measurement_timing.h"

void measurement_timing_task(void *pvParameters)
{
    while (true)
    {
        // Was mach ich hiermit eigentlich??
        // TODO: mal schaun ob ich das hier wirklich brauche

        vTaskDelay(pdMS_TO_TICKS(messungsDelay));
    }
}