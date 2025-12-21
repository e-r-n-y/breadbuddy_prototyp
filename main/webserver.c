#include "webserver.h"

const char *WEB_TAG = "Webserver";
int web_state = 0;
char index_html[4096];
char messung_html[4096];
char beenden_html[4096];
char zusammenfassung_html[4096];
char style_css[4096];
char response_data[4096];

// URI-Definitionen
httpd_uri_t uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_req_handler,
    .user_ctx = NULL

};

httpd_uri_t uri_post = {
    .uri = "/",
    .method = HTTP_POST,
    .handler = post_req_handler,
    .user_ctx = NULL

};

httpd_uri_t uri_style = {
    .uri = "/style.css",
    .method = HTTP_GET,
    .handler = style_handler,
    .user_ctx = NULL

};

// Allgemeine Funktion zum Lesen einer Datei
static esp_err_t read_file(const char *path, char *buffer, size_t buffer_size)
{
    memset(buffer, 0, buffer_size);
    struct stat st;
    if (stat(path, &st))
    {
        ESP_LOGE(WEB_TAG, "%s not found", path);
        return ESP_FAIL;
    }

    FILE *fp = fopen(path, "r");
    if (fread(buffer, st.st_size, 1, fp) == 0)
    {
        ESP_LOGE(WEB_TAG, "fread failed for %s", path);
        fclose(fp);
        return ESP_FAIL;
    }
    fclose(fp);
    return ESP_OK;
}

// sollte passen - lädt Files in einen Buffer
static void init_web_page_buffers(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true};

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

    if (read_file(INDEX_PATH, index_html, sizeof(index_html)) != ESP_OK)
        return;
    if (read_file(MESSUNG_PATH, messung_html, sizeof(messung_html)) != ESP_OK)
        return;
    if (read_file(BEENDEN_PATH, beenden_html, sizeof(beenden_html)) != ESP_OK)
        return;
    if (read_file(ZUSAMMENFASSUNG_PATH, zusammenfassung_html, sizeof(zusammenfassung_html)) != ESP_OK)
        return;
    if (read_file(STYLE_PATH, style_css, sizeof(style_css)) != ESP_OK)
        return;
}

// diese Funktion sollte einen Inhalt an den browser senden
esp_err_t send_web_page(httpd_req_t *req, const char *content)
{
    int response;
    sprintf(response_data, content);
    response = httpd_resp_send(req, response_data, HTTPD_RESP_USE_STRLEN);
    return response;
}

esp_err_t get_req_handler(httpd_req_t *req)
{
    // TODO: was ist mit den style.css
    switch (web_state)
    {
    case 0:
        break;
    case 1:
        return send_web_page(req, messung_html);
        break;
    case 2:
        return send_web_page(req, beenden_html);
        break;
    case 3:
        return send_web_page(req, zusammenfassung_html);
        break;
    default:
        return send_web_page(req, index_html);
        break;
    }
    return send_web_page(req, index_html);
}

char *readValue(char *buf, char *key)
{
    char value[64] = {0};
    char keyname[64] = {0};

    snprintf(keyname, sizeof(keyname), "%s=", key);
    keyname[sizeof(keyname) - 1] = '\0';

    char *pos = strstr(buf, keyname);
    if (pos)
    {
        pos += strlen(keyname);
        char *end = strpbrk(pos, "&\r\n");
        size_t len = end ? (size_t)(end - pos) : strlen(pos);
        len = MIN(len, sizeof(value) - 1);
        strncpy(value, pos, len);
        value[len] = '\0';
        ESP_LOGI(WEB_TAG, "Value of %s: %s -- WebState: %d", key, value, web_state);
    }
    return value;
}

esp_err_t post_req_handler(httpd_req_t *req)
{
    char buf[256];
    int ret, remaining = req->content_len;
    char action_value[64] = {0};

    while (remaining > 0)
    {
        ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf) - 1));
        if (ret <= 0)
        {
            return ESP_FAIL;
        }
        buf[ret] = '\0';
        remaining -= ret;

        ESP_LOGI(WEB_TAG, "POST data: %s", buf);

        // strcpy(action_value, readValue(buf, "action"));

        // Suche nach "action=" und lese den Wert aus
        char *action_pos = strstr(buf, "action=");
        if (action_pos)
        {
            action_pos += strlen("action=");
            char *end = strpbrk(action_pos, "&\r\n");
            size_t len = end ? (size_t)(end - action_pos) : strlen(action_pos);
            len = MIN(len, sizeof(action_value) - 1);
            strncpy(action_value, action_pos, len);
            action_value[len] = '\0';
            ESP_LOGI(WEB_TAG, "Action value: %s", action_value);
        }
    }

    // Beispiel: web_state ändern je nach empfangenen Daten
    if (strcmp(action_value, "begin") == 0)
        web_state = 1;
    else if (strcmp(action_value, "end") == 0)
        web_state = 2;
    else if (strcmp(action_value, "finally") == 0)
        web_state = 3;
    else if (strcmp(action_value, "back") == 0 && web_state > 0)
        web_state--;
    else if (strcmp(action_value, "new") == 0)
        web_state = 0;

    return get_req_handler(req);
}

esp_err_t style_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/css; charset=utf-8");

    FILE *file = fopen(STYLE_PATH, "r");
    if (file == NULL)
    {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    char buffer[1024];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        if (httpd_resp_send_chunk(req, buffer, bytes_read) != ESP_OK)
        {
            fclose(file);
            return ESP_FAIL;
        }
    }

    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0); // Ende der Übertragung
    return ESP_OK;
}

esp_err_t phwert_handler(httpd_req_t *req)
{
    // TODO:
    return send_web_page(req, style_css);
}

httpd_handle_t setup_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
        httpd_register_uri_handler(server, &uri_style);
    }

    return server;
}

// Webserver Task

void webserver_task(void *pvParameter)
{
    if (wifi_connect_status)
    {
        ESP_LOGI(WEB_TAG, "SPIFFS Web Server is running ... ...\n");
        init_web_page_buffers();
        setup_server();

        // Stack-Nutzung prüfen
        UBaseType_t stackRemaining = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGW(WEB_TAG, "Stack remaining: %u words", stackRemaining);
    }
    else
    {
        ESP_LOGE(WEB_TAG, "WiFi not connected, cannot start webserver");
    }

    // Task completed, delete itself
    vTaskDelete(NULL);
}