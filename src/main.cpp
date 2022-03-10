/*
  NOTES
  1. Set NODE_ID and NODE_ACCESS_TOKEN (main.cpp)
  2. Set MIN_HOP (LoRaSENSE.h)
*/

#include <Arduino.h>
#include "LoRaSENSE.h"

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//OLED pins
#define OLED_SDA 21
#define OLED_SCL 22 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

//Constants
// #define NODE_ID 0xAAAAAAAA
// #define NODE_ACCESS_TOKEN "wGkmunxRiUWWfaLkLu8q"  // Thingsboard access token for node A
#define NODE_ID 0xBBBBBBBB
// #define NODE_ACCESS_TOKEN "u24bOqqfCGKZ4IMc0M6j"  // Thingsboard access token for node B
// #define NODE_ID 0xCCCCCCCC
// #define NODE_ACCESS_TOKEN "XWJo5u7tAyvPGnduuqOa"  // Thingsboard access token for node C
#define CYCLE_TIME 10000     // 10s, for testing only!!

//Debugging
#define DATA_TESTING false

//Screen
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

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

  LoRaSENSE.setAfterInit(&afterInit);
  LoRaSENSE.setOnConnecting(&onConnecting);
  LoRaSENSE.setOnConnect(&onConnect);
  LoRaSENSE.setup();
}

void loop() {
  if (millis() - lastCycle >= CYCLE_TIME) {
    lastCycle = millis(); // lastCycle must ALWAYS be reset every START of the cycle
    #ifdef DATA_TESTING
      if (DATA_TESTING) {
        long long* data = new long long[5];
        data[0] = rand();   // pm2.5
        data[1] = rand();   // pm10
        data[2] = rand();   // co
        data[3] = rand();   // temp
        data[4] = rand();   // humid
        Packet* dataPkt = new Packet(DATA_TYP, LoRaSENSE.getId(), LoRaSENSE.getParentId(), LoRaSENSE.getId(), reinterpret_cast<byte*>(data), sizeof(long long)*5);
        Serial.printf("Adding test data packet %i to queue...\n", dataPkt->getPacketId());
        LoRaSENSE.addPacketToQueue(dataPkt);
      }
    #endif
  }
  LoRaSENSE.loop();
}