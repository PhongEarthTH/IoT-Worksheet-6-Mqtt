#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

const int ledPin = D6;   // กำหนดตามขาที่เชื่อมต่อ LED
bool ledStatus = false;  // false = OFF, true = ON

DHT dht14(D4, DHT11);
ESP8266WebServer server(80);

float tem;
float hum;

const char* ssid = "Galaxy A71 96DE";
const char* password = "25452002";
const char* mqtt_server = "192.168.124.66";
const int mqtt_port = 1883;

HTTPClient http;
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // ตรวจสอบว่าคำสั่งมาจาก MQTT topic "LED" หรือไม่
  if (strcmp(topic, "LED") == 0) {
    // ตรวจสอบคำสั่งที่มาจาก MQTT payload
    if (length == 1) {
      if ((char)payload[0] == '1') {
        digitalWrite(ledPin, LOW);  // เปิดไฟ LED
        Serial.println("LED turned ON");
      } else if ((char)payload[0] == '0') {
        digitalWrite(ledPin, HIGH);  // ปิดไฟ LED
        Serial.println("LED turned OFF");
      }
    }
  }
}


void handleRoot() {
  String html = "<html><head>";
  html += "<meta charset=\"UTF-8\">";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }";
  html += ".button { padding: 10px 20px; background-color: #0099FF; color: white; border: none; border-radius: 5px; cursor: pointer; }";
  html += ".button:hover { background-color: #0077CC; }";
  html += "</style>";
  html += "</head><body>";

  // แสดงสถานะของไฟ LED
  html += "<h2>สถานะไฟ LED D6: " + String(digitalRead(ledPin) == HIGH ? "ON" : "OFF") + "</h2>";
  html += "<a href=\"/toggle\"><button class=\"button\">Switch LED</button></a><br><br>";

  html += "</body></html>";
  server.send(200, "text/html; charset=UTF-8", html);
}



void handleToggle() {
  ledStatus = !ledStatus;
  digitalWrite(ledPin, ledStatus ? LOW : HIGH);  // เปิดหรือปิดไฟ LED ตามสถานะ ledStatus

  // เพิ่มการเปลี่ยนเส้นทาง URL
  server.sendHeader("Location", "/");
  
  server.send(302, "text/plain", ledStatus ? "ON" : "OFF");
  
  // ส่งข้อมูล MQTT
  String mqttMessage = ledStatus ? "on" : "off";
  if (client.publish("LED", mqttMessage.c_str())) {
    Serial.println("MQTT message sent successfully");
  } else {
    Serial.println("Failed to send MQTT message");
  }
}



void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // สมัครสมาชิกหรือ subscribe ไปยัง MQTT topic "LED"
      client.subscribe("LED");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}


void setup() {
  pinMode(ledPin, OUTPUT);  // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port); // เชื่อมต่อ MQTT Broker ด้วยข้อมูลที่ระบุ
  client.setCallback(callback);

  tem = 0.0;
  hum = 0.0;
  dht14.begin();

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.begin();
}


void loop() {
  server.handleClient();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    tem = dht14.readTemperature();
    hum = dht14.readHumidity();

    StaticJsonDocument<200> jsonDocument;
    jsonDocument["humidity"] = hum;
    jsonDocument["temperature"] = tem;  // แก้ชื่อ key เป็น "temperature"
    String jsonData;
    serializeJson(jsonDocument, jsonData);
    client.publish("DHT", jsonData.c_str());
  }
}