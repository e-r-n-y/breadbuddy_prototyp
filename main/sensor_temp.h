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
extern SemaphoreHandle_t sema_measurement;
extern const int messungsDelay;

// Settings für Sensor
// #define TEMP_I2C_MASTER_SCL GPIO_NUM_36
// #define TEMP_I2C_MASTER_SDA GPIO_NUM_35
// #define TEMP_I2C_PORT 1 // einzelne Ports-Variante
#define TEMP_I2C_MASTER_SCL GPIO_NUM_1
#define TEMP_I2C_MASTER_SDA GPIO_NUM_2
#define MASTER_FREQUENCY 100000 // brauch ich das?
#define MLX90614_I2C_ADDRESS 0x5A
#define TEMP_I2C_PORT GPIO_NUM_0

// =====================================
// Umbau auf i2cdev
#include <i2cdev.h>
// MLX90614 Register Adressen
#define MLX90614_TA 0x06    // Ambient temperature
#define MLX90614_TOBJ1 0x07 // Object temperature
// Externe Variablen
extern uint32_t temp_obj;
extern uint32_t temp_amb;
extern i2c_dev_t temp_dev;
esp_err_t mlx90614_read_temperature(i2c_dev_t *dev, uint8_t reg, float *temperature);
void init_temp_sensor();
// =======================================

// Funktionen
void temp_task(void *pvParameter);