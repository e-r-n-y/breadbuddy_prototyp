#pragma once

#include "cJSON.h"
#include "time.h"

typedef struct
{
    float menge_g;
    float temperatur_c;
} zutat_t;

typedef struct
{
    float menge_g;
} zutat_ohne_temp_t;

typedef struct
{
    zutat_t starter;
    zutat_t wasser;
    zutat_t mehl;
    zutat_ohne_temp_t salz;
    zutat_t probe;
} probe_data_t;

typedef struct
{
    float ph_value;
    char time_string[64];
} ph_t;

probe_data_t *probe_data_from_json(const char *json_string);
char *probe_data_to_json(const probe_data_t *data);