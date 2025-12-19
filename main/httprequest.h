#pragma once

#include <string.h>
#include <stdio.h>
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "10.0.10.9"
#define WEB_PORT "3000"
#define WEB_PATH "/measurements"

void http_post_measurement(const char *messungsname, int widerstand_ohm);
void http_post_resistance_volume(const char *messungsname, int widerstand_ohm, float volume);
void http_post_res_co2_eth(const char *messungsname, int widerstand_ohm, int co2_ppm, int ethanol_ppm);
void http_post_mostdata(const char *messungsname, float temp_obj, float temp_amb, float humidity, int widerstand_ohm, int co2_ppm, int ethanol_ppm);
