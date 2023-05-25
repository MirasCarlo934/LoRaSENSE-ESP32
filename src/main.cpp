/*
  NOTES
  1. Set NODE_ID and NODE_ACCESS_TOKEN
  2. Set MIN_HOP
  3. Set DATA_TESTING to true if testing with randomized sensor values
*/

// Constants
#define NODE_ID 0xAAAAAAAA
// #define NODE_ID 0xBBBBBBBB
// #define NODE_ID 0xCCCCCCCC
// #define NODE_ID 0xDDDDDDDD
// #define NODE_ID 0xEEEEEEEE
#define MOBILE_NODE false
#define MIN_HOP 1
#define MAX_HOP INT32_MAX
#define WIFI_ONLY false

// #define DATA_TESTING true        // set true to send randomized data to the network
#define DATA_SEND true              // set true to send sensor data to the network
#define SENSORS_ON true             // set true to read data from sensors
#define DHT22_ON true
#define MQ7_ON false
#define PMS7003_ON true

#define STARTUP_DELAY_MAX 5000     // max of 5s delay upon startup
// #define CYCLE_TIME 10000                    // 10s, for testing only
#define CYCLE_TIME 60000                    // DEFAULT: 60s
// #define CYCLE_TIME 150000                   // 150s, in accordance with the 60s-90s cycle time of MQ-7
#define WIFI_TIMEOUT 10000                   // 10s
#define RREQ_TIMEOUT 3000                   // 3s
#define RREP_DELAY_MAX 1000                 // max of 1s delay when sending RREP packets
#define DACK_TIMEOUT 5000                   // 5s
#define RREQ_LIMIT 10                        // Amount of RREQ packets to send before attempting to connect to Wi-Fi again
// #define NETWORK_RECORD_TIME 600000       // 10m
// #define NETWORK_RECORD_TIME CYCLE_TIME*5   // for debugging only
#define NETWORK_RECORD_TIME 360000          // DEFAULT: 6m, 10 NETRs per hour

#include <Arduino.h>
#include "LoRaSENSE.h"
#include "SoftwareSerial.h"

//Sensor libraries
#ifdef DHT22_ON
  #if DHT22_ON == true
    #include "DHT.h"
  #endif
#endif
#ifdef MQ7_ON
  #if MQ7_ON == true
    #include "MQ7.h"
  #endif
#endif
#ifdef PMS7003_ON
  #if PMS7003_ON == true
    #include "PMS.h"
  #endif
#endif

//Mobile node libraries
#ifdef MOBILE_NODE
  #if MOBILE_NODE == true
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
#define PMS7003_RX 19
#define PMS7003_TX 15
#define PMS7003_BAUD 9600

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
// char *ssid_arr[wifi_arr_len] = {"trash"};
// char *pwd_arr[wifi_arr_len] = {"piamiras27laurens"};
// char *ssid_arr[wifi_arr_len] = {"Testiphone"};
// char *pwd_arr[wifi_arr_len] = {"notiphone"};

//Node credentials
const int nodes = 5;
unsigned int node_ids[nodes] = {0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE};
char* node_tokens[nodes] = {"wGkmunxRiUWWfaLkLu8q", "u24bOqqfCGKZ4IMc0M6j", "XWJo5u7tAyvPGnduuqOa", "KESffXVZoedYJPEdQvTa", "yt5a5JwN9YT5XDblw7Fj"};
char* node_rsta_tokens[nodes] = {"egG6V9ycretmen06GVLZ", "e8hFBf7Oo0BcpznuTeRR", "dJxapl6dSAgBLEsoT5qh", "Jg5L5EYkDvsKzGu7oblO", "fqws1MXC314MeU6wzxdI"};
char* node_netr_tokens[nodes] = {"jve08ZmGmbxPHMvNzDLI", "efG7s1jHM76dyKbE1Run", "64yvwslOeOEKrjbZy1wB", "D7IrQSuII5d3HN5vP0HB", "OVDjzUZI1BbfHLkB5zMq"};
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

//Location and Sensor data
long cycles = 0;     // sensor cycles made
double last_lat = 0; // last recorded latitude
double last_lng = 0; // last recorded longitude
float last_co = 0;
float last_temp = 0;
float last_humid = 0;
float last_pm1 = 0;
float last_pm2_5 = 0;
float last_pm10 = 0;
float total_pm1 = 0;
float total_pm2_5 = 0;
float total_pm10 = 0;
int pm_measurements = 0;
uint32_t orig_free_heap = 0;

