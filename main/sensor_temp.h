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

// includes für Sensor
#include "mlx90614.h"

// Allgemeines
extern const char *TAG_TEMP;
extern const int messungsDelay;

// Settings für Sensor
#define TEMP_SCL_PIN GPIO_NUM_36
#define TEMP_SDA_PIN GPIO_NUM_35
#define MASTER_FREQUENCY 100000
#define TEMP_I2C_PORT 1

// Funktionen
void temp_task(void *pvParameter);