#pragma once

// System Includes
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h" // für SemaphoreHandle_t

// allgemeine Includes
#include <esp_log.h>
// #include <esp_err.h>
// #include <hal/gpio_types.h>
#include <driver/gpio.h>

#include "httprequest.h"

// Allgemeines
extern SemaphoreHandle_t sema_measurement;
extern const int messungsDelay;
extern char messungsname[64];
extern uint32_t resistance;
extern uint32_t co2_ppm;
extern uint32_t adc_eth_ppm;
extern float temp_obj;
extern float temp_amb;
extern float humidity;
extern float temp_amb_co2;

// Funktionen
void database_task(void *pvParameters);