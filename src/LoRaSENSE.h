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
#define NACK_TYP 0b101
#define RSTA_TYP 0b111

#define WIFI_TIMEOUT 5000   // 5s
#define RREQ_TIMEOUT 5000   // 5s
#define WIFI_RECONN 60000   // 60s
// #define DATA_SEND 300000    // 300s/5mins
#define DATA_SEND 10000     // 10s, for testing only!!

//Server info
// #define SERVER_ENDPOINT "https://psgj46mwwb.execute-api.us-east-1.amazonaws.com/production/packet"
#define SERVER_ENDPOINT "https://thingsboard.cloud/api/v1/$ACCESS_TOKEN/telemetry"
#define SERVER_CA ""

//Debugging (can be overridden)
// #define ROOTABLE true
#define ROOTABLE false

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
        Packet(Packet packet, int sender_id, int receiver_id);  // for DATA packet forwarding
        ~Packet();
        void send();
        void printToSerial();

        byte getType();
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
        Packet* popFront();
};

class LoRaSENSE {
    private:
        String thingsboard_access_token;
        int id;
        int parent_id;
        char** ssid_arr;
        char** pwd_arr;
        int wifi_arr_len;
        PacketQueue packetQueue;
        std::function<void()> funcOnConnect;

        int hopCount = 99999999;
        bool connected;
        bool rreqSent = false;
        unsigned long wifiTimeout;
        unsigned long lastRreqSent;
        unsigned long lastWifiAttempt;
        unsigned long lastDataSent = 0; // describes the time from which the LAST DATA CYCLE started, not the actual last data packet sent

        // Packet constructRreqPacket();
    public:
        LoRaSENSE(String thingsboard_access_token, int id, char** ssid_arr, char** pwd_arr, int wifi_arr_len, long timeout);
        ~LoRaSENSE();
        void setup();
        void loop();
        void connectToNetwork();
        void connectToLoRa();

        void setOnConnect(void funcOnConnect());
        int getId();
        int getParentId();
        int getHopCount();
        bool isConnected();
};

#endif