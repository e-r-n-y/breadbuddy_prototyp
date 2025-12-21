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
extern char zusammenfassung_html[4096];
extern char style_css[4096];
extern char response_data[4096];

static esp_err_t read_file(const char *path, char *buffer, size_t buffer_size);
static void init_web_page_buffers(void);
esp_err_t send_web_page(httpd_req_t *req, const char *content);
esp_err_t get_req_handler(httpd_req_t *req);
char *readValue(char *buf, char *key);
esp_err_t post_req_handler(httpd_req_t *req);
esp_err_t style_handler(httpd_req_t *req);
esp_err_t phwert_handler(httpd_req_t *req);
httpd_handle_t setup_server(void);
void webserver_task(void *pvParameter);

// URI-Definitionen
extern httpd_uri_t uri_get;
extern httpd_uri_t uri_post;
extern httpd_uri_t uri_style;
