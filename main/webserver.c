#include "webserver.h"

const char *WEB_TAG = "Webserver";
int web_state = 0;
int ph_idx = 0;
// extern float ph_value;
char last_ph_time[64] = "---";
char index_html[4096];
char messung_html[4096];
char beenden_html[4096];
char zusammenfassung_html[8192];
char style_css[4096];
char response_data[4096];
time_t now;
struct tm timeinfo;

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

httpd_uri_t uri_favicon = {
    .uri = "/favicon.ico",
    .method = HTTP_GET,
    .handler = favicon_handler,
    .user_ctx = NULL};

// Allgemeine Funktion zum Lesen einer Datei
esp_err_t read_file(const char *path, char *buffer, size_t buffer_size)
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
void init_web_page_buffers(void)
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
    snprintf(temp_amb_str, sizeof(temp_amb_str), "%.1f", temp_amb_co2);
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
    snprintf(last_ph_str, sizeof(last_ph_str), "%.2f", last_ph_value);
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

    // Ersetze {{STOP_DATE}}
    pos = strstr(response_buffer, "{{STOP_DATE}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 measurement_stop_date,
                 pos + strlen("{{STOP_DATE}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{STOP_TIME}}
    pos = strstr(response_buffer, "{{STOP_TIME}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 measurement_stop_time,
                 pos + strlen("{{STOP_TIME}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{DURATION}}
    pos = strstr(response_buffer, "{{DURATION}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 measurement_duration,
                 pos + strlen("{{DURATION}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{PROBE_TEMP}}
    char probe_temp_str[32];
    snprintf(probe_temp_str, sizeof(probe_temp_str), "%.1f", probendaten.probe.temperatur_c);
    pos = strstr(response_buffer, "{{PROBE_TEMP}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 probe_temp_str,
                 pos + strlen("{{PROBE_TEMP}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{MEHL_MENGE}}
    char mehl_menge_str[32];
    snprintf(mehl_menge_str, sizeof(mehl_menge_str), "%.1f", probendaten.mehl.menge_g);
    pos = strstr(response_buffer, "{{MEHL_MENGE}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 mehl_menge_str,
                 pos + strlen("{{MEHL_MENGE}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{WASSER_MENGE}}
    char wasser_menge_str[32];
    snprintf(wasser_menge_str, sizeof(wasser_menge_str), "%.1f", probendaten.wasser.menge_g);
    pos = strstr(response_buffer, "{{WASSER_MENGE}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 wasser_menge_str,
                 pos + strlen("{{WASSER_MENGE}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{STARTER_MENGE}}
    char starter_menge_str[32];
    snprintf(starter_menge_str, sizeof(starter_menge_str), "%.1f", probendaten.starter.menge_g);
    pos = strstr(response_buffer, "{{STARTER_MENGE}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 starter_menge_str,
                 pos + strlen("{{STARTER_MENGE}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{SALZ_MENGE}}
    char salz_menge_str[32];
    snprintf(salz_menge_str, sizeof(salz_menge_str), "%.1f", probendaten.salz.menge_g);
    pos = strstr(response_buffer, "{{SALZ_MENGE}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 salz_menge_str,
                 pos + strlen("{{SALZ_MENGE}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{PROBE_MENGE}}
    char probe_menge_str[32];
    snprintf(probe_menge_str, sizeof(probe_menge_str), "%.1f", probendaten.probe.menge_g);
    pos = strstr(response_buffer, "{{PROBE_MENGE}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 probe_menge_str,
                 pos + strlen("{{PROBE_MENGE}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{NOTIZEN}}
    pos = strstr(response_buffer, "{{NOTIZEN}}");
    if (pos)
    {
        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 notizen,
                 pos + strlen("{{NOTIZEN}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{PH_DATA_JSON}}
    pos = strstr(response_buffer, "{{PH_DATA_JSON}}");
    if (pos)
    {
        // Erstelle JSON Array mit PH-Werten
        char ph_json[2048] = "[";
        bool first = true;

        for (int i = 0; i < ph_idx && i < 32; i++)
        {
            if (ph_array[i].ph_value > 0)
            {
                char entry[128];
                if (!first)
                {
                    strcat(ph_json, ",");
                }
                snprintf(entry, sizeof(entry),
                         "{\"time\":\"%s\",\"value\":%.2f}",
                         ph_array[i].time_string,
                         ph_array[i].ph_value);
                strcat(ph_json, entry);
                first = false;
            }
        }
        strcat(ph_json, "]");

        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 ph_json,
                 pos + strlen("{{PH_DATA_JSON}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{ELAPSED_TIME}}
    pos = strstr(response_buffer, "{{ELAPSED_TIME}}");
    if (pos)
    {

        // Berechne vergangene Zeit wenn Messung läuft
        if (start_time > 0)
        {
            time_t current_time;
            time(&current_time);
            time_t elapsed_seconds = current_time - start_time;

            int hours = elapsed_seconds / 3600;
            int minutes = (elapsed_seconds % 3600) / 60;

            snprintf(elapsed_time_str, sizeof(elapsed_time_str), "%dh %02dmin", hours, minutes);
        }

        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 elapsed_time_str,
                 pos + strlen("{{ELAPSED_TIME}}"));
        strcpy(response_buffer, temp);
    }

    // Ersetze {{PHASE}}
    pos = strstr(response_buffer, "{{PHASE}}");
    if (pos)
    {
        const char *phase_str;
        switch (state.phase)
        {
        case INITIAL:
            phase_str = "Initialphase";
            break;
        case LATENZ:
            phase_str = "Latenzphase";
            break;
        case EXPANSION:
            phase_str = "Expansionsphase";
            break;
        case EMMISION:
            phase_str = "Emissionsphase";
            break;
        case DEGRATION:
            phase_str = "Degradationsphase";
            break;
        default:
            phase_str = "Unbekannt";
            break;
        }

        size_t before_len = pos - response_buffer;
        snprintf(temp, 8192, "%.*s%s%s",
                 (int)before_len, response_buffer,
                 phase_str,
                 pos + strlen("{{PHASE}}"));
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

            // TODO: das sollte eigentlich erst beim Messungsstart durchgeführt werden!!!
            // Erfasse aktuelle Zeit
            time(&start_time);
            ESP_LOGE("TIME", "Startzeit: %lu", (uint32_t)start_time);
            localtime_r(&start_time, &timeinfo);

            // Erstelle Messungs-ID: JJMMTT_HHMM
            strftime(measurement_id, sizeof(measurement_id), "%y%m%d_%H%M", &timeinfo);

            // Uhrzeit: HH:MM
            strftime(measurement_start_time, sizeof(measurement_start_time), "%H:%M", &timeinfo);

            // Datum: DD.MM.JJJJ
            strftime(measurement_start_date, sizeof(measurement_start_date), "%d.%m.%Y", &timeinfo);

            ESP_LOGI("TIME", "Messung gestartet: %s um %s am %s",
                     measurement_id, measurement_start_time, measurement_start_date);

            // Erzeuge den Messungsname String
            strcpy(messungsname, measurement_id);
            strcat(messungsname, "_breadbuddy_prototyp");
            ESP_LOGE("TIME", "Messungsname: %s", messungsname);

            // resetten des Ph-Value Arrays
            memset(ph_array, 0, sizeof(ph_array));
            ph_idx = 0;

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
            // Endzeit notieren
            // Erfasse aktuelle Zeit
            time(&stop_time);
            ESP_LOGE("TIME", "Endzeit: %lu", (uint32_t)stop_time);
            localtime_r(&stop_time, &timeinfo);

            // Uhrzeit: HH:MM
            strftime(measurement_stop_time, sizeof(measurement_stop_time), "%H:%M", &timeinfo);

            // Datum: DD.MM.JJJJ
            strftime(measurement_stop_date, sizeof(measurement_stop_date), "%d.%m.%Y", &timeinfo);

            ESP_LOGI("TIME", "Messung gestoppt: %s um %s am %s",
                     measurement_id, measurement_stop_time, measurement_stop_date);

            // TODO:
            // Gesamtdauer der Messung berechnen
            duration = stop_time - start_time;
            ESP_LOGI("TIME", "Messungsdauer [s]: %lu", (uint32_t)duration);
            int hours;
            int minutes;
            minutes = duration / 60;
            hours = minutes / 60;
            minutes = minutes % 60;

            // localtime_r(&duration, &timeinfo);
            sprintf(measurement_duration, "%dh %02dmin", hours, minutes);
            ESP_LOGI("TIME", "Messungsdauer: %s", measurement_duration);

            // kein SemaphoreGive weil die Messung ja dann beendet werden soll

            // hier muss nochmal die Datenbank angesprochen werden bevor alle Aktivität eingestellt werden kann!!!
            http_post_alldata(messungsname, temp_obj, temp_amb_co2, humidity, resistance, co2_ppm, adc_eth_ppm, ph_value, time_remain, time_ready, baking, &probendaten, notizen);

            web_state = 3;
        }
        else if (strcmp(action_value, "back") == 0 && web_state > 0)
        {
            web_state = 1;
        }
        else if (strcmp(action_value, "new") == 0)
        {
            // RESET and CLEAN UP
            adc_res_raw = 0;
            adc_res_voltage = 0;
            resistance = 0;
            last_resistance = 0;
            knickpunkt_erreicht = false;
            adc_eth_raw = 0;
            adc_eth_voltage = 0.0;
            adc_eth_ppm = 0;
            co2_ppm = 0;
            temp_amb_co2 = 0;
            humidity = 0.0;
            temp_obj = 0.0;
            temp_amb = 0.0;
            notizen[0] = '\0';
            strcpy(bake_rest_time, "---");
            time_remain = 25200;
            strcpy(bake_time, "---");
            time_ready = 0;
            strcpy(last_bake_time, "---");
            baking = false;
            strcpy(measurement_id, "---");
            strcpy(measurement_start_time, "---");
            strcpy(measurement_start_date, "---");
            strcpy(measurement_stop_time, "---");
            strcpy(measurement_stop_date, "---");
            strcpy(measurement_duration, "---");
            strcpy(elapsed_time_str, "---");
            state.elapsed_time = 0;
            state.remaining_time = 0;
            state.phase = INITIAL;
            initialize_dataset_int(&dataset_resistance);
            initialize_dataset_int(&dataset_co2);
            initialize_dataset_float(&dataset_temperature);

            web_state = 0;
        }
        else if (strcmp(action_value, "log_ph") == 0)
        {
            xSemaphoreTake(sema_measurement, portMAX_DELAY);
            // PH-Werte einmelden
            readValue(buf, "ph_wert", buffer, 64);
            ph_value = atof(buffer);
            last_ph_value = ph_value;

            // Aktuelle Zeit für PH-Wert speichern
            time(&now);
            localtime_r(&now, &timeinfo);
            strftime(last_ph_time, sizeof(last_ph_time), "%H:%M", &timeinfo);

            ESP_LOGI(WEB_TAG, "PH-Wert gespeichert: %.2f um %s", ph_value, last_ph_time);

            ph_array[ph_idx].ph_value = ph_value;
            strcpy(ph_array[ph_idx].time_string, last_ph_time);
            ph_idx++;

            xSemaphoreGive(sema_measurement);

            web_state = 1;
        }
        else if (strcmp(action_value, "baketime") == 0)
        {
            xSemaphoreTake(sema_measurement, portMAX_DELAY);

            // Aktuelle Zeit für Backzeit speichern
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
    const char *filepath = STYLE_PATH;

    // Prüfe ob Datei existiert
    struct stat st;
    if (stat(filepath, &st) != 0)
    {
        ESP_LOGE(WEB_TAG, "style.css not found");
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    FILE *file = fopen(filepath, "r");
    if (file == NULL)
    {
        ESP_LOGE(WEB_TAG, "Failed to open style.css");
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    // Setze Cache-Header (7 Tage)
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=604800, immutable");

    // ETag für besseres Caching
    char etag[32];
    snprintf(etag, sizeof(etag), "\"%lld\"", (long long)st.st_mtime);
    httpd_resp_set_hdr(req, "ETag", etag);

    // Content-Type
    httpd_resp_set_type(req, "text/css; charset=utf-8");

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
    httpd_resp_send_chunk(req, NULL, 0);

    ESP_LOGI(WEB_TAG, "style.css sent successfully (%ld bytes)", st.st_size);
    return ESP_OK;
}

esp_err_t phwert_handler(httpd_req_t *req)
{
    // TODO:
    return send_web_page(req, style_css);
}

esp_err_t favicon_handler(httpd_req_t *req)
{
    const char *filepath = "/spiffs/favicon.ico";

    // Prüfe ob Datei existiert
    struct stat st;
    if (stat(filepath, &st) != 0)
    {
        ESP_LOGW(WEB_TAG, "favicon.ico not found, sending empty response");
        // Sende leere Antwort mit korrektem Status
        httpd_resp_set_status(req, "204 No Content");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }

    FILE *file = fopen(filepath, "rb"); // Wichtig: "rb" für binary mode!
    if (file == NULL)
    {
        ESP_LOGE(WEB_TAG, "Failed to open favicon.ico");
        httpd_resp_set_status(req, "204 No Content");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }

    // Setze Cache-Header (7 Tage = 604800 Sekunden)
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=604800, immutable");

    // Optional: ETag für besseres Caching
    char etag[32];
    snprintf(etag, sizeof(etag), "\"%lld\"", (long long)st.st_mtime);
    httpd_resp_set_hdr(req, "ETag", etag);

    // Setze Content-Type
    httpd_resp_set_type(req, "image/x-icon");

    // Datei in Chunks senden
    char buffer[512];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        if (httpd_resp_send_chunk(req, buffer, bytes_read) != ESP_OK)
        {
            ESP_LOGE(WEB_TAG, "Failed to send favicon chunk");
            fclose(file);
            return ESP_FAIL;
        }
    }

    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0); // Ende der Übertragung

    ESP_LOGI(WEB_TAG, "favicon.ico sent successfully (%ld bytes)", st.st_size);
    return ESP_OK;
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
    config.stack_size = 12288;
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
        httpd_register_uri_handler(server, &uri_style);
        httpd_register_uri_handler(server, &uri_favicon);
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
