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

    Phase temp = state->phase;
    // werte nicht stabilisiert = Initialphase

    // widerstand neutral + co2 leichte Steigung = Latenzphase
    if (state->phase == INITIAL && state->trend_resistance < 15 && state->trend_co2 < 150)
    {
        ESP_LOGW("ANALYSIS", "Latenzphase wurde erkannt");
        return LATENZ;
    }

    // widerstand steigung + co2 leichte Steigung = Expansionsphase
    if (state->phase == LATENZ && state->trend_resistance > 15 && state->trend_co2 > 150)
    {
        ESP_LOGW("ANALYSIS", "Expansionsphase wurde erkannt");
        return EXPANSION;
    }

    // widerstand Peak bereits erreicht und negative Steigung + co2 steile Steigung = Emmissionsphase
    if (state->phase == EXPANSION && state->trend_co2 > 700)
    {
        ESP_LOGW("ANALYSIS", "Emmissionsphase wurde erkannt");
        return EMMISION;
    }

    // co2 negative Steigung = Degrationsphase
    if (state->phase == EMMISION && state->trend_co2 < 500)
    {
        ESP_LOGW("ANALYSIS", "Degradationsphase wurde erkannt");
        return DEGRATION;
    }

    return temp;
}

void update_elapsed_time(Measurement_state *state)
{
    time_t now;
    time(&now);

    state->elapsed_time = now - start_time;
    ESP_LOGI("ANALYSIS", "Time NOW %lld", now);
    ESP_LOGI("ANALYSIS", "Time START %lld", start_time);
    ESP_LOGI("ANALYSIS", "Time ELAPSED %lld", state->elapsed_time);
}

