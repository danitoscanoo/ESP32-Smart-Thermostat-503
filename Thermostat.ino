#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h> 
#include "time.h"
#include <WebServer.h>
#include <esp_task_wdt.h>

// ================= USER CONFIGURATION =================
const char* ssid     = "WRITE_HERE_YOU_WIFI_SSID";
const char* password = "WRITE_HERE_YOUR_WIFI_PASSWORD";

// ================= PIN DEFINITIONS =================
#define RELAY_PIN 26

// LinE I2C 1 (Sensor BMP280)
#define I2C_SDA_SENS 21
#define I2C_SCL_SENS 22

// Line I2C 2 (Display OLED)
#define I2C_SDA_OLED 33
#define I2C_SCL_OLED 32

// ================= OBJECT AND VARIABLE =================
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire1, -1);

Adafruit_BMP280 bmp; 

// Web Server on port 80
WebServer server(80);

// Watchdog (60 seconds off timeout)
#define WDT_TIMEOUT 60 

float targetTemp = 20.0;
float currentTemp = 0.0;
bool relayState = false;
String systemMode = "AUTO";
String currentTimeString = "--:--";

// Temperature settings
const float TEMP_COMFORT = 20.5;
const float TEMP_ECO = 17.0;
const float ISTERESI = 0.5;

// NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;      // GMT+1
const int   daylightOffset_sec = 3600;

unsigned long lastWifiCheck = 0;
const unsigned long wifiCheckInterval = 15 * 60 * 1000; // 15 min
int wifiRetryCount = 0;

// ================= ICONS (Bitmaps) =================
const unsigned char bitmap_wifi [] PROGMEM = { 
  0x3C, 0x00, 0x7E, 0x00, 0xE7, 0x00, 0xC3, 0x00, 0x81, 0x00, 0x24, 0x00, 0x00, 0x00, 0x18, 0x00 
};
const unsigned char bitmap_fire [] PROGMEM = { 
  0x08, 0x00, 0x1C, 0x00, 0x36, 0x00, 0x36, 0x00, 0x77, 0x00, 0x7F, 0x00, 0x7F, 0x00, 0x3E, 0x00, 0x1C, 0x00, 0x08, 0x00 
};

// ================= WEB PAGE (HTML) =================
String getHTML() {
  String ptr = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  ptr += "<meta http-equiv='refresh' content='10'>"; // Ricarica ogni 10s
  ptr += "<title>Termostato Home</title>";
  ptr += "<style>body{font-family:sans-serif;text-align:center;background:#f4f4f4;padding:20px;}";
  ptr += ".card{background:#fff;padding:30px;border-radius:15px;box-shadow:0 4px 8px rgba(0,0,0,0.1);max-width:400px;margin:auto;}";
  ptr += "h1{color:#333;margin-bottom:10px;}";
  ptr += ".val{font-size:3em;font-weight:bold;color:#007BFF;}";
  ptr += ".status{color:#28a745;font-weight:bold;font-size:1.2em;}";
  ptr += ".off{color:#dc3545;font-weight:bold;font-size:1.2em;}";
  ptr += "button{padding:10px 20px;font-size:16px;margin:5px;border:none;border-radius:5px;cursor:pointer;background:#007BFF;color:white;}";
  ptr += "input{padding:10px;font-size:16px;width:80px;text-align:center;}";
  ptr += "</style></head><body>";
  
  ptr += "<div class='card'><h1>Termostato</h1>";
  ptr += "<p style='color:#777;'>" + currentTimeString + "</p>";
  ptr += "<div><span class='val'>" + String(currentTemp, 1) + "&deg;C</span></div>";
  
  ptr += "<p>Target: <b>" + String(targetTemp, 1) + "&deg;C</b> (" + systemMode + ")</p>";
  
  ptr += "<p>Stato Caldaia: ";
  if(relayState) ptr += "<span class='status'>ACCESA &#128293;</span>"; // Icona fuoco
  else ptr += "<span class='off'>SPENTA</span>";
  ptr += "</p>";
  
  ptr += "<hr><form action='/set' method='GET'>Manuale: <input name='val' type='number' step='0.1' value='" + String(targetTemp, 1) + "'> <button>SET</button></form>";
  ptr += "<br><form action='/auto'><button style='background:#6c757d;'>Torna ad AUTO</button></form>";
  ptr += "</div></body></html>";
  return ptr;
}

