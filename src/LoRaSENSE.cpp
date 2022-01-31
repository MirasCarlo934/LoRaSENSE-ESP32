#include "LoRaSENSE.h"

//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 433E6

LoRaSENSE::LoRaSENSE(char** ssid_arr, char** pwd_arr, int wifi_arr_len, long timeout) {
    this->ssid_arr = ssid_arr;
    this->pwd_arr = pwd_arr;
    this->wifi_arr_len = wifi_arr_len;
    this->timeout = timeout;
}

LoRaSENSE::~LoRaSENSE() {

}

void LoRaSENSE::setup() {
    //SPI LoRa pins
    SPI.begin(SCK, MISO, MOSI, SS);
    //setup LoRa transceiver module
    LoRa.setPins(SS, RST, DIO0);
    
    if (!LoRa.begin(BAND)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }
    Serial.println("LoRa initialized");

    //Setup Wi-Fi
    WiFi.mode(WIFI_STA);
    Serial.println("WiFi initialized");
}

void LoRaSENSE::loop() {
    if (!connected) {
        connectToNetwork();
    }
}

void LoRaSENSE::connectToNetwork() {
    for (int i = 0; i < wifi_arr_len; ++i) {
        Serial.printf("Connecting to Wi-Fi router \"%s\"", ssid_arr[i]);
        WiFi.begin(ssid_arr[i], pwd_arr[i]);
        for (int time = 0; WiFi.status() != WL_CONNECTED && time < timeout; time += 500) {
            delay(500);
            Serial.print(".");
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("connected");
            break;
        } else {
            Serial.println("unable to connect");
        }
    }
    if (WiFi.status() != WL_CONNECTED) {
        //Connect to LoRa mesh
    } else {
        //Node is root
        hopCount = 0;
        connected = true;
        funcOnConnect();
    }
}

void LoRaSENSE::setOnConnect(void funcOnConnect()) {
    this->funcOnConnect = std::bind(funcOnConnect);
}

int LoRaSENSE::getHopCount() {
    return hopCount;
}

bool LoRaSENSE::isConnected() {
    return connected;
}