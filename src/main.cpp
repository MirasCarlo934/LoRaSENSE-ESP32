/*
  NOTES
  1. Set NODE_ID and NODE_ACCESS_TOKEN (main.cpp)
  2. Set MIN_HOP (LoRaSENSE.h)
  3. Set DATA_TESTING to true if testing with randomized sensor values
*/

//Constants
// #define NODE_ID 0xAAAAAAAA
// #define NODE_ACCESS_TOKEN "wGkmunxRiUWWfaLkLu8q"  // Thingsboard access token for node A
#define NODE_ID 0xBBBBBBBB
// #define NODE_ACCESS_TOKEN "u24bOqqfCGKZ4IMc0M6j"  // Thingsboard access token for node B
// #define NODE_ID 0xCCCCCCCC
// #define NODE_ACCESS_TOKEN "XWJo5u7tAyvPGnduuqOa"  // Thingsboard access token for node C
// #define NODE_ID 0xDDDDDDDD
// #define NODE_ID 0xEEEEEEEE
#define CYCLE_TIME 10000     // 10s, for testing only!!
#define MOBILE_NODE false

//Debugging
// #define DATA_TESTING true   // set true to send randomized data to the network
#define DATA_SEND true      // set true to send sensor data to the network
#define SENSORS_ON true     // set true to read data from sensors

#include <Arduino.h>
#include "LoRaSENSE.h"

//Sensor libraries
#include "DHT.h"
#include "MQ7.h"

//Mobile node libraries
#ifdef MOBILE_NODE
  #if MOBILE_NODE == true
    #include "SoftwareSerial.h"
    #include "TinyGPS++.h"
    #define GPS_RX 36
    #define GPS_TX 39
    #define GPS_BAUD 9600
  #endif
#endif

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Sensor constants
#define DHT_PIN 14
#define DHT_TYPE 22
#define MQ7_PIN 34
#define MQ7_VCC 5.0

//OLED pins
#define OLED_SDA 21
#define OLED_SCL 22 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

//Wi-Fi credentials
const int wifi_arr_len = 1;
char *ssid_arr[wifi_arr_len] = {"mirasbahay"};
char *pwd_arr[wifi_arr_len] = {"carlopiadredcels"};

//Node credentials
const int nodes = 5;
unsigned int node_ids[nodes] = {0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE};
char* node_tokens[nodes] = {"wGkmunxRiUWWfaLkLu8q", "u24bOqqfCGKZ4IMc0M6j", "XWJo5u7tAyvPGnduuqOa", "KESffXVZoedYJPEdQvTa", "yt5a5JwN9YT5XDblw7Fj"};
double node_coords[][2] = {
  {14.209234046941177, 121.06352544064003},
  {14.207157320684628, 121.06521845071254},
  {14.206824309550607, 121.06818332502569},
  {14.206911045569822, 121.06149026454726},
  {14.205470505014995, 121.06228028118633}
};

//Mobile node
#ifdef MOBILE_NODE
  #if MOBILE_NODE == true
    SoftwareSerial ss(GPS_TX, GPS_RX);
    TinyGPSPlus gps;
  #endif
#endif

//Location
double last_lat = 0; // last recorded latitude
double last_lng = 0; // last recorded longitude

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

// void displayGpsData(double lat, double lng) {
//   display.clearDisplay();
//   display.setTextColor(WHITE);
//   display.setCursor(0,0);
//   display.print(lat);
//   display.setCursor(0,10);
//   display.print(lng);
//   display.display();
// }

/**
 * @brief Appends an array of Data unions to a given byte array.
 * 
 * @param byte_arr the byte array, MUST be instantiated with a capacity that can hold the appended Data bytes
 * @param last_byte_arr_i the byte_arr index of its last element
 * @param data_arr the Data union array to be appended to byte_arr
 * @param data_len the length of data_arr
 * @param data_size the size of each element of new_bytes_arr, MUST be either the size of a float or a double
 * @return int last array index added to byte_arr
 */
int appendDataToByteArray(byte* &byte_arr, int last_byte_arr_i, void* data_arr, int data_len, int data_size) {
  // byte_arr = new byte[data_size * data_len];
  int j = last_byte_arr_i;
  for (int i = 0; i < data_len; ++i) {
    byte* data;
    if (data_size == sizeof(Data)) {
      data = ((Data*)data_arr)[i].data_b;
    } else if (data_size == sizeof(Data_d)) {
      data = ((Data_d*)data_arr)[i].data_b;
    } else {
      throw "Not valid data size!";
    }
    for (; j < (last_byte_arr_i + (data_size * (i+1))); ++j) {
      byte_arr[j] = data[(j - last_byte_arr_i) % data_size];
    }
  }
  return j;
}

void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);
  #ifdef MOBILE_NODE
    #if MOBILE_NODE == true
      ss.begin(GPS_BAUD);
    #endif
  #endif

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

  #ifdef MOBILE_NODE
    #if MOBILE_NODE == true
      while (ss.available() > 0) {
        byte ss_b = ss.read();
        gps.encode(ss_b);
        // Serial.write(ss_b);
        if (gps.location.isUpdated()){
          // Serial.print("\nLatitude= "); 
          // Serial.print(gps.location.lat(), 6);
          // Serial.print(" Longitude= "); 
          // Serial.println(gps.location.lng(), 6);
          last_lat = gps.location.lat();
          last_lng = gps.location.lng();
          // displayGpsData(gps.location.lat(), gps.location.lng());
        }
      }
    #else
      for (int i = 0; i < nodes; ++i) {
        if (LoRaSENSE.getId() == node_ids[i]) {
          last_lat = node_coords[i][0];
          last_lng = node_coords[i][1];
          break;
        }
      }
    #endif
  #endif

  if (millis() - lastCycle >= CYCLE_TIME) {

    lastCycle = millis(); // lastCycle must ALWAYS be reset every START of the cycle

    #ifdef DATA_TESTING
      if (DATA_TESTING && LoRaSENSE.isConnected()) {
        Data pm2_5 = {4.5};
        Data pm10 = {10.0};
        Data co = {13.22};
        Data temp = {36.2};
        Data humid = {100.1};
        Data_d lat = {last_lat};
        Data_d lng = {last_lng};
        Data data_arr[] = {pm2_5, pm10, co, temp, humid};
        Data_d gps_data_arr[] = {lat, lng};
        byte* data = new byte[5*sizeof(Data) + 2*sizeof(Data_d)];
        int data_len = appendDataToByteArray(data, 0, data_arr, 5, sizeof(Data));
        data_len = appendDataToByteArray(data, data_len, gps_data_arr, 2, sizeof(Data_d));
        Packet* dataPkt = new Packet(DATA_TYP, LoRaSENSE.getId(), LoRaSENSE.getParentId(), LoRaSENSE.getId(), data, data_len);
        Serial.printf("\nAdding test data packet %i to queue...\n", dataPkt->getPacketId());
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
            Data_d lat = {last_lat};
            Data_d lng = {last_lng};
            Data data_arr[] = {pm2_5, pm10, co, temp, humid};
            Data_d gps_data_arr[] = {lat, lng};
            byte* data = new byte[5*sizeof(Data) + 2*sizeof(Data_d)];
            int data_len = appendDataToByteArray(data, 0, data_arr, 5, sizeof(Data));
            data_len = appendDataToByteArray(data, data_len, gps_data_arr, 2, sizeof(Data_d));
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