/*
TODO: mögliche Berechnungen
- Über Erfahrungswerte bei Gesamtzeit und Temperatur (Stauchungsfaktor)
- Ermitteln in welcher Phase die Messung ist Ende der Phase berechnen + Standardzeit
- Über Eckpunkte (Start, Phasenübergänge, Temperatur, sonstige Indikatoren) einen Stauchungsfaktor bestimmen
- Niveau der aktuellen Werte mit üblichen Maximalwerten (Gesamt und je Phase) vergleichen und Dauer hochrechnen
- Über Zusammensetzung der Probe (Wassergehalt, Startermenge, Salzgehalt)
- Startermenge: Berechnung über exponetielles Wachstum
- Steigung des CO2-Gehalts an Indiz für die Aktivität des Sauerteigs nehmen
- Mehrere Werte ermitteln und gewichtet Mitteln
*/
void calculate_remaining_time(Measurement_state *state)
{
    // TODO: Referenzkurve /-zeiten für eine Sauerteiggährung
    // Dauer - Gesamt bis Backreife
    // Dauer - einzelne Phasen

    // TODO: Faktor nach Temperatur (laut einer Grafik)
    // 25 - 45 Grad ideal 90% bei 35 100%
    // 20 -25 Grad 60%
    // 10-20 Grad 50%
    // Linear von 20/50 auf 35/100
    // lineare Gleichung y = k * x + d
    /*
    float k = 3.33333;
    float d = -16.666667;
    float temperatur = 27.4;
    float effizienz = k * temperatur + d;
    */

    // TODO: Faktor nach Startermenge
    // Referenzkurve mit 15% Starter
    // Vergleich mit 5%, 10%, 20%, 25%
    // lineare ?? Gleichung für Faktor

    // TODO: Faktor nach Wassergehalt
    // Referenzkurve mit 73% Wassergehalt
    // Vergleich mit 65%, 70%, 75%, 80%, 85%
    // Gleichung für Faktor

    // TODO: 1.) Berechnung rein nach Zeit mit Faktor Temperatur und Startermenge

    // TODO: 2.) Berechnung nach Phasen mit Referenzzeiten
    if (state->phase == LATENZ)
    {
        // Steigung von Latenzphase hochrechnen und Standard Expansionsphase dazurechnen
        // Steigung CO2 als Indikator für Aktivität nehmen
        // Faktor für Startermenge
        // Faktor für Temperatur
    }

    if (state->phase == EXPANSION)
    {
        // Steigung von Expansionsphase hochrechnen
        // Steigung CO2 als Indikator für Aktivität nehmen
        // Faktor für Startermenge
        // Faktor für Temperatur
    }

    // TODO: 3.) Berechnung über eintretende Events (Knickpunkt, Temperaturanstieg)

    // TODO: 4.) Berechnung über Niveau, Steigung und übliche Maximalwerte

    // TODO: 5.) Berechnung über exponentielles Wachstum der Starterkultur
    // Referenzzusammensetzung
    // 100% Mehl, 73% Wasser, 15% Starter, 1,8% Salz - Dauer 10 Stunden bis Backreife
    // 15% Starter bedeuten hier 7,9% der Gesamtmasse (100 + 73 + 15 + 1.8 = 189,8) => 15 / 189,8 = 7,9 %
    float starter_anteil = probendaten.starter.menge_g / (probendaten.mehl.menge_g + probendaten.wasser.menge_g + probendaten.starter.menge_g + probendaten.salz.menge_g) * 100.0f;
    float verduennungsfaktor = 100.0f / starter_anteil;
    float verdoppelungen = log2(verduennungsfaktor);
    float verdoppelungszeit = 2.75; // TODO: Dieser Faktor ist die Zeitabhängige komponenten und hier sollten Faktoren wie Temp einfließen
    float gesamtdauer = verdoppelungen * verdoppelungszeit;
    time_t remaining_time_5 = (time_t)(gesamtdauer * 3600) - state->elapsed_time;
    ESP_LOGI("ANALYSIS_5", "Anteil Starter %.2f%%", (probendaten.starter.menge_g / probendaten.mehl.menge_g * 100.0f));
    ESP_LOGI("ANALYSIS_5", "Anteil Starter an Gesamtmenge %.2f%%", starter_anteil);
    ESP_LOGI("ANALYSIS_5", "Verdünnungsfaktor %.2f", verduennungsfaktor);
    ESP_LOGI("ANALYSIS_5", "Verdoppelungen %.2f", verdoppelungen);
    ESP_LOGI("ANALYSIS_5", "Gesamtdauer %.2f Stunden", gesamtdauer);
    ESP_LOGI("ANALYSIS_5", "Verbleibende Zeit %lld", remaining_time_5);

    time_t now;
    time(&now);
    ESP_LOGI("ANALYSIS", "Backzeit %lld", now + remaining_time_5);

    // TODO: gewichteter Mittelwert ermitteln
    if (remaining_time_5 < 0)
    {
        state->remaining_time = 0;
    }
    else
    {
        state->remaining_time = remaining_time_5;
    }
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

void dataanalysis_task(void *pvParameters)
{
    // State initialisieren
    state.elapsed_time = 0;
    state.remaining_time = 0;
    state.phase = INITIAL;

    initialize_dataset_int(&dataset_resistance);
    initialize_dataset_int(&dataset_co2);
    initialize_dataset_float(&dataset_temperature);

    while (true)
    {
        if (xSemaphoreTake(sema_measurement, (TickType_t)100) == pdTRUE)
        {
            // Stack-Nutzung prüfen
            UBaseType_t stackRemaining = uxTaskGetStackHighWaterMark(NULL);
            ESP_LOGW("ANALYSIS", "Stack remaining: %u words", stackRemaining);

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

            update_elapsed_time(&state);      // TODO:
            calculate_remaining_time(&state); // TODO: In dieser Funktion sollten mehrere Berechnungen zusammenfließen

            // TODO: Restzeit als HH:MM formatieren
            if (state.remaining_time == 0)
            {
                // wenn Zeit bereits abgelaufen ist
                strcpy(bake_rest_time, "---");
            }
            else
            {
                int hours = state.remaining_time / 3600;
                int minutes = (state.remaining_time % 3600) / 60;
                snprintf(bake_rest_time, sizeof(bake_rest_time), "%02d:%02d", hours, minutes);
            }

            // Backzeitpunkt berechnen
            time_ready = start_time + state.remaining_time + state.elapsed_time;
            // ESP_LOGI("ANALYSIS", "vorauss. Endzeit: %lld", time_ready);

            localtime_r(&time_ready, &timeinfo);
            strftime(bake_time, sizeof(bake_time), "%H:%M", &timeinfo);

            xSemaphoreGive(sema_measurement);
            vTaskDelay(pdMS_TO_TICKS(messungsDelay / 3));
        }
    }
}