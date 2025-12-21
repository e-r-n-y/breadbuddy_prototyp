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

extern char messungsname[64];
extern float temp_obj;
extern float temp_amb;
extern float humidity;
extern uint32_t widerstand_ohm;
extern uint32_t co2_ppm;
extern uint32_t ethanol_ppm;

extern probe_data_t probendaten;
extern char notizen[1024];

void http_post_mostdata(char *messungsname, float temp_obj, float temp_amb, float humidity, int widerstand_ohm, int co2_ppm, int ethanol_ppm);
