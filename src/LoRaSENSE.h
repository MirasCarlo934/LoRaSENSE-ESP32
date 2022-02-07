#ifndef LORASENSE_H
#define LORASENSE_H

#include <Arduino.h>

//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

//Libraries for Wi-Fi
#include <WiFi.h>

//Libraries for CRC
// #include "CRC.h"

//Utils
// #include<Vector.h>

//Constants
#define RREQ_TYP 0b000
#define RREP_TYP 0b001
#define RERR_TYP 0b010
#define DATA_TYP 0b011
#define DACK_TYP 0b100
#define NACK_TYP 0b101
#define RSTA_TYP 0b111

//Debugging (can be overridden)
#define ROOTABLE false

typedef struct Packet {
    byte* payload;
    int len;
} Packet;

class LoRaSENSE {
    private:
        int id;
        int parent_id;
        char** ssid_arr;
        char** pwd_arr;
        int wifi_arr_len;
        long timeout;
        std::function<void()> funcOnConnect;

        int hopCount;
        bool connected;

        Packet constructRreqPacket();
    public:
        LoRaSENSE(int id, char** ssid_arr, char** pwd_arr, int wifi_arr_len, long timeout);
        ~LoRaSENSE();
        void setup();
        void loop();
        void connectToNetwork();

        void setOnConnect(void funcOnConnect());
        int getHopCount();
        bool isConnected();
};

#endif