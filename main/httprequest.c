#include "httprequest.h"

static const char *TAG = "breadbuddy_http_post";

void http_post_alldata(char *messungsname, float temp_obj, float temp_amb, float humidity, uint32_t widerstand_ohm, uint32_t co2_ppm, uint32_t ethanol_ppm, float ph_value, uint32_t time_remain, uint32_t time_ready, bool baking, probe_data_t *probendaten, char *notizen)
{
    char json_data[6144];
    strcpy(json_data, "{\"measurement\": \"");
    strcat(json_data, messungsname);

    strcat(json_data, "\",\"temp_obj\": ");
    char temp_obj_str[20];
    sprintf(temp_obj_str, "%.2f", temp_obj);
    strcat(json_data, temp_obj_str);

    strcat(json_data, ",\"temp_amb\": ");
    char temp_amb_str[20];
    sprintf(temp_amb_str, "%.2f", temp_amb);
    strcat(json_data, temp_amb_str);

    strcat(json_data, ",\"humidity\": ");
    char humidity_str[20];
    sprintf(humidity_str, "%.2f", humidity);
    strcat(json_data, humidity_str);

    strcat(json_data, ",\"resistance\": ");
    char resistance_str[20];
    sprintf(resistance_str, "%lu", widerstand_ohm);
    strcat(json_data, resistance_str);

    strcat(json_data, ",\"co2\": ");
    char co2_str[20];
    sprintf(co2_str, "%lu", co2_ppm);
    strcat(json_data, co2_str);

    strcat(json_data, ",\"ethanol\": ");
    char ethanol_str[20];
    sprintf(ethanol_str, "%lu", ethanol_ppm);
    strcat(json_data, ethanol_str);

    strcat(json_data, ",\"ph\": ");
    char ph_str[20];
    sprintf(ph_str, "%.2f", ph_value);
    strcat(json_data, ph_str);

    strcat(json_data, ",\"time_remain\": ");
    char time_remain_str[20];
    sprintf(time_remain_str, "%lu", time_remain);
    strcat(json_data, time_remain_str);

    /*
    strcat(json_data, ",\"time_ready\": ");
    char time_ready_str[20];
    sprintf(time_ready_str, "%lu", time_ready);
    strcat(json_data, time_ready_str);
    */

    strcat(json_data, ",\"baking\": ");
    char baking_str[20];
    // TODO: prüfen ob das auch so funktioniert mit dem Boolean wert im json
    sprintf(baking_str, "%s", baking ? "true" : "false");
    strcat(json_data, baking_str);

    strcat(json_data, ",\"sample_info\": ");
    char sample_info_str[4096];
    // TODO: prüfen!!!
    sprintf(sample_info_str, "%s", probe_data_to_json(probendaten));
    strcat(json_data, sample_info_str);

    strcat(json_data, ",\"notes\": ");
    char notes_str[1024];
    // TODO: prüfen!!!
    sprintf(notes_str, "\"%s\"", notizen);
    strcat(json_data, notes_str);

    strcat(json_data, "}");

    // TODO: Ausgabe des Json um es zu kontrollieren - DEBUGGING
    printf("%s\n", json_data);

    int content_length = strlen(json_data);

    char http_request[6500];
    sprintf(http_request,
            "POST %s HTTP/1.0\r\n"
            "Host: %s:%s\r\n"
            //"User-Agent: esp-idf/1.0 esp32\r\n" // braucht man nur wenn der Server das verlangt
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n" // das ist unerlässlich
            //"Prefer: return=representation\r\n" // optional wenn man nochmal die Daten zurückhaben will
            "\r\n"
            "%s",
            WEB_PATH, WEB_SERVER, WEB_PORT, content_length, json_data);

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
        vTaskDelay(pdMS_TO_TICKS(1000));
        return;
    }
    ESP_LOGI(TAG, "... allocated socket");

    if (connect(s, res->ai_addr, res->ai_addrlen) != 0)
    {
        ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);
        vTaskDelay(pdMS_TO_TICKS(4000));
        return;
    }

    ESP_LOGI(TAG, "... connected");
    freeaddrinfo(res);

    if (write(s, http_request, strlen(http_request)) < 0)
    {
        ESP_LOGE(TAG, "... socket send failed");
        close(s);
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
}