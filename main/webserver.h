#pragma once

// System Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h" // für SemaphoreHandle_t

#include "esp_system.h"
#include "esp_spi_flash.h"
#include <esp_http_server.h>
#include "esp_spiffs.h"

#include <esp_log.h>
#include "connect_wifi.h"
#include "probe_data.h"
#include "httprequest.h"

#include <time.h>
#include <sys/time.h>

extern const char *WEB_TAG;
extern int web_state;
extern int wifi_connect_status;

// Files to be served
#define INDEX_PATH "/spiffs/index.html"
#define MESSUNG_PATH "/spiffs/messung.html"
#define BEENDEN_PATH "/spiffs/beenden.html"
#define ZUSAMMENFASSUNG_PATH "/spiffs/zusammenfassung.html"
#define STYLE_PATH "/spiffs/style.css"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

extern char index_html[4096];
extern char messung_html[4096];
extern char beenden_html[4096];
extern char zusammenfassung_html[8192];
extern char style_css[4096];
extern char response_data[4096];

static esp_err_t read_file(const char *path, char *buffer, size_t buffer_size);
static void init_web_page_buffers(void);
esp_err_t send_web_page(httpd_req_t *req, const char *content);
esp_err_t get_req_handler(httpd_req_t *req);
size_t readValue(char *buf, char *key, char *output, size_t output_size);
esp_err_t post_req_handler(httpd_req_t *req);
esp_err_t style_handler(httpd_req_t *req);
esp_err_t phwert_handler(httpd_req_t *req);
esp_err_t favicon_handler(httpd_req_t *req);
void url_decode(char *dst, const char *src);
httpd_handle_t setup_server(void);
void webserver_task(void *pvParameter);

// URI-Definitionen
extern httpd_uri_t uri_get;
extern httpd_uri_t uri_post;
extern httpd_uri_t uri_style;
extern httpd_uri_t uri_favicon;

extern probe_data_t probendaten;
extern char notizen[1024];
extern bool baking;
extern char measurement_id[64];
extern char measurement_start_time[64];
extern char measurement_start_date[64];
extern char measurement_stop_time[64];
extern char measurement_stop_date[64];
extern char measurement_duration[64];
extern char elapsed_time_str[32];
extern char bake_rest_time[64];
extern char bake_time[64];
extern char last_bake_time[64];
extern uint32_t resistance;
extern uint32_t co2_ppm;
extern uint32_t adc_eth_ppm;
extern float temp_amb;
extern float temp_amb_co2;
extern float temp_obj;
extern float humidity;
extern float ph_value;
extern float last_ph_value;
extern char last_ph_time[64];
extern ph_t ph_array[32];
extern uint32_t time_remain;
extern uint32_t time_ready;

extern char messungsname[64];

extern SemaphoreHandle_t sema_measurement;

extern time_t now;
extern struct tm timeinfo;
extern time_t start_time;
extern time_t stop_time;
extern time_t duration;