//Timekeeping
unsigned long lastCycle = 0; // describes the time from which the LAST DATA CYCLE started, not the actual last data packet sent
unsigned long lastNetworkRecord = 0;  // describes the time from which the LAST NETWORK RECORD has been sent

//Screen
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

//Sensors
#ifdef DHT22_ON
  #if DHT22_ON == true
    DHT dht(DHT_PIN, DHT_TYPE);
  #endif
#endif
#ifdef MQ7_ON
  #if MQ7_ON == true
    MQ7 mq7(MQ7_PIN, MQ7_VCC);
  #endif
#endif
#ifdef PMS7003_ON
  #if PMS7003_ON == true
    SoftwareSerial pms_ss(PMS7003_TX, PMS7003_RX);
    PMS pms(pms_ss);
    PMS::DATA pms_data;
    uint32_t lastPmsRead = 0;                       // last time PMS7003 sensor was read in millis
  #endif
#endif

//LoRaSENSE
class LoRaSENSE LoRaSENSE(node_ids, node_tokens, node_rsta_tokens, node_netr_tokens, nodes, NODE_ID, ssid_arr, pwd_arr, wifi_arr_len, WIFI_ONLY, MIN_HOP, MAX_HOP, WIFI_TIMEOUT, RREQ_TIMEOUT, RREP_DELAY_MAX, DACK_TIMEOUT, RREQ_LIMIT, CYCLE_TIME);

class NetworkRecordNode {
    public:
        int packetId = -1;
        long packetRtt = -1;
        NetworkRecordNode* next;
        NetworkRecordNode(int packetId, long packetRtt) {
            this->packetId = packetId;
            this->packetRtt = packetRtt;
            next = nullptr;
        }
};

//Network record data
NetworkRecordNode* networkRecord = nullptr;
long packetSends = 0;         // packet sends from ALL nodes
long successfulPacketSends = 0;
long origPacketSends = 0;     // original packet sends from this node
long successfulOrigPacketSends = 0;
long allRtt = 0;
long origRtt = 0;
long successfulBytesSent = 0;
int currentPacketId = 0;          
long currentPacketRtt = 0;   // time in millis when the current packet was sent

int countNetworkRecord() {
    NetworkRecordNode* head = networkRecord;
    int count = 0;
    while (head != nullptr) {
        ++count;
        head = head->next;
    }
    return count;
}

void addNetworkRecord(int packetId, long packetRtt) {
    if (networkRecord == nullptr) {
        networkRecord = new NetworkRecordNode(packetId, packetRtt);
        return;
    }
    NetworkRecordNode* head = networkRecord;
    while (head->next != nullptr) {
        head = head->next;
    }
    head->next = new NetworkRecordNode(packetId, packetRtt);
    // DEBUG
        Serial.printf("New network record added (Packet ID: %i, RTT: %li) [count: %i]\n", packetId, packetRtt, countNetworkRecord());
    //
}

void displayInfo() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  String idStr = String(LoRaSENSE.getId(), HEX);
  idStr.toUpperCase();
  String line1 = idStr + " (" + LoRaSENSE.getHopCount() + ")(" + cycles + ")";
  display.print(line1);
  display.setCursor(0,10);
  display.printf("T:%.2f | H:%.2f", last_temp, last_humid);
  display.setCursor(0,20);
  display.printf("1:%.2f | 2.5:%.2f", last_pm1, last_pm2_5);
  display.setCursor(0,30);
  display.printf("10:%.2f", last_pm10);
  display.setCursor(0,40);
  display.printf("CO:%.2f", last_co);
  display.setCursor(0,50);
  uint32_t free_heap = ESP.getFreeHeap();
  display.printf("HEAP: %u", free_heap);
  display.display();
}

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

void onSend() {
  if (!LoRaSENSE.packetQueueIsEmpty() && currentPacketId != LoRaSENSE.peekPacketQueue()->getPacketId()) {
    Packet* packet = LoRaSENSE.peekPacketQueue();
    if (currentPacketId != packet->getPacketId()) { // new packet sent; not a resend
      currentPacketRtt = millis();
    }
    currentPacketId = packet->getPacketId();
    if (packet->getType() == DATA_TYP || packet->getType() == NETR_TYP || packet->getType() == RSTA_TYP) {
      if (packet->getSourceId() == LoRaSENSE.getId()) {
        ++origPacketSends;
      }
      ++packetSends;
    }
  }
}

