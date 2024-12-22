#include <Arduino.h>
#include <string>
#include <HX711.h>
#include <WiFi.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

// PubSubClient | Arduino Documentation
// https://docs.arduino.cc/libraries/pubsubclient/#Releases

// #define LED_PIN 15
// #define HALF_LOOP 500

const char *ssid="Archer-C6-2G";
const char *password="1234567890";
WiFiClient espClient;

// #define MQTT_LUXLED "LuxLED"
#define MQTT_BEERS "my/beers"
const char *mqtt_server = "toad.rmq.cloudamqp.com";
const char* mqtt_user = "uoqpcaih:uoqpcaih";
const char* mqtt_password = "1VxyoFDD97p5zd9EWGGa0qBt6mjTZIAt";
PubSubClient client(espClient);

#define DHTPIN 15
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

HX711 scale;
const int DT_PIN = 18;
const int SCK_PIN = 19;

const int ADIN_1 = 33;  // ADC1
const int VDD = 3.3; // 電源 3.3V
const int ANALOG_MAX = 4096; // 12bit = 4096

long lastMsg = 0;
char msg[50];
int value = 0;

// https://github.com/bogde/HX711/tree/master

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
  }
  Serial.println();
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // client.connect の第2引数と第3引数にユーザー名とパスワードを追加
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(MQTT_BEERS);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(100);
  Serial.printf("%s - run\n",__func__);
  // pinMode(LED_PIN, OUTPUT);

  // pinMode(ADIN_1, ANALOG);
  // analogSetAttenuation(ADC_11db);

  scale.begin(DT_PIN, SCK_PIN);

  dht.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("connecting");
  }
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}


void loop() {
  // put your main code here, to run repeatedly:
  // digitalWrite(LED_PIN, HIGH);
  // Serial.printf("%s - LED_PIN - HIGH\n",__func__);
  // delay(HALF_LOOP);
 
  long value = scale.read_average(5) * -1;
  Serial.print("scale:  ");
  Serial.print(value);
  Serial.println("mV");

  float t = dht.readTemperature();
  float h = dht.readHumidity();
  Serial.println("Temperature: " + String(t, 1) + "°C\tHumidity: " + String(h, 0) + "%");

  // digitalWrite(LED_PIN, LOW);
  // Serial.printf("%s - LED_PIN - LOW\n",__func__);
  // delay(HALF_LOOP);
 
  // int ADC = analogRead(ADIN_1);
  // Serial.print("Llight: ");
  // Serial.print(ADC);
  // Serial.println("mV");

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // char buffer[10]; // 変換後の文字列を格納するための配列 
  // itoa(ADC, buffer, 10); // 10は基数（decimal）
  // itoa(value, buffer, 10); // 10は基数（decimal）

  // JSONドキュメントの作成
  StaticJsonDocument<200> doc;
  // DynamicJsonDocument doc(200);
  doc["scale"] = String(value);
  doc["temperature"] = String(t, 1);
  doc["humidity"] = String(h, 0);

  // doc["scale"] = value;
  // doc["temperature"] = t;
  // doc["humidity"] = h;

  char buffer[200];
  serializeJson(doc, buffer);

  Serial.print("Publishing message: ");
  Serial.println(buffer);

  // MQTTにパブリッシュ
  // client.publish(MQTT_BEERS, buffer);
  client.publish("my/beers/scale", String(value).c_str());
  client.publish("my/beers/temperature", String(t).c_str());
  client.publish("my/beers/humidity", String(h).c_str());

  // 30秒 非同期パブリッシュの待機
  delay(30000);

  // 10秒スリープ
  // esp_sleep_enable_timer_wakeup(10000 * 1000); // 10000ミリ秒をマイクロ秒に変換
  // 30分スリープ
  esp_sleep_enable_timer_wakeup(1800000 * 1000); // 1800000ミリ秒をマイクロ秒に変換

  Serial.println("Going to sleep now");
  esp_deep_sleep_start();

  // 30分
  //delay(300000);

}