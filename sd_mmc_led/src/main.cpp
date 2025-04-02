#include "FS.h"
#include "SD_MMC.h"
#include <ArduinoJson.h>

int clk = 14;
int cmd = 15;
int d0 = 2;
int d1 = 4;
int d2 = 12;
int d3 = 13;

File logFile;
const char* logPath = "/pomiary.txt";

const uint8_t ADC_number = 2;
unsigned long lastMeasure = 0;
unsigned long lastFlush = 0;
String buffer = "";

const int LED_PIN = 27; // Diody LED na GPIO 27 (IO27)

void setup() {
  Serial.begin(115200);
  delay(1500); // Czekamy na ustabilizowanie zasilania i karty

  // Inicjalizacja diody
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Na start zgaszona

  // Pull-upy SD
  pinMode(d0, INPUT_PULLUP);
  pinMode(d1, INPUT_PULLUP);
  pinMode(d2, INPUT_PULLUP);
  pinMode(d3, INPUT_PULLUP);
  pinMode(cmd, INPUT_PULLUP);

  // Próby inicjalizacji SD
  bool sd_ok = false;
  for (int i = 0; i < 10; i++) {
    if (SD_MMC.begin("/sdcard", true, 4000000)) {
      sd_ok = true;
      break;
    }
    Serial.printf("SD init failed (attempt %d), retrying...\n", i + 1);
    delay(500);
  }

  if (!sd_ok) {
    Serial.println("Card Mount Failed after retries");
    return;
  }

  // Jeśli SD działa – zapal diodę
  digitalWrite(LED_PIN, HIGH);

  // Stwórz plik jeśli nie istnieje
  if (!SD_MMC.exists(logPath)) {
    File tmpFile = SD_MMC.open(logPath, FILE_WRITE);
    if (tmpFile) tmpFile.close();
  }

  logFile = SD_MMC.open(logPath, FILE_APPEND);
  if (!logFile) {
    Serial.println("Nie udało się otworzyć pliku do zapisu.");
    return;
  }

  Serial.println("Gotowy do zapisu danych w JSON co 20 ms.");
}

void loop() {
  unsigned long now = millis();

  if (now - lastMeasure >= 1) {
    lastMeasure = now;

    float voltage_measurement[ADC_number] = {12.3, 12.4};
    float current_measurement[ADC_number] = {1.2, 1.3};
    float velocity = 10.5;
    float GPS_speed = 10.5;
    double latitude = 51.107885;
    double longitude = 17.038538;
    float flow = 0.4;
    long measurement_time = 1712068456;
    unsigned long time_ms = millis();

    StaticJsonDocument<512> doc;

    for (uint8_t i = 0; i < ADC_number; i++) {
      doc["pomiar_VT" + String(i)] = voltage_measurement[i];
      doc["pomiar_I" + String(i)] = current_measurement[i];
    }

    doc["predkosc"] = velocity;
    doc["GPS_speed"] = GPS_speed;
    doc["latitude"] = String(latitude, 10);
    doc["longitude"] = String(longitude, 10);
    doc["hydrogen_flow"] = flow;
    doc["czas_pomiaru"] = measurement_time;
    doc["time_ms"] = time_ms;

    String jsonString;
    serializeJson(doc, jsonString);
    buffer += jsonString + "\n";
  }

  if (now - lastFlush >= 200) {
    lastFlush = now;

    if (buffer.length() > 0 && logFile) {
      logFile.print(buffer);
      logFile.flush();
      buffer = "";
    }
  }
}
