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

#include "time.h"

// Allgemeines
#include <driver/gpio.h>
extern const char *TAG_RES;
extern SemaphoreHandle_t sema_measurement;
extern adc_oneshot_unit_handle_t adc1_handle;
extern const int messungsDelay;

// Settings für ADC
#define RES_ADC_ATTEN ADC_ATTEN_DB_12
#define RES_ADC_CHANNEL ADC_CHANNEL_9
#define RES_ADC_WIDTH ADC_BITWIDTH_DEFAULT
#define RES_GPIO GPIO_NUM_18

// Werte für Messung
#define RESISTOR_VALUE_OHMS 6700 // Value of the known resistor in ohms
extern int adc_res_raw;
extern int adc_res_voltage;
extern uint32_t resistance;
extern uint32_t last_resistance;
extern time_t knickpunkt; // besseren Name finden
extern bool knickpunkt_erreicht;
extern uint32_t time_remain;
extern uint32_t time_ready;
extern time_t duration;
extern time_t start_time;

// Funktionen
bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);

void adc_calibration_deinit(adc_cali_handle_t handle);

// Task für Messung
void resistance_task(void *pvParameters);