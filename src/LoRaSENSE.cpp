#include "LoRaSENSE.h"

// //Libraries for LoRa
// #include <SPI.h>
// #include <LoRa.h>

// //Libraries for Wi-Fi
// #include <WiFi.h>

//Libraries for CRC
// For some reason, di gumagana yung CRC.h pag sa LoRaSENSE.h naka-include
#include "CRC.h"

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

Packet::Packet(byte type, int sender_id, int receiver_id, int source_id, byte* data, int data_len) {
    int len = 12 + data_len; // minimum header + data
    byte* raw_payload = (byte*) malloc((len-2) * sizeof(byte*)); // excludes checksum
    byte* payload = (byte*) malloc(len * sizeof(byte*));
    int packet_id = rand();

    raw_payload[0] = (type << 5) & 0xFF;
    raw_payload[1] = 0;
    payload[0] = (type << 5) & 0xFF;
    payload[1] = 0;
    raw_payload[2] = (packet_id >> 24) & 0xFF;
    raw_payload[3] = (packet_id >> 16) & 0xFF;
    raw_payload[4] = (packet_id >> 8) & 0xFF;
    raw_payload[5] = packet_id & 0xFF;
    payload[4] = (packet_id >> 24) & 0xFF;
    payload[5] = (packet_id >> 16) & 0xFF;
    payload[6] = (packet_id >> 8) & 0xFF;
    payload[7] = packet_id & 0xFF;
    raw_payload[6] = (sender_id >> 24) & 0xFF;
    raw_payload[7] = (sender_id >> 16) & 0xFF;
    raw_payload[8] = (sender_id >> 8) & 0xFF;
    raw_payload[9] = sender_id & 0xFF;
    payload[8] = (sender_id >> 24) & 0xFF;
    payload[9] = (sender_id >> 16) & 0xFF;
    payload[10] = (sender_id >> 8) & 0xFF;
    payload[11] = sender_id & 0xFF;
    if (type != RREQ_TYP) {
        raw_payload[10] = (receiver_id >> 24) & 0xFF;
        raw_payload[11] = (receiver_id >> 16) & 0xFF;
        raw_payload[12] = (receiver_id >> 8) & 0xFF;
        raw_payload[13] = receiver_id & 0xFF;
        payload[12] = (receiver_id >> 24) & 0xFF;
        payload[13] = (receiver_id >> 16) & 0xFF;
        payload[14] = (receiver_id >> 8) & 0xFF;
        payload[15] = receiver_id & 0xFF;
        len += 4;
        if (type != DACK_TYP && type != NACK_TYP) {
            raw_payload[14] = (source_id >> 24) & 0xFF;
            raw_payload[15] = (source_id >> 16) & 0xFF;
            raw_payload[16] = (source_id >> 8) & 0xFF;
            raw_payload[17] = source_id & 0xFF;
            payload[16] = (source_id >> 24) & 0xFF;
            payload[17] = (source_id >> 16) & 0xFF;
            payload[18] = (source_id >> 8) & 0xFF;
            payload[19] = source_id & 0xFF;
            len += 4;
        }
    }
    if (data_len) {
        for (int i = 0; i < data_len; ++i) {
            raw_payload[i+18] = data[i];
            payload[i+20] = data[i];
        }
    }

    uint16_t crc = crc16_CCITT(raw_payload, 6);
    payload[2] = (crc >> 8) & 0xFF;
    payload[3] = crc & 0xFF;
    // this->payload = (byte**) malloc(sizeof(byte**));
    this->payload = payload;
    this->len = len;
    this->data_len = data_len;
}

Packet::~Packet() {

}

int Packet::getPayload(byte* &payload) {
    payload = this->payload;
    return this->len;
}

int Packet::getLength() {
    return this->len;
}

LoRaSENSE::LoRaSENSE(int id, char** ssid_arr, char** pwd_arr, int wifi_arr_len, long timeout) {
    this->id = id;
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

    //Setup randomizer
    srand(time(0));
}

void LoRaSENSE::loop() {
    if (!connected) {
        connectToNetwork();
    }
}

// TODO: this can be simplified
void LoRaSENSE::connectToNetwork() {
    Serial.println(ROOTABLE);
    if (ROOTABLE) {
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
    }
    if (WiFi.status() != WL_CONNECTED) {
        //Connect to LoRa mesh
        if (!rreqSent) { //Send RREQ packet
            Serial.println("No valid Wi-Fi router in range. Connecting to LoRa mesh");
            WiFi.mode(WIFI_OFF);
            Packet rreq(RREQ_TYP, id, 0, 0, nullptr, 0);
            byte* payload;
            int payload_len = rreq.getPayload(payload);
            LoRa.beginPacket();
            for (int i = 0; i < payload_len; ++i) {
                LoRa.print(payload[i]);
                // Serial.printf("%u ", payload[i]);
                // if ((i % 4) == 3) {
                //     Serial.printf("\n");
                // }
            }
            LoRa.endPacket(true);
            rreqSent = true;
        } else { //Wait for response
            int packetSize = LoRa.parsePacket();
            if (packetSize) {
                int rssi = LoRa.packetRssi();
                byte receivedPacket[packetSize];
                for (int i = 0; LoRa.available(); ++i) {
                    receivedPacket[i] = LoRa.read();
                }
                
            }
        }
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