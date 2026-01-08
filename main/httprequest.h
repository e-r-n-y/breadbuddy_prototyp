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

/* HTTP POST Error Codes */
typedef enum
{
    HTTP_POST_SUCCESS = 0,
    HTTP_POST_ERR_JSON_CREATE = -1,
    HTTP_POST_ERR_JSON_PRINT = -2,
    HTTP_POST_ERR_MEM_ALLOC = -3,
    HTTP_POST_ERR_DNS_LOOKUP = -4,
    HTTP_POST_ERR_SOCKET_ALLOC = -5,
    HTTP_POST_ERR_SOCKET_CONNECT = -6,
    HTTP_POST_ERR_SOCKET_SEND = -7,
    HTTP_POST_ERR_SOCKET_TIMEOUT = -8
} http_post_error_t;

http_post_error_t http_post_alldata(char *messungsname, float temp_obj, float temp_amb, float humidity, uint32_t widerstand_ohm, uint32_t co2_ppm, uint32_t ethanol_ppm, float ph_value, uint32_t time_remain, uint32_t time_ready, bool baking, probe_data_t *probendaten, char *notizen);
