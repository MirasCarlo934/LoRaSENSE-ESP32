#include <Arduino.h>

//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

//Libraries for Wi-Fi
#include <WiFi.h>

#ifndef LORASENSE_H
#define LORASENSE_H

class LoRaSENSE {
    private:
        char** ssid_arr;
        char** pwd_arr;
        int wifi_arr_len;
        long timeout;
        std::function<void()> funcOnConnect;

        int hopCount;
        bool connected;
    public:
        LoRaSENSE(char** ssid_arr, char** pwd_arr, int wifi_arr_len, long timeout);
        ~LoRaSENSE();
        void setup();
        void loop();
        void connectToNetwork();

        void setOnConnect(void funcOnConnect());
        int getHopCount();
        bool isConnected();
};

#endif