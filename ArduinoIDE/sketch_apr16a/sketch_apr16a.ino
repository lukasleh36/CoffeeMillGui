#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <WiFiManager.h> 
#include <EEPROM.h> // EEPROM-Bibliothek hinzufügen

#define EEPROM_SIZE 512 // Definieren Sie die Größe des EEPROMs
#define MILL_COUNT_ADDR 0 // Definieren Sie die Adresse für millCount
#define MILL_TIME1_ADDR sizeof(unsigned int) // Definieren Sie die Adresse für millingTime1
#define MILL_TIME2_ADDR MILL_TIME1_ADDR + sizeof(float) // Definieren Sie die Adresse für millingTime2

int MILL_PIN = 2;
int BTN1_PIN = 12;
int BTN2_PIN = 13;

int BTN1_STATE = 0;
int BTN2_STATE = 0;

char mqtt_server[40] = "192.168.2.12";
char mqtt_port[6] = "1883";
char mqtt_user[40] = "mqtt-user"; 
char mqtt_password[40];

WiFiClient espClient;
PubSubClient mqttClient(espClient);

float millingTime1 = 4.2, millingTime2 = 6.8; 
unsigned int millCount = 0;

void setup() {
  EEPROM.begin(EEPROM_SIZE); // Initialisieren Sie das EEPROM
  loadFromEEPROM(); // Laden Sie die Werte aus dem EEPROM

  pinMode(MILL_PIN, OUTPUT);
  pinMode(BTN1_PIN, INPUT_PULLUP);
  pinMode(BTN2_PIN, INPUT_PULLUP);
  Serial.begin(9600);

  WiFiManager wifiManager;

  // add custom parameters
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, 40);
  WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password, 40);

  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);

  if (!wifiManager.autoConnect("Kaffeemühle")) {
    Serial.println("Failed to connect and hit timeout");
    delay(5000);
    ESP.restart();
  }

  // read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_password, custom_mqtt_password.getValue());

  Serial.println("Connected to wifi");
  Serial.println("MQTT server: " + String(mqtt_server));
  Serial.println("MQTT port: " + String(mqtt_port));
  Serial.println("MQTT user: " + String(mqtt_user));
  Serial.println("MQTT password: " + String(mqtt_password));

  mqttClient.setServer(mqtt_server, atoi(mqtt_port));
  mqttClient.setCallback(mqttCallback);
}

void loop() {
    BTN1_STATE = digitalRead(BTN1_PIN);
    BTN2_STATE = digitalRead(BTN2_PIN);
  if (!mqttClient.connected()) {
    Serial.println("MQTT not connected, trying to reconnect...");
    reconnectToMqtt();
  }
  mqttClient.loop();
  if (BTN1_STATE == LOW || BTN2_STATE == LOW) {
    digitalWrite(MILL_PIN, HIGH); 
    if(BTN1_STATE == LOW) {
      delay(static_cast<int>(millingTime1 * 1000));
    } else {
      delay(static_cast<int>(millingTime2 * 1000));
    }
    digitalWrite(MILL_PIN, LOW); 
    Serial.println("Button Pressed with millingTime1: " + String(static_cast<int>(millingTime1 * 1000)) + " millingTime2: " + String(static_cast<int>(millingTime2 * 1000)) + " millCount: " + String(millCount));
    millCount++;
    saveToEEPROM(); // Speichern Sie die Werte im EEPROM
    mqttClient.publish("millCount", String(millCount).c_str());
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println("Received message on topic: " + String(topic));
  Serial.println("Message: " + message);

  if (String(topic) == "millingTime1") {
    millingTime1 = message.toFloat();
    saveToEEPROM(); // Speichern Sie die Werte im EEPROM
    Serial.println("millingTime1: " + String(millingTime1));
  } else if (String(topic) == "millingTime2") {
    millingTime2 = message.toFloat();
    saveToEEPROM(); // Speichern Sie die Werte im EEPROM
    Serial.println("millingTime2: " + String(millingTime2));
  } else if (String(topic) == "refreshMillingTimes") {
    mqttClient.publish("millingTime1", String(millingTime1).c_str());
    mqttClient.publish("millingTime2", String(millingTime2).c_str());
  }
}

void reconnectToMqtt() {
  while (!mqttClient.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (mqttClient.connect("CoffeeMill", mqtt_user, mqtt_password)) {
      Serial.println("MQTT connected");
      mqttClient.subscribe("millingTime1");
      mqttClient.subscribe("millingTime2");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void saveToEEPROM() {
  EEPROM.put(MILL_COUNT_ADDR, millCount);
  EEPROM.put(MILL_TIME1_ADDR, millingTime1);
  EEPROM.put(MILL_TIME2_ADDR, millingTime2);
  EEPROM.commit();
}

void loadFromEEPROM() {
  EEPROM.get(MILL_COUNT_ADDR, millCount);
  EEPROM.get(MILL_TIME1_ADDR, millingTime1);
  EEPROM.get(MILL_TIME2_ADDR, millingTime2);
}