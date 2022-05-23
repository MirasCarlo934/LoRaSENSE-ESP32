#ifndef LORASENSE_H
#define LORASENSE_H

#include <Arduino.h>
#include <ArduinoJson.h>

//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

//Libraries for Wi-Fi
#include <WiFi.h>

//Libraries for HTTP/S
#include <HTTPClient.h>

//Libraries for CRC
// #include "CRC.h"

//Constants
#define RREQ_TYP 0b000
#define RREP_TYP 0b001
#define RERR_TYP 0b010
#define DATA_TYP 0b011
#define DACK_TYP 0b100
#define NETR_TYP 0b110
#define RSTA_TYP 0b111

// #define WIFI_TIMEOUT 30000  // 30s
// #define RREQ_TIMEOUT 5000   // 5s
// #define WIFI_RECONN 60000   // 60s
// #define ACK_TIMEOUT 1000    // 1s
// #define ACK_TIMEOUT 5000    // 5s
// #define RESEND_MAX_TIME 369

#define RSSI_THRESH -90

//Server info
// #define SERVER_ENDPOINT "https://psgj46mwwb.execute-api.us-east-1.amazonaws.com/production/packet"
#define SERVER_ENDPOINT "https://thingsboard.cloud/api/v1/$ACCESS_TOKEN/telemetry"
#define SERVER_CA ""

union Data {
    float data_f;
    byte data_b[sizeof(float)];
};

union Data_l {
    long data_l;
    byte data_b[sizeof(long)];
};

union Data_d {
    double data_d;
    byte data_b[sizeof(double)];
};

int appendDataToByteArray(byte* byte_arr, int last_byte_arr_i, void* data_arr, int data_len, int data_size);

class Packet {

    private:
        byte* payload;
        long len;
        int data_len;
        int rssi = 0;   // holds the RSSI of the packet when received
        long send_time = 0; // absolute time in millis when the packet will be sent, 0 to send immediately
        void defaultInit(byte type, int packet_id, int sender_id, int receiver_id, int source_id, byte* data, int data_len);
   
    public:
        Packet();
        Packet(byte* payload, int len);
        Packet(byte type, int sender_id, int receiver_id, int source_id, byte* data, int data_len);
        Packet(Packet* packet, int sender_id, int receiver_id);  // for DATA packet forwarding
        Packet(Packet* packet, int sender_id, int receiver_id, byte* data, int data_len); // for RSTA packet forwarding
        ~Packet();

        // Class methods
        bool checkCRC();
        bool send();
        void printToSerial();

        // Encapsulation
        byte getType();
        char* getTypeInString();
        int getPacketId();
        int getSenderId();
        int getSourceId();
        int getReceiverId();
        int getPayload(byte* &payload);
        void getData(byte* data);
        int getLength();
        int getDataLength();
        void setRssi(int rssi);
        int getRssi();
        void setSendTime(long send_time);
        long getSendTime();

};

class PacketQueueNode {

    public:
        PacketQueueNode* next;
        Packet* packet;
        
        PacketQueueNode(Packet* packet);

};

class PacketQueue {

    private:
        PacketQueueNode* head;

    public:
        PacketQueue();
        int getSize();
        bool isEmpty();
        void push(Packet* packet);
        void pushFront(Packet* packet);
        Packet* peekFront();
        Packet* popFront();
        Packet* peek(int position);
        Packet* pop(int position);

};

class LoRaSENSE {

    private:
        unsigned int* node_ids;
        char** node_tokens;
        char** node_rsta_tokens;
        char** node_netr_tokens;
        int nodes;

