#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <WiFiManager.h> 

#define MILL_PIN 4

const char* mqttServer = "your_mqtt_server_address";
const* mqttUser = "your_mqtt_username";
const char* mqttPassword = "your_mqtt_password";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

float millingTime1 = 4.2, millingTime2 = 6.8; 
unsigned int millCount = 0;

void setup() {
  pinMode(MILL_PIN, OUTPUT);
  Serial.begin(9600);
  EEPROM.begin(512); 

  EEPROM.get(0, millingTime1);
  EEPROM.get(sizeof(float), millingTime2);
  EEPROM.get(2*sizeof(float), millCount);

  WiFiManager wifiManager;
  if (!wifiManager.autoConnect("Kaffeemühle")) {
    Serial.println("Failed to connect and hit timeout");
    delay(5000);
    ESP.restart();
  }

  Serial.println("Connected to WiFi");

  mqttClient.setServer(mqttServer, 1883);
  mqttClient.setCallback(mqttCallback);
}

void loop() {
  if (!mqttClient.connected()) {
    reconnectToMqtt();
  }
  mqttClient.loop();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == "millingTime1") {
    millingTime1 = message.toFloat();
    EEPROM.put(0, millingTime1);
    EEPROM.commit();
  } else if (String(topic) == "millingTime2") {
    millingTime2 = message.toFloat();
    EEPROM.put(sizeof(float), millingTime2);
    EEPROM.commit();
  }

  // Increase mill count and send it over MQTT
  millCount++;
  EEPROM.put(2*sizeof(float), millCount);
  EEPROM.commit();
  mqttClient.publish("millCount", String(millCount).c_str());
}

void reconnectToMqtt() {
  while (!mqttClient.connected()) {
    if (mqttClient.connect("CoffeeMill", mqttUser, mqttPassword)) {
      mqttClient.subscribe("millingTime1");
      mqttClient.subscribe("millingTime2");
    } else {
      delay(5000);
    }
  }
}