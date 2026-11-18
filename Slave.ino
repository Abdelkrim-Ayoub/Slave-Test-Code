
#include <WiFi.h>
#include <PubSubClient.h>

// Wi-Fi provided by Wokwi
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// Temporary public MQTT broker
const char* MQTT_SERVER = "broker.hivemq.com";
const int MQTT_PORT = 1883;

// Change this for the second ESP32
const char* DEVICE_ID = "slave01";

// Use the SAME prefix in the Kali master code
const char* PREFIX = "prefix for slave i removed it cause it point of access";

WiFiClient espClient;
PubSubClient mqtt(espClient);

String telemetryTopic;
String commandTopic;
String ackTopic;

unsigned long lastPublish = 0;
bool pumpState = false;

void publishAck(const char* action) {
  String payload = "{";
  payload += "\"device\":\"" + String(DEVICE_ID) + "\",";
  payload += "\"action\":\"" + String(action) + "\",";
  payload += "\"pump_state\":";
  payload += pumpState ? "true" : "false";
  payload += "}";

  mqtt.publish(ackTopic.c_str(), payload.c_str());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";

  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println("Command received: " + message);

  if (message.indexOf("PUMP_ON") >= 0) {
    pumpState = true;
    publishAck("PUMP_ON");
  }
  else if (message.indexOf("PUMP_OFF") >= 0) {
    pumpState = false;
    publishAck("PUMP_OFF");
  }
}

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.println("Connecting to MQTT...");

    if (mqtt.connect(DEVICE_ID)) {
      Serial.println("MQTT connected");

      mqtt.subscribe(commandTopic.c_str());

      Serial.println("Subscribed: " + commandTopic);
    } else {
      Serial.print("MQTT error: ");
      Serial.println(mqtt.state());
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  telemetryTopic =
    String(PREFIX) + "/" + DEVICE_ID + "/telemetry";

  commandTopic =
    String(PREFIX) + "/" + DEVICE_ID + "/command";

  ackTopic =
    String(PREFIX) + "/" + DEVICE_ID + "/ack";

  connectWiFi();

  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(callback);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqtt.connected()) {
    connectMQTT();
  }

  mqtt.loop();

  if (millis() - lastPublish >= 5000) {
    lastPublish = millis();

    // Simulated sensor readings
    float temperature = random(220, 300) / 10.0;
    float humidity = random(500, 800) / 10.0;
    float ph = random(550, 700) / 100.0;
    float ec = random(100, 200) / 100.0;

    String payload = "{";
    payload += "\"device\":\"" + String(DEVICE_ID) + "\",";
    payload += "\"temperature\":" + String(temperature, 1) + ",";
    payload += "\"humidity\":" + String(humidity, 1) + ",";
    payload += "\"ph\":" + String(ph, 2) + ",";
    payload += "\"ec\":" + String(ec, 2);
    payload += "}";

    mqtt.publish(telemetryTopic.c_str(), payload.c_str());

    Serial.println("Telemetry published:");
    Serial.println(payload);
  }
}
