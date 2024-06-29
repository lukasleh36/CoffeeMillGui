#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h> // Schritt 1: mDNS-Bibliothek einbinden

#define EEPROM_SIZE 512
#define MILL_TIME1_ADDR sizeof(unsigned int)
#define MILL_TIME2_ADDR MILL_TIME1_ADDR + sizeof(float)

const char* ssid = "CoffeeMill(f)";

ESP8266WebServer server(80);

int MILL_PIN = D1;
int BTN1_PIN = D5;
int BTN2_PIN = D6;
float millingTime1 = 4.2, millingTime2 = 6.8;

void setup() {
  Serial.begin(9600);
  EEPROM.begin(EEPROM_SIZE);
  loadFromEEPROM();

  pinMode(MILL_PIN, OUTPUT);
  pinMode(BTN1_PIN, INPUT_PULLUP);
  pinMode(BTN2_PIN, INPUT_PULLUP);

  WiFi.softAP(ssid);
  Serial.println("Access Point gestartet");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.softAPIP());

  if (!MDNS.begin("coffee")) { // Schritt 2: mDNS-Service starten
    Serial.println("Fehler beim Einrichten von mDNS");
  }
  MDNS.addService("http", "tcp", 80);

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", getHTML());
  });

  server.on("/set", HTTP_POST, []() {
  String time1 = server.arg("millingTime1");
  String time2 = server.arg("millingTime2");
  millingTime1 = time1.toFloat();
  millingTime2 = time2.toFloat();
  saveToEEPROM();
  server.sendHeader("Location", "/?saved=true"); // Fügt den Query-Parameter hinzu
  server.send(303); // HTTP 303 See Other
});

  server.begin();
  Serial.println("HTTP server gestartet");
}

void loop() {
  server.handleClient();
  MDNS.update(); // Schritt 3: mDNS-Service im Loop halten
  checkButtons();
}

// Der Rest des Codes bleibt unverändert...

void checkButtons() {
  if (digitalRead(BTN1_PIN) == LOW) {
    startMilling(millingTime1);
  }
  if (digitalRead(BTN2_PIN) == LOW) {
    startMilling(millingTime2);
  }
}

void startMilling(float time) {
  digitalWrite(MILL_PIN, HIGH);
  delay(time * 1000); // Zeit in Millisekunden umwandeln
  digitalWrite(MILL_PIN, LOW);
}

String getHTML() {
    String html = "<!DOCTYPE html>"
                  "<html><head><title>Grinder Configuration</title>"
                  "<meta name='viewport' content='width=device-width, initial-scale=0.7'>"
                  "<link href='https://fonts.googleapis.com/css2?family=Roboto:wght@400;500&display=swap' rel='stylesheet'>"
                  "<style>"
                  "body {font-family: 'Roboto', sans-serif; margin: 0; padding: 0; display: flex; justify-content: center; align-items: center; height: 80%; background-color: #f5f5f5;}"
                  "form {background-color: white; padding: 20px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.2); transition: box-shadow 0.3s; width: 90%; max-width: 90%;}"
                  "form:hover {box-shadow: 0 8px 16px rgba(0,0,0,0.2);}"
                  "h1 {color: #333; font-size: 24px;}" // Schriftgröße für Überschriften erhöht
                  "input[type='text'], input[type='submit'] {display: block; width: calc(100% - 20px); margin: 10px 0; padding: 10px; border-radius: 4px; border: 1px solid #ccc; font-size: 18px;}" // Schriftgröße für Eingabeelemente erhöht
                  "input[type='submit'] {background-color: #4CAF50; color: white; border: none; cursor: pointer; transition: background-color 0.3s ease;}"
                  "input[type='submit']:hover {background-color: #45a049;}"
                  ".confirmation {opacity: 0; transition: opacity 2s ease-in-out; background-color: #4CAF50; color: white; padding: 10px; margin: 20px 0; border-radius: 4px; animation: fadeIn 2s forwards;}"
                  "@keyframes fadeIn {from {opacity: 0;} to {opacity: 1;}}"
                  "@media (max-width: 600px) {body {flex-direction: column;}}"
                  "</style></head>"
                  "<body><div><h1>Grinder Times Configuration</h1>";
  if (server.hasArg("saved") && server.arg("saved") == "true") {
    html += "<div class='confirmation' style='display:none;' id='confirmationMessage'>Settings saved successfully!</div>"
            "<script>"
            "document.addEventListener('DOMContentLoaded', function() {"
            "  document.getElementById('confirmationMessage').style.display='block';"
            "});"
            "</script>";
  }
  html += "<form action='/set' method='POST'>"
          "Grinder Time 1:<input type='text' name='millingTime1' value='" + String(millingTime1) + "'><br>"
          "Grinder Time 2:<input type='text' name='millingTime2' value='" + String(millingTime2) + "'><br>"
          "<input type='submit' value='Save'>"
          "</form></div></body></html>";
  return html;
}

void saveToEEPROM() {
  EEPROM.put(MILL_TIME1_ADDR, millingTime1);
  EEPROM.put(MILL_TIME2_ADDR, millingTime2);
  EEPROM.commit();
}

void loadFromEEPROM() {
  EEPROM.get(MILL_TIME1_ADDR, millingTime1);
  EEPROM.get(MILL_TIME2_ADDR, millingTime2);
}