void onSendSuccess() {
  // add network record
  if (!LoRaSENSE.packetQueueIsEmpty()) {
    Packet* packet = LoRaSENSE.peekPacketQueue();
    byte packetType = LoRaSENSE.peekPacketQueue()->getType();
    if (packet->getType() == DATA_TYP || packet->getType() == NETR_TYP || packet->getType() == RSTA_TYP) {
      currentPacketRtt = millis() - currentPacketRtt;
      if (packet->getSourceId() == LoRaSENSE.getId()) {
        ++successfulOrigPacketSends;
        origRtt += currentPacketRtt;
      }
      ++successfulPacketSends;
      allRtt += currentPacketRtt;
      successfulBytesSent += packet->getLength() * sizeof(byte);
      addNetworkRecord(currentPacketId, currentPacketRtt);
      Serial.printf("[NETR] New network record with sends: %li, succSends: %li, origSends: %li, succOrigSends: %li, rtt: %li, origRtt: %li, bytes: %li\n", 
        packetSends, successfulPacketSends, origPacketSends, successfulOrigPacketSends, allRtt, origRtt, successfulBytesSent);
    }
  }

  // display on screen
  displayInfo();
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

void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);
  
  #ifdef MOBILE_NODE
    #if MOBILE_NODE == true
      ss.begin(GPS_BAUD);
    #endif
  #endif
  #ifdef PMS7003_ON
    #if PMS7003_ON == true
      pms_ss.begin(PMS7003_BAUD);
      pms.passiveMode();
      pms.wakeUp();
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
  #ifdef DHT22_ON
    #if DHT22_ON == true
      dht.begin();
    #endif
  #endif
  Serial.println("Sensors initialized");

  //initialize variables
  orig_free_heap = ESP.getFreeHeap();

  LoRaSENSE.setAfterInit(&afterInit);
  LoRaSENSE.setOnConnecting(&onConnecting);
  LoRaSENSE.setOnConnect(&onConnect);
  LoRaSENSE.setOnSendSuccess(&onSendSuccess);
  LoRaSENSE.setOnSend(&onSend);

  // delay to avoid packet collision when 2 or more nodes are turned on simultaneously
  long rand_delay = esp_random() % STARTUP_DELAY_MAX;
  Serial.printf("Initialization DONE. Wait for %ims\n", rand_delay);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Initialization DONE");
  display.setCursor(0,10);
  display.print("Wait for ");
  display.print(rand_delay);
  display.print("ms");
  display.display();
  lastCycle = rand_delay; // delay the first time the node will send data
  delay(rand_delay);

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

  // Read from PMS7003 every second
  #if PMS7003_ON == true
    if (millis() % 1000 == 0) {
      while (pms_ss.available()) { 
        pms_ss.read(); 
      }
      pms.requestRead();
      if (pms.readUntil(pms_data)) {
        total_pm1 += pms_data.PM_AE_UG_1_0;
        total_pm2_5 += pms_data.PM_AE_UG_2_5;
        total_pm10 += pms_data.PM_AE_UG_10_0;
        ++pm_measurements;
      }
    }
  #endif

  // Send sensor data
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
      #if SENSORS_ON == true
        Serial.println("");
        Serial.printf("CYCLE %li\n", cycles);
        ++cycles;

        // Read from MQ-7 and DHT22
        float c = 0, h = 0, t = 0;

        #ifdef MQ7_ON
          #if MQ7_ON == true
            c = mq7.getPPM();
            if (isnan(c)) {
                Serial.println("Failed to read from MQ7 sensor!");
                c = 0;
            }
          #endif
        #endif
        #ifdef DHT22_ON
          #if DHT22_ON == true
            h = dht.readHumidity();       
            t = dht.readTemperature();    // celsius
            if (isnan(h) || isnan(t)) {
                Serial.println("Failed to read from DHT sensor!");
                h = 0;
                t = 0;
            }
            // Compute heat index in Celsius (isFahreheit = false)
            float hic = dht.computeHeatIndex(t, h, false);
          #endif
        #endif
      
        // Get average from PMS7003 readings
        #ifdef PMS7003_ON == true
          last_pm1 = total_pm1 / pm_measurements;
          last_pm2_5 = total_pm2_5 / pm_measurements;
          last_pm10 = total_pm10 / pm_measurements;
          total_pm1 = 0;
          total_pm2_5 = 0;
          total_pm10 = 0;
          pm_measurements = 0;
        #endif

        Serial.printf("Lat: %f\n", last_lat);
        Serial.printf("Lng: %f\n", last_lng);
        Serial.printf("CO: %f\n", c);
        Serial.printf("Humidity: %f\n", h);
        Serial.printf("Temp: %f\n", t);
        Serial.printf("Heat Index: %f\n", hic);
        Serial.printf("PM 1.0: %f\n", last_pm1);
        Serial.printf("PM 2.5: %f\n", last_pm2_5);
        Serial.printf("PM 10.0: %f\n", last_pm10);
        Serial.printf("Free Heap: %u\n", ESP.getFreeHeap());
        Serial.printf("Next reading in %u ms\n\n", CYCLE_TIME);

        last_humid = h;
        last_temp = t;
        last_co = c;
        displayInfo();

        #ifdef DATA_SEND
          // Send data
          if (DATA_SEND && LoRaSENSE.isConnected()) {
            Data pm1 = {last_pm1};
            Data pm2_5 = {last_pm2_5};
            Data pm10 = {last_pm10};
            Data co = {c};
            Data temp = {t};
            Data humid = {h};
            Data_d lat = {last_lat};
            Data_d lng = {last_lng};
            Data_l sends = {packetSends};
            Data_l bytes = {successfulBytesSent};

            int data_arr_len = 6;
            Data data_arr[data_arr_len] = {pm1, pm2_5, pm10, co, temp, humid};
            Data_d gps_data_arr[] = {lat, lng};
            Data_l network_data_arr[] = {sends, bytes};
            byte data[data_arr_len*sizeof(Data) + 2*sizeof(Data_d)/* + 2*sizeof(Data_l)*/];
            int data_len = appendDataToByteArray(data, 0, data_arr, data_arr_len, sizeof(Data));
            data_len = appendDataToByteArray(data, data_len, gps_data_arr, 2, sizeof(Data_d));
            data_len = appendDataToByteArray(data, data_len, network_data_arr, 2, sizeof(Data_l));
            Packet* dataPkt = new Packet(DATA_TYP, LoRaSENSE.getId(), LoRaSENSE.getParentId(), LoRaSENSE.getId(), data, data_len);
            Serial.printf("Adding data packet %i to queue...\n", dataPkt->getPacketId());
            LoRaSENSE.pushPacketToQueue(dataPkt);
          }
        #endif
      #endif
    #endif

  }

  // Send network data
  if (millis() - lastNetworkRecord >= NETWORK_RECORD_TIME) {
    
    lastNetworkRecord = millis();

    NetworkRecordNode* head = networkRecord;
    int record_len = countNetworkRecord();
    int data_arr_len = 2*record_len + 7; // additional 7 for sends, succSends, origSends, succOrigSends, rtt, origRtt, and bytes
    Data_l data_arr[data_arr_len];
    byte data[data_arr_len*sizeof(Data_l)];

    // IN THIS ORDER ONLY!
    allRtt = allRtt / successfulPacketSends;
    origRtt = origRtt / successfulOrigPacketSends;

    Serial.printf("[NETR] Sending NETR with sends: %li, succSends: %li, origSends: %li, succOrigSends: %li, rtt: %li, origRtt: %li, bytes: %li\n", 
        packetSends, successfulPacketSends, origPacketSends, successfulOrigPacketSends, allRtt, origRtt, successfulBytesSent);

    Data_l sends = {packetSends};
    Data_l succSends = {successfulPacketSends};
    Data_l origSends = {origPacketSends};
    Data_l succOrigSends = {successfulOrigPacketSends};
    Data_l succBytes = {successfulBytesSent};
    Data_l rtt = {allRtt};
    Data_l d_origRtt = {origRtt};
    data_arr[0] = sends;
    data_arr[1] = succSends;
    data_arr[2] = origSends;
    data_arr[3] = succOrigSends;
    data_arr[4] = rtt;
    data_arr[5] = d_origRtt;
    data_arr[6] = succBytes;

    for (int i = 7; head != nullptr; i += 2) {
        data_arr[i].data_l = head->packetId;
        data_arr[i+1].data_l = head->packetRtt;
        NetworkRecordNode* prev = head;
        head = head->next;
        delete prev;  // TODO: is this a good idea? (deleting network records before ensuring that they were sent)
    }
    networkRecord = nullptr;  // at this point, all network record nodes should have already been deleted
    packetSends = 0;
    successfulPacketSends = 0;
    origPacketSends = 0;
    successfulOrigPacketSends = 0;
    successfulBytesSent = 0;
    allRtt = 0;
    origRtt = 0;

    int data_len = appendDataToByteArray(data, 0, data_arr, data_arr_len, sizeof(Data_l));
    Packet* netrPkt = new Packet(NETR_TYP, LoRaSENSE.getId(), LoRaSENSE.getParentId(), LoRaSENSE.getId(), data, data_len);
    LoRaSENSE.pushPacketToQueue(netrPkt);
  }

  LoRaSENSE.loop();
}