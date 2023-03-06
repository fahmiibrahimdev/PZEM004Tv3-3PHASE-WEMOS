#include <EEPROM.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PZEM004Tv30.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>

const char* ssid = "SSIDWiFi";
const char* password = "PASSWORDWiFi";
const char* mqtt_server = "your_mqtt_server";

int start1 = 0;
int start2 = 0;
int start3 = 0;

float energyTodayPZEM1 = 0.0;
float energyTodayPZEM3 = 0.0;
float energyTodayPZEM2 = 0.0;

float energyYesterdayPZEM1 = 0.0;
float energyYesterdayPZEM2 = 0.0;
float energyYesterdayPZEM3 = 0.0;

float energyTotalPZEM2 = 0.0;
float energyTotalPZEM1 = 0.0;
float energyTotalPZEM3 = 0.0;

const int RX1 = D1;
const int TX1 = D2;
const int RX2 = D3;
const int TX2 = D4;
const int RX3 = D5;
const int TX3 = D6;

const unsigned long interval = 10000;
unsigned long previousMillis = 0;
unsigned long NTPMillis = 0;

String topic1 = "SMARTHOME/KULKAS";
String topic2 = "SMARTHOME/MAGICOM";
String topic3 = "SMARTHOME/KIPAS";

SoftwareSerial pzem1Serial(RX1, TX1);
SoftwareSerial pzem2Serial(RX2, TX2);
SoftwareSerial pzem3Serial(RX3, TX3);
PZEM004Tv30 pzem1(pzem1Serial);
PZEM004Tv30 pzem2(pzem2Serial);
PZEM004Tv30 pzem3(pzem3Serial);

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long LastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 3600 * 7;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

void setup_wifi() {
  digitalWrite(LED_BUILTIN, HIGH);

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW);  // nyalakan LED
    delay(500);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, HIGH);  // matikan LED
    delay(500);
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi Connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(LED_BUILTIN, LOW);
}

void callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<200> doc;
  // Serial.print("Message Arrived [");
  // Serial.print(topic);
  // Serial.print("] ");
  String msg;

  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  if (start1 == 0) {
    if (strcmp(topic, topic1.c_str()) == 0) {
      if (deserializeJson(doc, msg)) {
        Serial.println(F("Gagal menguraikan pesan JSON"));
        return;
      } else {
        energyYesterdayPZEM1 = doc["ENERGY"]["Yesterday"].as<float>();
        energyTotalPZEM1 = doc["ENERGY"]["Total"].as<float>();
        start1 = 1;
        Serial.println("TOPIC 1 Success set value Yesteday and Total");
      }
    }
  }


  if (start2 == 0) {
    if (strcmp(topic, topic2.c_str()) == 0) {
      if (deserializeJson(doc, msg)) {
        Serial.println(F("Gagal menguraikan pesan JSON"));
        return;
      } else {
        energyYesterdayPZEM2 = doc["ENERGY"]["Yesterday"].as<float>();
        energyTotalPZEM2 = doc["ENERGY"]["Total"].as<float>();
        start2 = 1;
        Serial.println("TOPIC 2 Success set value Yesteday and Total");
      }
    }
  }


  if (start3 == 0) {
    if (strcmp(topic, topic3.c_str()) == 0) {
      if (deserializeJson(doc, msg)) {
        Serial.println(F("Gagal menguraikan pesan JSON"));
        return;
      } else {
        energyYesterdayPZEM3 = doc["ENERGY"]["Yesterday"].as<float>();
        energyTotalPZEM3 = doc["ENERGY"]["Total"].as<float>();
        start3 = 1;
        Serial.println("TOPIC 3 Success set value Yesteday and Total");
      }
    }
  }
}

void reconnect() {

  while (!client.connected()) {
    Serial.print("Attempting MQTT Connection...");
    String clientId = "node-redxx1";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), "username_mqtt", "password_mqtt")) {
      Serial.println("connected");
      subscribe(topic1);
      subscribe(topic2);
      subscribe(topic3);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}

void subscribe(String topic) {
  client.subscribe(topic.c_str());
}

