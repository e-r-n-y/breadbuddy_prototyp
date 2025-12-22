#include "webserver.h"

const char *WEB_TAG = "Webserver";
int web_state = 0;
extern float ph_value;
char last_ph_time[64] = "---";
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
    // Größerer Buffer für dynamischen Content
    char *response_buffer = malloc(8192);
    if (response_buffer == NULL)
    {
        ESP_LOGE(WEB_TAG, "Failed to allocate memory for response");
        return ESP_ERR_NO_MEM;
    }

    // Kopiere Content in Buffer
    strncpy(response_buffer, content, 8191);
    response_buffer[8191] = '\0';

    // Ersetze Platzhalter
    char *temp = malloc(8192);
    if (temp == NULL)
    {
        free(response_buffer);
        return ESP_ERR_NO_MEM;
    }

    // Ersetze {{MEASUREMENT_ID}}
    char *pos = strstr(response_buffer, "{{MEASUREMENT_ID}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        size_t after_len = strlen(pos + strlen("{{MEASUREMENT_ID}}"));
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 measurement_id,
                 pos + strlen("{{MEASUREMENT_ID}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{START_TIME}}
    pos = strstr(response_buffer, "{{START_TIME}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 measurement_start_time,
                 pos + strlen("{{START_TIME}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{REST_TIME}}
    pos = strstr(response_buffer, "{{REST_TIME}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 bake_rest_time,
                 pos + strlen("{{REST_TIME}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{BAKE_TIME}}
    pos = strstr(response_buffer, "{{BAKE_TIME}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 bake_time,
                 pos + strlen("{{BAKE_TIME}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{START_DATE}}
    pos = strstr(response_buffer, "{{START_DATE}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 measurement_start_date,
                 pos + strlen("{{START_DATE}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{RESISTANCE}}
    char resistance_str[32];
    snprintf(resistance_str, sizeof(resistance_str), "%lu", (unsigned long)resistance);
    pos = strstr(response_buffer, "{{RESISTANCE}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 resistance_str,
                 pos + strlen("{{RESISTANCE}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{CO2}}
    char co2_str[32];
    snprintf(co2_str, sizeof(co2_str), "%lu", (unsigned long)co2_ppm);
    pos = strstr(response_buffer, "{{CO2}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 co2_str,
                 pos + strlen("{{CO2}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{ETHANOL}}
    char ethanol_str[32];
    snprintf(ethanol_str, sizeof(ethanol_str), "%lu", (unsigned long)adc_eth_ppm);
    pos = strstr(response_buffer, "{{ETHANOL}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 ethanol_str,
                 pos + strlen("{{ETHANOL}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{TEMP_AMB}}
    char temp_amb_str[32];
    snprintf(temp_amb_str, sizeof(temp_amb_str), "%.1f", temp_amb);
    pos = strstr(response_buffer, "{{TEMP_AMB}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 temp_amb_str,
                 pos + strlen("{{TEMP_AMB}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{TEMP_OBJ}}
    char temp_obj_str[32];
    snprintf(temp_obj_str, sizeof(temp_obj_str), "%.1f", temp_obj);
    pos = strstr(response_buffer, "{{TEMP_OBJ}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 temp_obj_str,
                 pos + strlen("{{TEMP_OBJ}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{HUMIDITY}}
    char humidity_str[32];
    snprintf(humidity_str, sizeof(humidity_str), "%.1f", humidity);
    pos = strstr(response_buffer, "{{HUMIDITY}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 humidity_str,
                 pos + strlen("{{HUMIDITY}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{LAST_PH}}
    char last_ph_str[32];
    snprintf(last_ph_str, sizeof(last_ph_str), "%.2f", ph_value);
    pos = strstr(response_buffer, "{{LAST_PH}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 last_ph_str,
                 pos + strlen("{{LAST_PH}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{LAST_PH_TIME}}
    pos = strstr(response_buffer, "{{LAST_PH_TIME}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 last_ph_time,
                 pos + strlen("{{LAST_PH_TIME}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{LAST_BAKE_TIME}}
    pos = strstr(response_buffer, "{{LAST_BAKE_TIME}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 last_bake_time,
                 pos + strlen("{{LAST_BAKE_TIME}}"));
        strcpy(response_buffer, temp);
    }

    // Sende Response
    int result = httpd_resp_send(req, response_buffer, HTTPD_RESP_USE_STRLEN);

    free(temp);
    free(response_buffer);

    return result;
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

size_t readValue(char *buf, char *key, char *output, size_t output_size)
{
    char keyname[64] = {0};

    snprintf(keyname, sizeof(keyname), "%s=", key);
    keyname[sizeof(keyname) - 1] = '\0';

    char *pos = strstr(buf, keyname);
    if (pos)
    {
        pos += strlen(keyname);
        char *end = strpbrk(pos, "&\r\n");
        size_t len = end ? (size_t)(end - pos) : strlen(pos);

        // Temporärer Buffer für URL-encoded Wert
        char temp[256];
        len = MIN(len, sizeof(temp) - 1);
        strncpy(temp, pos, len);
        temp[len] = '\0';

        // URL-Dekodierung durchführen
        url_decode(output, temp);

        ESP_LOGI(WEB_TAG, "Value of %s: %s -- WebState: %d", key, output, web_state);
    }
    return 0;
}

esp_err_t post_req_handler(httpd_req_t *req)
{
    char buf[256];
    int ret;
    int remaining = req->content_len;
    char action_value[64] = {0};
    char buffer[64] = {0};

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

        // Action-Feld abrufen
        readValue(buf, "action", action_value, 64);
        ESP_LOGW(WEB_TAG, "Action value: __%s__", action_value);

        // Beispiel: web_state ändern je nach empfangenen Daten
        if (strcmp(action_value, "begin") == 0)
        {
            // Probendaten abrufen und in struct speichern
            readValue(buf, "starter_menge", buffer, 64);
            probendaten.starter.menge_g = atof(buffer);
            readValue(buf, "starter_temperatur", buffer, 64);
            probendaten.starter.temperatur_c = atof(buffer);
            readValue(buf, "wasser_menge", buffer, 64);
            probendaten.wasser.menge_g = atof(buffer);
            readValue(buf, "wasser_temperatur", buffer, 64);
            probendaten.wasser.temperatur_c = atof(buffer);
            readValue(buf, "mehl_menge", buffer, 64);
            probendaten.mehl.menge_g = atof(buffer);
            readValue(buf, "mehl_temperatur", buffer, 64);
            probendaten.mehl.temperatur_c = atof(buffer);
            readValue(buf, "salz_menge", buffer, 64);
            probendaten.salz.menge_g = atof(buffer);
            readValue(buf, "probe_menge", buffer, 64);
            probendaten.probe.menge_g = atof(buffer);
            readValue(buf, "probe_temperatur", buffer, 64);
            probendaten.probe.temperatur_c = atof(buffer);

            // um Messung auch tatsächlich zu starten
            xSemaphoreGive(sema_measurement);

            web_state = 1;
        }
        else if (strcmp(action_value, "end") == 0)
        {
            web_state = 2;
        }
        else if (strcmp(action_value, "finally") == 0)
        {

            xSemaphoreTake(sema_measurement, portMAX_DELAY);
            // Notizen speichern
            readValue(buf, "notes", notizen, sizeof(notizen));
            ESP_LOGI(WEB_TAG, "Notizen: %s", notizen);
            // TODO:
            // Endzeit notieren
            // Gesamtdauer der Messung berechnen

            // kein SemaphoreGive weil die Messung ja dann beendet werden soll
            // FIXME: hier muss nochmal die Datenbank angesprochen werden bevor alle Aktivität eingestellt werden kann!!!

            web_state = 3;
        }
        else if (strcmp(action_value, "back") == 0 && web_state > 0)
        {
            web_state = 1;
        }
        else if (strcmp(action_value, "new") == 0)
        {
            web_state = 0;
        }
        else if (strcmp(action_value, "log_ph") == 0)
        {
            xSemaphoreTake(sema_measurement, portMAX_DELAY);
            // PH-Werte einmelden
            readValue(buf, "ph_wert", buffer, 64);
            ph_value = atof(buffer);

            xSemaphoreGive(sema_measurement);

            // Aktuelle Zeit für PH-Wert speichern
            time_t now;
            struct tm timeinfo;
            time(&now);
            localtime_r(&now, &timeinfo);
            strftime(last_ph_time, sizeof(last_ph_time), "%H:%M", &timeinfo);

            ESP_LOGI(WEB_TAG, "PH-Wert gespeichert: %.2f um %s", ph_value, last_ph_time);

            web_state = 1;
        }
        else if (strcmp(action_value, "baketime") == 0)
        {
            xSemaphoreTake(sema_measurement, portMAX_DELAY);

            // Aktuelle Zeit für Backzeit speichern
            time_t now;
            struct tm timeinfo;
            time(&now);
            localtime_r(&now, &timeinfo);
            strftime(last_bake_time, sizeof(last_bake_time), "%H:%M", &timeinfo);

            ESP_LOGI(WEB_TAG, "Backzeitpunkt gespeichert: um %s", last_bake_time);

            baking = true;

            xSemaphoreGive(sema_measurement);

            web_state = 1;
        }
        else
        {
            web_state = 0;
        }
    }

    ESP_LOGW("STATE", "Webstate: %d", web_state);

    // return get_req_handler(req);

    // Sende 303 See Other Redirect statt direkt die Seite
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
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

// URL-Dekodierung (ersetzt + durch Leerzeichen und %XX durch entsprechende Zeichen)
void url_decode(char *dst, const char *src)
{
    char a, b;
    while (*src)
    {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b)))
        {
            if (a >= 'a')
                a -= 'a' - 'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            if (b >= 'a')
                b -= 'a' - 'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';
            *dst++ = 16 * a + b;
            src += 3;
        }
        else if (*src == '+')
        {
            *dst++ = ' ';
            src++;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
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