/*
  NOTES
  1. Set NODE_ID and NODE_ACCESS_TOKEN (main.cpp)
  2. Set MIN_HOP (LoRaSENSE.h)
  3. Set DATA_TESTING to true if testing with randomized sensor values
*/

#include <Arduino.h>
#include "LoRaSENSE.h"

//Sensor libraries
#include "DHT.h"
#include "MQ7.h"

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Sensor constants
#define DHT_PIN 35
#define DHT_TYPE 22
#define MQ7_PIN 34
#define MQ7_VCC 5.0

//OLED pins
#define OLED_SDA 21
#define OLED_SCL 22 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

//Constants
#define NODE_ID 0xAAAAAAAA
// #define NODE_ACCESS_TOKEN "wGkmunxRiUWWfaLkLu8q"  // Thingsboard access token for node A
// #define NODE_ID 0xBBBBBBBB
// #define NODE_ACCESS_TOKEN "u24bOqqfCGKZ4IMc0M6j"  // Thingsboard access token for node B
// #define NODE_ID 0xCCCCCCCC
// #define NODE_ACCESS_TOKEN "XWJo5u7tAyvPGnduuqOa"  // Thingsboard access token for node C
#define CYCLE_TIME 10000     // 10s, for testing only!!

//Debugging
#define DATA_TESTING true   // set true to send randomized data to the network
#define DATA_SEND true      // set true to send sensor data to the network
// #define SENSORS_ON true     // set true to read data from sensors

//Wi-Fi credentials
const int wifi_arr_len = 1;
char *ssid_arr[wifi_arr_len] = {"mirasbahay"};
char *pwd_arr[wifi_arr_len] = {"carlopiadredcels"};

//Node credentials
const int nodes = 3;
unsigned int node_ids[nodes] = {0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC};
char* node_tokens[nodes] = {"wGkmunxRiUWWfaLkLu8q", "u24bOqqfCGKZ4IMc0M6j", "XWJo5u7tAyvPGnduuqOa"};

//Timekeeping
unsigned long lastCycle = 0; // describes the time from which the LAST DATA CYCLE started, not the actual last data packet sent

//Screen
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

//Sensors
DHT dht(DHT_PIN, DHT_TYPE);
MQ7 mq7(MQ7_PIN, MQ7_VCC);

//LoRaSENSE
class LoRaSENSE LoRaSENSE(node_ids, node_tokens, nodes, NODE_ID, ssid_arr, pwd_arr, wifi_arr_len, WIFI_TIMEOUT);

void afterInit() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  String idStr = String(LoRaSENSE.getId(), HEX);
  idStr.toUpperCase();
  String displayStr = "Node " + idStr;
  display.print(displayStr);

  display.setCursor(0,10);
  display.print("Initialization OK!");
  display.display();

  Serial.println("Initialization OK!");

  // This can be removed; only here to be able to display after init message
  delay(1000);
}

void onConnecting() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Connecting...");
  display.display();
}

void onConnect() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  String idStr = String(LoRaSENSE.getId(), HEX);
  idStr.toUpperCase();
  String displayStr = idStr + " (" + String(LoRaSENSE.getHopCount()) + ")";
  display.print(displayStr);
  display.setCursor(0,10);
  if (LoRaSENSE.getHopCount() == 0) {
    display.print("Connected as ROOT");
  } else {
    String parentId = String(LoRaSENSE.getParentId(), HEX);
    parentId.toUpperCase();
    String msg = "Connected to " + parentId;
    display.print(msg);
  }
  display.setCursor(0,20);
  String connectTime = "(" + String(LoRaSENSE.getConnectTime()) + " ms)";
  display.print(connectTime);
  display.display();
}

int convertDataToByteArray(byte* &byte_arr, Data* data_arr, int data_len, int data_size) {
  byte_arr = new byte[data_size * data_len];
  int j = 0;
  for (int i = 0; i < data_len; ++i) {
    Data data = data_arr[i];
    for (; j < (data_size * (i+1)); ++j) {
      byte_arr[j] = data.data_b[j % data_size];
    }
  }
  return j;
}

void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);

  //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  //initialize sensors
  dht.begin();
  Serial.println("Sensors initialized");

  LoRaSENSE.setAfterInit(&afterInit);
  LoRaSENSE.setOnConnecting(&onConnecting);
  LoRaSENSE.setOnConnect(&onConnect);
  LoRaSENSE.setup();
}

void loop() {

  if (millis() - lastCycle >= CYCLE_TIME) {

    lastCycle = millis(); // lastCycle must ALWAYS be reset every START of the cycle

    #ifdef DATA_TESTING
      if (DATA_TESTING && LoRaSENSE.isConnected()) {
        Data pm2_5 = {4.5};
        Data pm10 = {10.0};
        Data co = {13.22};
        Data temp = {36.2};
        Data humid = {100.1};
        Data data_arr[] = {pm2_5, pm10, co, temp, humid};
        byte* data;
        int data_len = convertDataToByteArray(data, data_arr, 5, sizeof(float));
        Packet* dataPkt = new Packet(DATA_TYP, LoRaSENSE.getId(), LoRaSENSE.getParentId(), LoRaSENSE.getId(), data, data_len);
        Serial.printf("Adding test data packet %i to queue...\n", dataPkt->getPacketId());
        LoRaSENSE.pushPacketToQueue(dataPkt);
      }
    #endif

    #ifdef SENSORS_ON
      // Collect data from sensors
      if (SENSORS_ON) {
        Serial.println("");

        float c = mq7.getPPM();
        float h = dht.readHumidity();       
        float t = dht.readTemperature();    // celsius

        if (isnan(h) || isnan(t)) {
            Serial.println("Failed to read from DHT sensor!");
            h = 0;
            t = 0;
            // return;
        }

        if (isnan(c)) {
            Serial.println("Failed to read from MQ7 sensor!");
            c = 0;
        }

        // Compute heat index in Celsius (isFahreheit = false)
        float hic = dht.computeHeatIndex(t, h, false);

        Serial.printf("CO: %f\n", c);
        Serial.printf("Humidity: %f\n", h);
        Serial.printf("Temp: %f\n", t);
        Serial.printf("Heat Index: %f\n", hic);
        Serial.printf("Next reading in %u ms\n\n", CYCLE_TIME);

        #ifdef DATA_SEND
          // Send data
          if (DATA_SEND && LoRaSENSE.isConnected()) {
            Data pm2_5 = {0.0};
            Data pm10 = {0.0};
            Data co = {c};
            Data temp = {t};
            Data humid = {h};
            Data data_arr[] = {pm2_5, pm10, co, temp, humid};
            byte* data;
            int data_len = convertDataToByteArray(data, data_arr, 5, sizeof(float));
            Packet* dataPkt = new Packet(DATA_TYP, LoRaSENSE.getId(), LoRaSENSE.getParentId(), LoRaSENSE.getId(), data, data_len);
            Serial.printf("Adding data packet %i to queue...\n", dataPkt->getPacketId());
            LoRaSENSE.pushPacketToQueue(dataPkt);
          }
        #endif
      }
    #endif

  }

  LoRaSENSE.loop();

}