#include "analysis.h"

void initialize_dataset_int(Dataset_int *ds)
{
    for (int i = 0; i < VALUECOUNT; i++)
    {
        ds->data[i] = 0;
    }
    ds->count = 0;
}

void initialize_dataset_float(Dataset_float *ds)
{
    for (int i = 0; i < VALUECOUNT; i++)
    {
        ds->data[i] = 0.0;
    }
    ds->count = 0;
}

uint8_t update_dataset_int(Dataset_int *ds, uint32_t value)
{
    if (ds->count == VALUECOUNT)
    {
        return 1;
    }

    ds->data[ds->count] = value;
    ds->count++;

    return 0;
}

uint8_t update_dataset_float(Dataset_float *ds, float value)
{
    if (ds->count == VALUECOUNT)
    {
        return 1;
    }

    ds->data[ds->count] = value;
    ds->count++;

    return 0;
}

// Berechnet die Steigung der letzten X Punkte
float calculate_trend_int(Dataset_int *ds)
{
    if (ds->count < WINDOW_SIZE)
        return 0.0f;

    float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    int n = WINDOW_SIZE;

    // Wir nehmen nur die letzten 'n' Punkte aus dem Array
    int start_index = ds->count - n;

    for (int i = 0; i < n; i++)
    {
        float x = (float)i; // Zeitindex (0, 1, 2, 3)
        float y = (float)ds->data[start_index + i];

        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }

    float m_slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
    return m_slope; // Änderung pro 15-Minuten-Intervall
}

float calculate_trend_float(Dataset_float *ds)
{
    if (ds->count < WINDOW_SIZE)
        return 0.0f;

    float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    int n = WINDOW_SIZE;

    // Wir nehmen nur die letzten 'n' Punkte aus dem Array
    int start_index = ds->count - n;

    for (int i = 0; i < n; i++)
    {
        float x = i; // Zeitindex (0, 1, 2, 3)
        float y = ds->data[start_index + i];

        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }

    float m_slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
    return m_slope; // Änderung pro 15-Minuten-Intervall
}

uint32_t get_latest_value_int(Dataset_int *ds)
{
    if (ds->count < 1)
    {
        return 0;
    }
    else
    {
        return ds->data[ds->count];
    }
}

float get_latest_value_float(Dataset_float *ds)
{
    if (ds->count < 1)
    {
        return 0;
    }
    else
    {
        return ds->data[ds->count];
    }
}

Phase calculate_phase(Measurement_state *state)
{
    Phase temp = {0, 0, 0, 0, 0, 0, INITIAL, 0, 0};
    return temp;
}
time_t update_elapsed_time(Measurement_state *state)
{
    time_t temp = 0;
    return temp;
}
time_t calculate_remaining_time(Measurement_state *state)
{
    time_t temp = 0;
    return temp;
}

void check_events(Dataset_int *ds)
{
    if (ds->count < WINDOW_SIZE)
        return;

    float current_val = ds->data[ds->count - 1];
    float slope = calculate_trend_int(ds);

    // EVENT 1: Absolute Grenze (z.B. Überhitzung/Überlauf)
    if (current_val > 50.0)
    {
        printf("ALARM: Schwellenwert ueberschritten (%.2f)!\n", current_val);
    }

    // EVENT 2: Zu schneller Anstieg (Trend-Check)
    // Wenn slope > 2.0 bedeutet das: Anstieg von mehr als 2 Einheiten pro 15 Min
    if (slope > 2.0)
    {
        printf("WARNUNG: Wert steigt zu schnell (Steigung: %.2f)!\n", slope);
    }

    // EVENT 3: Ende-Erkennung (Stagnation)
    // Wenn die Steigung fast Null ist (nahe Plateau)
    if (slope > -0.05 && slope < 0.05 && ds->count > 10)
    {
        printf("INFO: Messung stabilisiert sich. Ende erreicht.\n");
    }
}

void task_dataanalysis(void *pvParameters)
{

    Measurement_state state;
    state.elapsed_time = 0;
    // state.remaining_time = 0;

    while (true)
    {
        if (xSemaphoreTake(sema_measurement, (TickType_t)100) == pdTRUE)
        {
            // Resistance-Dataset
            state.trend_resistance = calculate_trend_int(&dataset_resistance);
            state.value_resistance = get_latest_value_int(&dataset_resistance);

            // CO2-Dataset
            state.trend_co2 = calculate_trend_int(&dataset_co2);
            state.value_co2 = get_latest_value_int(&dataset_co2);

            // Temperature-Dataset
            state.trend_temperature = calculate_trend_float(&dataset_temperature);
            state.value_temperature = get_latest_value_float(&dataset_temperature);

            state.phase = calculate_phase(&state); // TODO: Phasen definieren über Trends und Werte (evt. mehrere Trend mit unterschiedlichen Zeiträumen (Lange, mittel, kurz))
            // eventuell als statemashine schreiben

            // TODO: eventuell in struct auch notieren wann jede Phase begonnen hat um hochrechnen zu können

            state.elapsed_time = update_elapsed_time(&state);        // TODO:
            state.remaining_time = calculate_remaining_time(&state); // TODO: In dieser Funktion sollten mehreer Berechnungen zusammenfließen

            /*
            TODO: mögliche Berechnungen
            - Über Erfahrungswerte bei Gesamtzeit und Temperatur (Stauchungsfaktor)
            - Ermitteln in welcher Phase die Messung ist Ende der Phase berechnen + Standardzeit
            - Über Eckpunkte (Start, Phasenübergänge, Temperatur, sonstige Indikatoren) einen Stauchungsfaktor bestimmen
            - Niveau der aktuellen Werte mit üblichen Maximalwerten (Gesamt und je Phase) vergleichen und Dauer hochrechnen
            - Über Zusammensetzung der Probe (Wassergehalt, Startermenge, Salzgehalt)
            - Startermenge: Berechnung über exponetielles Wachstum



            */

            xSemaphoreGive(sema_measurement);
            // TODO: vTaskDelay(pdMS_TO_TICKS(messungsDelay));
        }
    }
}