void setup() {
  Serial.begin(115200);
  timeClient.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  while (!Serial) continue;  // Wait until Serial is ready

  pzem1.setAddress(0xF8);
  pzem2.setAddress(0xF9);
  pzem3.setAddress(0xFA);

  Serial.println();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long currentMillis = millis();

  if (currentMillis - NTPMillis >= 1000) {
    NTPMillis = currentMillis;
    timeClient.update();

    if (timeClient.getHours() == 0 && timeClient.getMinutes() == 0 && timeClient.getSeconds() == 0) {
      resetEnergy(pzem1);
      resetEnergy(pzem2);
      resetEnergy(pzem3);
    }

    if (timeClient.getHours() == 23 && timeClient.getMinutes() == 59 && timeClient.getSeconds() == 59) {
      energyYesterdayPZEM1 = energyTodayPZEM1;
      energyYesterdayPZEM2 = energyTodayPZEM2;
      energyYesterdayPZEM3 = energyTodayPZEM3;
    }
  }

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    readAndPrintData(pzem1, 1, energyTodayPZEM1);

    readAndPrintData(pzem2, 2, energyTodayPZEM2);

    readAndPrintData(pzem3, 3, energyTodayPZEM3);
  }
}

void readAndPrintData(PZEM004Tv30 pzem, int init, float energyToday) {

  float voltage = pzem.voltage();
  float current = pzem.current();
  float power = pzem.power();
  float energy = pzem.energy();
  float frequency = pzem.frequency();
  float pf = pzem.pf();
  float energyDiff = energy - energyToday;

  if (isnan(voltage)) {
    Serial.println("Error reading voltage");
  } else if (isnan(current)) {
    Serial.println("Error reading current");
  } else if (isnan(power)) {
    Serial.println("Error reading power");
  } else if (isnan(energy)) {
    Serial.println("Error reading energy");
  } else if (isnan(frequency)) {
    Serial.println("Error reading frequency");
  } else if (isnan(pf)) {
    Serial.println("Error reading power factor");
  } else {

    if (init == 1) {
      energyTodayPZEM1 = voltage;
      if (energyDiff > 0) {
        energyTotalPZEM1 += energyDiff;
      }
      client.publish(topic1.c_str(), JSONEncode(voltage, current, power, energy, energyYesterdayPZEM1, energyTotalPZEM1).c_str(), true);
      Serial.println(JSONEncode(voltage, current, power, energy, energyYesterdayPZEM1, energyTotalPZEM1));
    }

    if (init == 2) {
      energyTodayPZEM2 = voltage;
      if (energyDiff > 0) {
        energyTotalPZEM2 += energyDiff;
      }
      client.publish(topic2.c_str(), JSONEncode(voltage, current, power, energy, energyYesterdayPZEM2, energyTotalPZEM2).c_str(), true);
      Serial.println(JSONEncode(voltage, current, power, energy, energyYesterdayPZEM2, energyTotalPZEM2));
    }

    if (init == 3) {
      energyTodayPZEM3 = voltage;
      if (energyDiff > 0) {
        energyTotalPZEM3 += energyDiff;
      }
      client.publish(topic3.c_str(), JSONEncode(voltage, current, power, energy, energyYesterdayPZEM3, energyTotalPZEM3).c_str(), true);
      Serial.println(JSONEncode(voltage, current, power, energy, energyYesterdayPZEM3, energyTotalPZEM3));
    }
  }

  Serial.println();
}

String JSONEncode(float voltage, float current, float power, float energy, float energyYesterday, float energyTotal) {

  StaticJsonDocument<200> doc;

  doc["ENERGY"]["Voltage"] = String(voltage);
  doc["ENERGY"]["Current"] = String(current);
  doc["ENERGY"]["Power"] = String(power);
  doc["ENERGY"]["Today"] = String(energy, 3);
  doc["ENERGY"]["Yesterday"] = String(energyYesterday, 3);
  doc["ENERGY"]["Total"] = String(energyTotal, 3);

  String output;
  serializeJson(doc, output);
  return output;
}

void resetEnergy(PZEM004Tv30 pzem) {
  pzem.resetEnergy();
}
