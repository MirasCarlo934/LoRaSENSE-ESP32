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
#define RSTA_TYP 0b111

#define WIFI_TIMEOUT 30000  // 30s
#define RREQ_TIMEOUT 5000   // 5s
#define WIFI_RECONN 60000   // 60s
// #define ACK_TIMEOUT 1000    // 1s
#define ACK_TIMEOUT 5000    // 5s
#define RESEND_MAX_TIME 369

#define RSSI_THRESH -90

//Server info
// #define SERVER_ENDPOINT "https://psgj46mwwb.execute-api.us-east-1.amazonaws.com/production/packet"
#define SERVER_ENDPOINT "https://thingsboard.cloud/api/v1/$ACCESS_TOKEN/telemetry"
#define SERVER_CA ""

//Debugging (can be overridden)
#define MIN_HOP 1

class Packet {

    private:
        byte* payload;
        long len;
        int data_len;
        void defaultInit(byte type, int packet_id, int sender_id, int receiver_id, int source_id, byte* data, int data_len);
   
    public:
        Packet();
        Packet(byte* payload, int len);
        Packet(byte type, int sender_id, int receiver_id, int source_id, byte* data, int data_len);
        Packet(Packet* packet, int sender_id, int receiver_id);  // for DATA packet forwarding
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
        int getData(byte* &data);
        int getLength();
        int getDataLength();

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

};

class LoRaSENSE {

    private:
        unsigned int* node_ids;
        char** node_tokens;
        int nodes;

        // Network connection variables
        unsigned int id = 0;
        int parent_id = 0;
        char** ssid_arr;
        char** pwd_arr;
        int wifi_arr_len = 0;
        int wifi_i = 0;                     // iterator for ssid_arr and pwd_arr
        int hopCount = INT_MAX;
        bool connectingToWifi = false;
        bool connectingToLora = false;
        bool connected = false;
        bool waitingForAck = false;
        bool resent = false;
        unsigned long startConnectTime = 0;
        unsigned long connectTime = 0;
        unsigned long wifiTimeout = 0;
        unsigned long lastRreqSent = 0;
        unsigned long lastWifiAttempt = 0;
        unsigned long nextSendAttempt = 0; // time in millis where next send attempt can be made
        unsigned long lastSendAttempt = 0;      // time to wait for a DACK/NACK
        PacketQueue packetQueue;

        // Callback variables
        std::function<void()> funcAfterInit;
        std::function<void()> funcOnConnecting;
        std::function<void()> funcOnConnect;

        // Packet processing functions
        void processRreq(Packet* packet);
        void processRrep(Packet* packet, int rssi);
        void processData(Packet* packet);
        void processDack(Packet* packet);
        void sendPacketViaLora(Packet* packet, bool waitForAck);
        void sendPacketToServer(Packet* packet);

    public:
        LoRaSENSE(unsigned int* node_ids, char** node_tokens, int nodes, unsigned int id, char** ssid_arr, char** pwd_arr, int wifi_arr_len, long timeout);
        ~LoRaSENSE();

        void setup();
        void loop();
        void connectToWifi(char* ssid, char* pwd);
        void connectToLora();
        void reconnect();        
        bool sendPacketInQueue();   // true if packet was sent, false if not
        void pushPacketToQueue(Packet* packet);
        void pushPacketToQueueFront(Packet* packet);

        void setAfterInit(void funcAfterInit());
        void setOnConnecting(void funcOnConnecting());
        void setOnConnect(void funcOnConnect());
        unsigned int getId();
        int getParentId();
        int getHopCount();
        bool isConnected();
        unsigned long getConnectTime();

};

#endif