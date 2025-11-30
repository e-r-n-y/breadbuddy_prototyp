#pragma once

// System Includes
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h" // für SemaphoreHandle_t

// allgemeines
#include <esp_log.h>
// #include <esp_err.h>
// #include <hal/gpio_types.h>

// Includes für ADC
#include "soc/soc_caps.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

// Allgemeines
#include <driver/gpio.h>
extern const char *TAG_ETH;
extern SemaphoreHandle_t sema_adc;
extern adc_oneshot_unit_handle_t adc1_handle;
extern const int messungsDelay;

// Settings für ADC
#define ETH_ADC_ATTEN ADC_ATTEN_DB_12
#define ETH_ADC_CHANNEL ADC_CHANNEL_7 // ist Pin 8
#define ETH_ADC_WIDTH ADC_BITWIDTH_DEFAULT

// Werte für Messung
extern int adc_eth_raw;
extern float adc_eth_voltage;
extern uint32_t adc_eth_ppm;

// Funktionen
void ethanol_task(void *pvParameters);