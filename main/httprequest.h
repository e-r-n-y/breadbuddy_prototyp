#pragma once

#include <string.h>
#include <stdio.h>
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "probe_data.h"

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "10.0.10.9"
#define WEB_PORT "3000"
#define WEB_PATH "/measurements"

/*
extern char messungsname[64];
extern float temp_obj;
extern float temp_amb;
extern float humidity;
extern uint32_t widerstand_ohm;
extern uint32_t co2_ppm;
extern uint32_t ethanol_ppm;
extern float ph_value;
extern bool baking;

extern probe_data_t probendaten;
extern char notizen[1024];
*/

void http_post_mostdata(char *messungsname, float temp_obj, float temp_amb, float humidity, uint32_t widerstand_ohm, uint32_t co2_ppm, uint32_t ethanol_ppm);

void http_post_alldata(char *messungsname, float temp_obj, float temp_amb, float humidity, uint32_t widerstand_ohm, uint32_t co2_ppm, uint32_t ethanol_ppm, float ph_value, uint32_t time_remain, uint32_t time_ready, bool baking, probe_data_t *probendaten, char *notizen);
