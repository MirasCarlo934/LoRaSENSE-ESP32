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

Packet::Packet(byte* payload, int len) {
    byte* newPayload = new byte[len];
    for (int i = 0; i < len; ++i) {
        newPayload[i] = payload[i];
    }
    this->payload = newPayload;
    this->len = len;
    if (getType() == RREQ_TYP || getType() == RERR_TYP || getType() == DACK_TYP || getType() == NACK_TYP) {
        this->data_len = 0;
    } else {
        this->data_len = this->len - 20;
    }
}

Packet::Packet(byte type, int sender_id, int receiver_id, int source_id, byte* data, int data_len) {
    int len = 12 + data_len; // minimum header (RREQ/RERR) + data
    if (type == DACK_TYP || type == NACK_TYP) {
        len += 4;
    } else {
        len += 8;
    }
    byte* raw_payload = new byte[len-2]; // excludes checksum
    byte* payload = new byte[len];
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
    if (type != RREQ_TYP && type != RERR_TYP) {
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
    this->payload = payload;
    this->len = len;
    this->data_len = data_len;
}

Packet::~Packet() {
    delete payload;
}

void Packet::send() {
    LoRa.beginPacket();
    for (int i = 0; i < len; ++i) {
        LoRa.write(payload[i]);
    }
    LoRa.endPacket();
}

void Packet::printToSerial() {
    Serial.println("---BEGIN PACKET---");
    for (int i = 0; i < this->len; ++i) {
        Serial.printf("%u ", this->payload[i]);
        if ((i % 4) == 3) {
            Serial.printf("\n");
        }
    }
    Serial.println("----END PACKET----");
}

byte Packet::getType() {
    return this->payload[0] >> 5;
}

// TODO: COPY nalang
int Packet::getPayload(byte* &payload) {
    payload = this->payload;
    return this->len;
}

int Packet::getData(byte* &data) {
    data = new byte[data_len];
    for (int i = 0; i < data_len; ++i) {
        data[i] = payload[i+20];
    }
    return data_len;
}

int Packet::getSenderId() {
    int senderId = 0;
    senderId = senderId | (this->payload[8] << 24);
    senderId = senderId | (this->payload[9] << 16);
    senderId = senderId | (this->payload[10] << 8);
    senderId = senderId | this->payload[11];
    return senderId;
}

int Packet::getSourceId() {
    int sourceId = 0;
    sourceId = sourceId | (this->payload[16] << 24);
    sourceId = sourceId | (this->payload[17] << 16);
    sourceId = sourceId | (this->payload[18] << 8);
    sourceId = sourceId | this->payload[19];
    return sourceId;
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
    delete[] ssid_arr;
    delete[] pwd_arr;
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
    if (!connected && !rreqSent) {
        connectToNetwork();
    } else {
        // Continuous listen for packets
        // TODO: add CRC!!!
        int packetSize = LoRa.parsePacket();
        if (packetSize) {
            Serial.print("Packet received...");
            int rssi = LoRa.packetRssi();
            byte packetBuf[packetSize];
            for (int i = 0; LoRa.available(); ++i) {
                packetBuf[i] = LoRa.read();
            }
            Packet packet(packetBuf, packetSize);
            if (packet.getType() == RREQ_TYP) {
                Serial.println("RREQ");
                byte data[4]; // hop count
                data[0] = (hopCount >> 24) & 0xFF;
                data[1] = (hopCount >> 16) & 0xFF;
                data[2] = (hopCount >> 8) & 0xFF;
                data[3] = hopCount & 0xFF;
                Packet rrep(RREP_TYP, this->id, packet.getSenderId(), this->id, data, 4);
                rrep.send();
                Serial.println("RREP packet sent");
                delay(1000);
            }
            else if (packet.getType() == RREP_TYP) {
                Serial.println("RREP");
                int sourceId = packet.getSourceId();
                byte* data;
                int data_len = packet.getData(data);
                int hopCount = 0;
                hopCount = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
                if (hopCount < (this->hopCount - 1) && rssi >= -90) {
                    this->parent_id = sourceId;
                    this->hopCount = hopCount + 1;
                    Serial.printf("New parent '%s'\n", String(sourceId, HEX));
                    Serial.printf("Hop count: %i", this->hopCount);
                    funcOnConnect();
                }
            }
        }
    }
}

// TODO: this can be simplified
void LoRaSENSE::connectToNetwork() {
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
        Serial.println("No valid Wi-Fi router in range. Connecting to LoRa mesh");
        WiFi.mode(WIFI_OFF);
        Packet rreq(RREQ_TYP, id, 0, 0, nullptr, 0);
        rreq.send();
        Serial.println("RREQ packet sent!");
        rreqSent = true;
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

int LoRaSENSE::getId() {
    return id;
}

int LoRaSENSE::getHopCount() {
    return hopCount;
}

bool LoRaSENSE::isConnected() {
    return connected;
}