void handleRoot() { server.send(200, "text/html", getHTML()); }
void handleSet() { 
  if(server.hasArg("val")) { 
    targetTemp = server.arg("val").toFloat(); 
    systemMode = "MANUAL"; 
  } 
  server.sendHeader("Location", "/"); server.send(302, "text/plain", ""); 
}
void handleAuto() { 
  systemMode = "AUTO"; 
  server.sendHeader("Location", "/"); server.send(302, "text/plain", ""); 
}

// ================= DISPLAY =================
void updateOLED() {
  display.clearDisplay();

  display.setTextSize(1); display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0); display.print(currentTimeString);

  if (WiFi.status() == WL_CONNECTED) display.drawBitmap(110, 0, bitmap_wifi, 10, 8, SH110X_WHITE);
  else { display.setCursor(110, 0); display.print("NO"); }

  display.setTextSize(3); 
  display.setCursor(25, 20); 
  display.print(currentTemp, 1);
  display.setTextSize(1); 
  display.print("C");

  display.setTextSize(1);
  display.setCursor(0, 54);
  display.print("Set:"); display.print(targetTemp, 1);

  if (relayState) {
    display.setCursor(80, 54); display.print("ON");
    display.drawBitmap(100, 52, bitmap_fire, 10, 10, SH110X_WHITE);
  } else {
    display.setCursor(90, 54); display.print("OFF");
  }
  display.display();
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  Wire.begin(I2C_SDA_SENS, I2C_SCL_SENS);
  Wire1.begin(I2C_SDA_OLED, I2C_SCL_OLED);

  // Watchdog Setup
  esp_task_wdt_config_t wdt_config = { .timeout_ms = WDT_TIMEOUT * 1000, .idle_core_mask = (1 << 0), .trigger_panic = true };
  esp_task_wdt_init(&wdt_config); 
  esp_task_wdt_add(NULL);

  if(!display.begin(0x3C, true)) { // 0x3C address, usa Wire1
    Serial.println(F("ERRORE: Display non trovato su Wire1!"));
  }
  display.clearDisplay();
  display.setTextSize(1); display.setTextColor(SH110X_WHITE);
  display.setCursor(10, 20); display.print("Avvio sistema...");
  display.display();

  if (!bmp.begin(0x76)) {  
    Serial.println("ERRORE: Sensore BMP280 non trovato! Controlla cavi.");
    display.setCursor(10, 40); display.print("Err Sensore!"); display.display();
  }

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     
                  Adafruit_BMP280::SAMPLING_X2,     
                  Adafruit_BMP280::SAMPLING_X16,    
                  Adafruit_BMP280::FILTER_X16,      
                  Adafruit_BMP280::STANDBY_MS_500); 


  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  display.clearDisplay();
  display.setCursor(0, 20); display.print("Connessione WiFi");
  display.display();

  int tentativi = 0;
  while (WiFi.status() != WL_CONNECTED && tentativi < 20) {
    delay(500);
    Serial.print(".");
    display.print("."); display.display();
    tentativi++;
    esp_task_wdt_reset(); // Resetta watchdog mentre aspetta
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/auto", handleAuto);
  server.begin();
  
  Serial.println("\nSistema Avviato!");
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());
}

// ================= LOOP =================
void loop() {
  // Reset Watchdog
  esp_task_wdt_reset();

  server.handleClient();

  float lettura = bmp.readTemperature();
  // Semplice controllo anti-errore (se da nan, teniamo il valore vecchio)
  if (!isnan(lettura)) {
    currentTemp = lettura;
  }

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char timeStrBuff[6];
    strftime(timeStrBuff, sizeof(timeStrBuff), "%H:%M", &timeinfo);
    currentTimeString = String(timeStrBuff);

    // AUTO logic: 09:00 -> 18:00 ECO, AFTER -> COMFORT
    if (systemMode == "AUTO") {
      if (timeinfo.tm_hour >= 9 && timeinfo.tm_hour < 18) {
        targetTemp = TEMP_ECO;
      } else {
        targetTemp = TEMP_COMFORT;
      }
    }
  } else {
    currentTimeString = "--:--";
  }

  if (currentTemp < (targetTemp - ISTERESI)) {
    digitalWrite(RELAY_PIN, LOW);
    relayState = true;
  } 
  else if (currentTemp >= targetTemp) {
    digitalWrite(RELAY_PIN, HIGH);
    relayState = false;
  }

  if (millis() - lastWifiCheck > wifiCheckInterval) {
    lastWifiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.disconnect(); 
      WiFi.reconnect(); 
      wifiRetryCount++;
      if (wifiRetryCount >= 4) ESP.restart();
    } else {
      wifiRetryCount = 0;
    }
  }

  updateOLED();

  delay(200); 
}