#include "httprequest.h"

static const char *TAG = "breadbuddy_http_post";

void http_post_alldata(char *messungsname, float temp_obj, float temp_amb, float humidity, uint32_t widerstand_ohm, uint32_t co2_ppm, uint32_t ethanol_ppm, float ph_value, uint32_t time_remain, uint32_t time_ready, bool baking, probe_data_t *probendaten, char *notizen)
{
    // Erstelle JSON-Objekt
    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return;
    }

    // Füge alle Felder hinzu
    cJSON_AddStringToObject(root, "measurement", messungsname);
    cJSON_AddNumberToObject(root, "temp_obj", temp_obj);
    cJSON_AddNumberToObject(root, "temp_amb", temp_amb);
    cJSON_AddNumberToObject(root, "humidity", humidity);
    cJSON_AddNumberToObject(root, "resistance", widerstand_ohm);
    cJSON_AddNumberToObject(root, "co2", co2_ppm);
    cJSON_AddNumberToObject(root, "ethanol", ethanol_ppm);
    cJSON_AddNumberToObject(root, "ph", ph_value);
    cJSON_AddNumberToObject(root, "time_remain", time_remain);
    cJSON_AddBoolToObject(root, "baking", baking);
    cJSON_AddStringToObject(root, "notes", notizen);

    // Probe-Daten als Sub-Objekt
    cJSON *probe = cJSON_CreateObject();
    cJSON *starter = cJSON_CreateObject();
    cJSON_AddNumberToObject(starter, "menge_g", probendaten->starter.menge_g);
    cJSON_AddNumberToObject(starter, "temperatur_c", probendaten->starter.temperatur_c);
    cJSON_AddItemToObject(probe, "starter", starter);

    cJSON *wasser = cJSON_CreateObject();
    cJSON_AddNumberToObject(wasser, "menge_g", probendaten->wasser.menge_g);
    cJSON_AddNumberToObject(wasser, "temperatur_c", probendaten->wasser.temperatur_c);
    cJSON_AddItemToObject(probe, "wasser", wasser);

    cJSON *mehl = cJSON_CreateObject();
    cJSON_AddNumberToObject(mehl, "menge_g", probendaten->mehl.menge_g);
    cJSON_AddNumberToObject(mehl, "temperatur_c", probendaten->mehl.temperatur_c);
    cJSON_AddItemToObject(probe, "mehl", mehl);

    cJSON *salz = cJSON_CreateObject();
    cJSON_AddNumberToObject(salz, "menge_g", probendaten->salz.menge_g);
    cJSON_AddItemToObject(probe, "salz", salz);

    cJSON *probe_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(probe_obj, "menge_g", probendaten->probe.menge_g);
    cJSON_AddNumberToObject(probe_obj, "temperatur_c", probendaten->probe.temperatur_c);
    cJSON_AddItemToObject(probe, "probe", probe_obj);

    // Probendaten in JSON Objekt einbauen
    cJSON_AddItemToObject(root, "sample_info", probe);

    // JSON zu String konvertieren
    char *json_string = cJSON_PrintUnformatted(root);
    if (json_string == NULL)
    {
        ESP_LOGE(TAG, "Failed to print JSON");
        cJSON_Delete(root);
        return;
    }

    ESP_LOGI(TAG, "JSON: %s\n", json_string);

    // HTTP-Request erstellen
    int content_length = strlen(json_string);
    char *http_request = malloc(content_length + 256);
    if (http_request == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate http_request");
        cJSON_free(json_string);
        cJSON_Delete(root);
        return;
    }

    sprintf(http_request,
            "POST %s HTTP/1.0\r\n"
            "Host: %s:%s\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s",
            WEB_PATH, WEB_SERVER, WEB_PORT, content_length, json_string);

    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

    int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

    if (err != 0 || res == NULL)
    {
        ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
        // CLEANUP!
        free(http_request);
        cJSON_free(json_string);
        cJSON_Delete(root);
        vTaskDelay(pdMS_TO_TICKS(1000));
        return;
    }

    /* Code to print the resolved IP.

       Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
    addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

    s = socket(res->ai_family, res->ai_socktype, 0);
    if (s < 0)
    {
        ESP_LOGE(TAG, "... Failed to allocate socket.");
        freeaddrinfo(res);
        // CLEANUP!
        free(http_request);
        cJSON_free(json_string);
        cJSON_Delete(root);
        vTaskDelay(pdMS_TO_TICKS(1000));
        return;
    }
    ESP_LOGI(TAG, "... allocated socket");

    if (connect(s, res->ai_addr, res->ai_addrlen) != 0)
    {
        ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);
        // CLEANUP!
        free(http_request);
        cJSON_free(json_string);
        cJSON_Delete(root);
        vTaskDelay(pdMS_TO_TICKS(4000));
        return;
    }

    ESP_LOGI(TAG, "... connected");
    freeaddrinfo(res);

    if (write(s, http_request, strlen(http_request)) < 0)
    {
        ESP_LOGE(TAG, "... socket send failed");
        close(s);
        // CLEANUP!
        free(http_request);
        cJSON_free(json_string);
        cJSON_Delete(root);
        vTaskDelay(pdMS_TO_TICKS(4000));
        return;
    }
    ESP_LOGI(TAG, "... socket send success");

    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                   sizeof(receiving_timeout)) < 0)
    {
        ESP_LOGE(TAG, "... failed to set socket receiving timeout");
        close(s);
        // CLEANUP!
        free(http_request);
        cJSON_free(json_string);
        cJSON_Delete(root);
        vTaskDelay(pdMS_TO_TICKS(4000));
        return;
    }
    ESP_LOGI(TAG, "... set socket receiving timeout success");

    /* Read HTTP response */
    do
    {
        bzero(recv_buf, sizeof(recv_buf));
        r = read(s, recv_buf, sizeof(recv_buf) - 1);
        for (int i = 0; i < r; i++)
        {
            putchar(recv_buf[i]);
        }
    } while (r > 0);

    ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
    close(s);

    // Final Cleanup
    free(http_request);
    cJSON_free(json_string);
    cJSON_Delete(root);
}