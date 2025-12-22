#include "probe_data.h"
#include "cJSON.h"
#include <string.h>

probe_data_t *probe_data_from_json(const char *json_string)
{
    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL)
        return NULL;

    probe_data_t *data = malloc(sizeof(probe_data_t));
    memset(data, 0, sizeof(probe_data_t));

    // Starter
    cJSON *starter = cJSON_GetObjectItem(root, "starter");
    if (starter)
    {
        data->starter.menge_g = cJSON_GetObjectItem(starter, "menge_g")->valuedouble;
        data->starter.temperatur_c = cJSON_GetObjectItem(starter, "temperatur_c")->valuedouble;
    }

    // Wasser
    cJSON *wasser = cJSON_GetObjectItem(root, "wasser");
    if (wasser)
    {
        data->wasser.menge_g = cJSON_GetObjectItem(wasser, "menge_g")->valuedouble;
        data->wasser.temperatur_c = cJSON_GetObjectItem(wasser, "temperatur_c")->valuedouble;
    }

    // Mehl
    cJSON *mehl = cJSON_GetObjectItem(root, "mehl");
    if (mehl)
    {
        data->mehl.menge_g = cJSON_GetObjectItem(mehl, "menge_g")->valuedouble;
        data->mehl.temperatur_c = cJSON_GetObjectItem(mehl, "temperatur_c")->valuedouble;
    }

    // Salz
    cJSON *salz = cJSON_GetObjectItem(root, "salz");
    if (salz)
    {
        data->salz.menge_g = cJSON_GetObjectItem(salz, "menge_g")->valuedouble;
    }

    // Probe
    cJSON *probe = cJSON_GetObjectItem(root, "probe");
    if (probe)
    {
        data->probe.menge_g = cJSON_GetObjectItem(probe, "menge_g")->valuedouble;
        data->probe.temperatur_c = cJSON_GetObjectItem(probe, "temperatur_c")->valuedouble;
    }

    cJSON_Delete(root);
    return data;
}

char *probe_data_to_json(const probe_data_t *data)
{
    cJSON *root = cJSON_CreateObject();

    cJSON *starter = cJSON_CreateObject();
    cJSON_AddNumberToObject(starter, "menge_g", data->starter.menge_g);
    cJSON_AddNumberToObject(starter, "temperatur_c", data->starter.temperatur_c);
    cJSON_AddItemToObject(root, "starter", starter);

    cJSON *wasser = cJSON_CreateObject();
    cJSON_AddNumberToObject(wasser, "menge_g", data->wasser.menge_g);
    cJSON_AddNumberToObject(wasser, "temperatur_c", data->wasser.temperatur_c);
    cJSON_AddItemToObject(root, "wasser", wasser);

    cJSON *mehl = cJSON_CreateObject();
    cJSON_AddNumberToObject(mehl, "menge_g", data->mehl.menge_g);
    cJSON_AddNumberToObject(mehl, "temperatur_c", data->mehl.temperatur_c);
    cJSON_AddItemToObject(root, "mehl", mehl);

    cJSON *salz = cJSON_CreateObject();
    cJSON_AddNumberToObject(salz, "menge_g", data->salz.menge_g);
    cJSON_AddItemToObject(root, "salz", salz);

    cJSON *probe = cJSON_CreateObject();
    cJSON_AddNumberToObject(probe, "menge_g", data->probe.menge_g);
    cJSON_AddNumberToObject(probe, "temperatur_c", data->probe.temperatur_c);
    cJSON_AddItemToObject(root, "probe", probe);

    // Variante mit Einrückungen
    // char *json_string = cJSON_Print(root);

    // Variante mit möglichst kompakter Darstellung
    char *json_string = cJSON_PrintUnformatted(root);

    cJSON_Delete(root);
    return json_string;
}