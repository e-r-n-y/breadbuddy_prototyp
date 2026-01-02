#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <esp_log.h>
#include <math.h>

#include "probe_data.h"

#define VALUECOUNT 128
#define WINDOW_SIZE 5

typedef enum
{
    INITIAL,
    LATENZ,
    EXPANSION,
    EMMISION,
    DEGRATION
} Phase;

typedef struct
{
    uint32_t data[VALUECOUNT];
    uint16_t count;

} Dataset_int;

typedef struct
{
    float data[VALUECOUNT];
    uint16_t count;

} Dataset_float;

typedef struct
{
    float trend_resistance;
    uint32_t value_resistance;
    float trend_co2;
    uint32_t value_co2;
    float trend_temperature;
    float value_temperature;
    Phase phase;
    time_t elapsed_time;
    time_t remaining_time;

} Measurement_state;

extern SemaphoreHandle_t sema_measurement;
extern Dataset_int dataset_resistance;
extern Dataset_int dataset_co2;
extern Dataset_float dataset_temperature;
extern Measurement_state state;
extern struct tm timeinfo;
extern time_t start_time;
extern probe_data_t probendaten;
extern const int messungsDelay;
extern char bake_rest_time[64];
extern char bake_time[64];
extern time_t time_ready;

void initialize_dataset_int(Dataset_int *ds);
void initialize_dataset_float(Dataset_float *ds);
uint8_t update_dataset_int(Dataset_int *ds, uint32_t value);
uint8_t update_dataset_float(Dataset_float *ds, float value);
float calculate_trend_int(Dataset_int *ds);
float calculate_trend_float(Dataset_float *ds);
uint32_t get_latest_value_int(Dataset_int *ds);
float get_latest_value_float(Dataset_float *ds);
Phase calculate_phase(Measurement_state *state);
void update_elapsed_time(Measurement_state *state);
void calculate_remaining_time(Measurement_state *state);
void check_events(Dataset_int *ds);

void dataanalysis_task(void *pvParameters);
