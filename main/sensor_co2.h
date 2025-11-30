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

// für SCD41 CO2 Sensor
#include <scd4x.h>
#include <i2cdev.h> // Add this include

// Allgemeines
extern const char *TAG_CO2;
extern const int messungsDelay;

// Settings for Sensor
#define CONFIG_EXAMPLE_I2C_MASTER_SCL GPIO_NUM_13
#define CONFIG_EXAMPLE_I2C_MASTER_SDA GPIO_NUM_14
// SCD41 I2C address according to datasheet
#define SCD41_I2C_ADDRESS 0x62
#define CO2_I2C_PORT 0

extern i2c_dev_t dev;
extern uint32_t adc_co2_ppm;

// Funktionen
void init_co2_sensor();
void co2_task(void *pvParameters);