        // User-defined variables
        unsigned int id = 0;
        char** ssid_arr = {};                       // Wi-Fi SSIDs to connect to
        char** pwd_arr = {};                        // Wi-Fi passwords to Wi-Fi SSIDs
        int wifi_arr_len = 0;
        int min_hop = 0;
        int max_hop = INT32_MAX;
        bool wifi_only = false;                     // true if node can only connect to wifi
        unsigned long wifiTimeout = 30000;          // time to wait for wifi connection; 30s by default
        unsigned long rreqTimeout = 5000;           // time to wait for RREQ packets; 5s by default
        unsigned long rrepDelayMax = 1000;
        unsigned long dackTimeout = 5000;           // time to wait for DACK packet confirmation; 5s by default
        unsigned long rreqLimit = 5;                // RREQ packets to send before connecting to wifi again; 5 by default
        unsigned long cycleTime = 60000;            // 1m by default

        // Network performance variables
        // long totalSends = 0;
        // long totalBytesSentSuccessfully = 0;

        // Network connection variables
        int parent_id = 0;
        int wifi_i = 0;                             // iterator for ssid_arr and pwd_arr
        int hopCount = INT_MAX;
        bool connectingToWifi = false;
        bool connectingToLora = false;
        bool connected = false;
        bool waitingForAck = false;
        bool resent = false;
        unsigned long startConnectTime = 0;
        unsigned long connectTime = 0;
        unsigned long lastRreqSent = 0;
        unsigned long lastWifiAttempt = 0;
        unsigned long nextSendAttempt = 0;          // time in millis where next send attempt can be made
        unsigned long lastSendAttempt = 0;          // time to wait for a DACK/NACK
        unsigned long beginSendToServer = 0;        // time when device began sending to server (for logging purposes)
        PacketQueue packetQueue;
        PacketQueue receivedPackets;
        HTTPClient* httpClient;                     // http client for root nodes

        // Callback variables
        std::function<void()> funcAfterInit;        // callback after LoRaSENSE successfully initializes
        std::function<void()> funcOnConnecting;     // callback when node is currently connecting to network
        std::function<void()> funcOnConnect;        // callback when node successfully connects to network
        std::function<void()> funcOnSendSuccess;    // callback when a packet has been successfully sent
        std::function<void()> funcOnSend;           // callback when a packet has been sent but NOT yet ack'ed

        // Packet processing functions
        void processRreq(Packet* packet);
        void processRrep(Packet* packet);
        void processRerr(Packet* packet);
        void processRsta(Packet* packet);
        void processData(Packet* packet);
        void processDack(Packet* packet);
        void processNetr(Packet* packet);
        void sendPacketViaLora(Packet* packet, bool waitForAck);
        void sendPacketToServer(Packet* packet);

    public:
        LoRaSENSE(unsigned int* node_ids, char** node_tokens, char** node_rsta_tokens, char** node_netr_tokens, int nodes, unsigned int id, char** ssid_arr, char** pwd_arr, int wifi_arr_len, bool wifi_only, int min_hop, int max_hop, unsigned long wifi_timeout, unsigned long rreq_timeout, unsigned long rrep_delay_max, unsigned long dack_timeout, unsigned long rreq_limt, unsigned long cycle_time);
        ~LoRaSENSE();

        void setup();
        void loop();

        // Network connection methods
        void connectToWifi(char* ssid, char* pwd);
        void connectToLora();
        void reconnect();        

        // LoRa radio callback methods
        void onLoraReceive(int packetSize);

        // Packet queue methods
        bool sendPacketInQueue();   // true if packet was sent, false if not
        void pushPacketToQueue(Packet* packet);
        void pushPacketToQueueFront(Packet* packet);
        Packet* peekPacketQueue();
        bool packetQueueIsEmpty();

        // Encapsulation methods
        void setAfterInit(void funcAfterInit());
        void setOnConnecting(void funcOnConnecting());
        void setOnConnect(void funcOnConnect());
        void setOnSendSuccess(void funcOnSendSuccess());
        void setOnSend(void funcOnSend());
        unsigned int getId();
        int getParentId();
        int getHopCount();
        bool isConnected();
        unsigned long getConnectTime();

};

#endif