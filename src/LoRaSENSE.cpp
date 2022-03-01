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

PacketQueueNode::PacketQueueNode(Packet* packet) {
    this->next = nullptr;
    this->packet = packet;
}

PacketQueue::PacketQueue() {
    head = nullptr;
}

int PacketQueue::getSize() {
    int size = 0;
    PacketQueueNode* node = this->head;
    while (node != nullptr) {
        node = node->next;
        ++size;
    }
    return size;
}

bool PacketQueue::isEmpty() {
    if (this->head == nullptr) return true;
    else return false;
}

void PacketQueue::push(Packet* packet) {
    PacketQueueNode* newNode = new PacketQueueNode(packet);
    if (getSize() == 0) {
        this->head = newNode;
    } else {
        PacketQueueNode* node = this->head;
        while (node->next != nullptr) {
            node = node->next;
        }
        node->next = newNode;
    }
}

Packet* PacketQueue::popFront() {
    if (this->head != nullptr) {
        Packet* packet = this->head->packet;
        this->head = this->head->next;
        return packet;
    } else {
        throw 0;
    }
}

Packet::Packet() {

}

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
    defaultInit(type, rand(), sender_id, receiver_id, source_id, data, data_len);
}

Packet::Packet(Packet packet, int sender_id, int receiver_id) {
    byte* data;
    int data_len = packet.getData(data);
    defaultInit(packet.getType(), packet.getPacketId(), sender_id, receiver_id, packet.getSourceId(), data, data_len);
}

Packet::~Packet() {
    delete payload;
}

void Packet::defaultInit(byte type, int packet_id, int sender_id, int receiver_id, int source_id, byte* data, int data_len) {
    int len = 12 + data_len; // minimum header (RREQ/RERR) + data
    if (type != RREQ_TYP && type != RERR_TYP) {
        len += 4;
        if (type != DACK_TYP && type != NACK_TYP) {
            len += 4;
        }
    }
    byte* raw_payload = new byte[len-2]; // excludes checksum
    byte* payload = new byte[len];

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
        // len += 4;
        if (type != DACK_TYP && type != NACK_TYP) {
            raw_payload[14] = (source_id >> 24) & 0xFF;
            raw_payload[15] = (source_id >> 16) & 0xFF;
            raw_payload[16] = (source_id >> 8) & 0xFF;
            raw_payload[17] = source_id & 0xFF;
            payload[16] = (source_id >> 24) & 0xFF;
            payload[17] = (source_id >> 16) & 0xFF;
            payload[18] = (source_id >> 8) & 0xFF;
            payload[19] = source_id & 0xFF;
            // len += 4;
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

bool Packet::send() {
    // Detect if there is a packet in the air first before sending
    if (LoRa.parsePacket()) {
        return false;
    }
    LoRa.beginPacket();
    for (int i = 0; i < len; ++i) {
        LoRa.write(payload[i]);
    }
    LoRa.endPacket();
    return true;
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

int Packet::getPacketId() {
    int packetId = 0;
    packetId = packetId | (this->payload[4] << 24);
    packetId = packetId | (this->payload[5] << 16);
    packetId = packetId | (this->payload[6] << 8);
    packetId = packetId | this->payload[7];
    return packetId;
}

// TODO: COPY nalang instead of REFERENCE
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

int Packet::getReceiverId() {
    int receiverId = 0;
    receiverId = receiverId | (this->payload[12] << 24);
    receiverId = receiverId | (this->payload[13] << 16);
    receiverId = receiverId | (this->payload[14] << 8);
    receiverId = receiverId | this->payload[15];
    return receiverId;
}

int Packet::getLength() {
    return this->len;
}






LoRaSENSE::LoRaSENSE(String thingsboard_access_token, int id, char** ssid_arr, char** pwd_arr, int wifi_arr_len, long timeout) {
    this->thingsboard_access_token = thingsboard_access_token;
    this->id = id;
    this->ssid_arr = ssid_arr;
    this->pwd_arr = pwd_arr;
    this->wifi_arr_len = wifi_arr_len;
    this->wifiTimeout = timeout;
}

LoRaSENSE::~LoRaSENSE() {
    delete[] ssid_arr;
    delete[] pwd_arr;
}

void LoRaSENSE::processRreq(Packet packet) {
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

void LoRaSENSE::processRrep(Packet packet, int rssi) {
    int sourceId = packet.getSourceId();
    byte* data;
    int data_len = packet.getData(data);
    int hopCount = 0;
    hopCount = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    if (hopCount < (this->hopCount - 1) && rssi >= -90) {
        this->parent_id = sourceId;
        this->hopCount = hopCount + 1;
        connected = true;
        Serial.printf("New parent '%s'\n", String(sourceId, HEX));
        Serial.printf("Hop count: %i\n", this->hopCount);
        funcOnConnect();
    }
}

void LoRaSENSE::processData(Packet packet) {
    Serial.println("Adding packet to queue...");
    byte* data;
    int data_len = packet.getData(data);
    Packet* newPacket = new Packet(packet.getType(), this->id, this->parent_id, packet.getSourceId(), data, data_len);
    packetQueue.push(newPacket);
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
    rand();
}

void LoRaSENSE::loop() {
    // TESTING
        if (connected && ((millis() - lastDataSent) > DATA_SEND)) {
            long long* data = new long long[5];
            data[0] = rand();   // pm2.5
            data[1] = rand();   // pm10
            data[2] = rand();   // co
            data[3] = rand();   // temp
            data[4] = rand();   // humid
            Packet* dataPkt = new Packet(DATA_TYP, this->id, this->parent_id, this->id, reinterpret_cast<byte*>(data), sizeof(long long)*5);
            Serial.printf("Adding test data packet %i to queue...\n", dataPkt->getPacketId());
            packetQueue.push(dataPkt);
        }
    //

    // TODO: this if-then-else block can be simplified
    if (!connected && !rreqSent) {
        connectToNetwork();
    } else if (!connected && ((millis() - lastRreqSent) > RREQ_TIMEOUT)) {
        rreqSent = false;
        connectToLoRa();
    } else if (!connected && ((millis() - lastWifiAttempt) > WIFI_RECONN)) {
        connectToNetwork();
    } else {
        // Continuous listen for packets
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
                // processRreq(packet);
                byte data[4]; // hop count
                data[0] = (hopCount >> 24) & 0xFF;
                data[1] = (hopCount >> 16) & 0xFF;
                data[2] = (hopCount >> 8) & 0xFF;
                data[3] = hopCount & 0xFF;
                Packet* rrep = new Packet(RREP_TYP, this->id, packet.getSenderId(), this->id, data, 4);
                // rrep.send();
                packetQueue.push(rrep);
                Serial.println("RREP packet sent");
                delay(1000);
            } else if (packet.getType() == RREP_TYP) {
                // processRrep(packet, rssi);
                int sourceId = packet.getSourceId();
                byte* data;
                int data_len = packet.getData(data);
                int hopCount = 0;
                hopCount = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
                Serial.printf("RREP from %s (hop count: %i)\n", String(sourceId, HEX), hopCount);
                if (hopCount < (this->hopCount - 1) && rssi >= -90) {
                    #ifdef MIN_HOP
                    if (hopCount >= MIN_HOP-1) {
                    #endif
                    this->parent_id = sourceId;
                    this->hopCount = hopCount + 1;
                    connected = true;
                    Serial.printf("New parent '%s'\n", String(sourceId, HEX));
                    Serial.printf("Hop count: %i\n", this->hopCount);
                    funcOnConnect();
                    #ifdef MIN_HOP
                    }
                    #endif
                }
            } else if (packet.getType() == DATA_TYP) {
                Serial.println("DATA");
                // processData(packet);
                Serial.println("Adding packet to queue...");
                byte* data;
                int data_len = packet.getData(data);
                Packet* newPacket = new Packet(packet, this->id, this->parent_id);
                packetQueue.push(newPacket);
            }
        }
    }
    // Send packets from packet queue
    if (/*connected && */!packetQueue.isEmpty()) {
        Serial.printf("Sending data packets...%i packet/s in queue\n", packetQueue.getSize());
        lastDataSent = millis();
        while (!packetQueue.isEmpty()) {
            if (millis() < nextSendAttempt) {
                break;
            }
            Packet* packet = packetQueue.popFront();
            if (hopCount > 0 || (packet->getType() != DATA_TYP && packet->getType() != RSTA_TYP)) {
                // TODO: continue here!!
                if (packet->getType() != RREQ_TYP && packet->getType() != RERR_TYP) {
                    Serial.printf("Sending packet %i to node %s...", packet->getPacketId(), String(packet->getReceiverId(), HEX).c_str());
                } else {
                    Serial.printf("Broadcasting packet %i...", packet->getPacketId());
                }
                bool sendSuccess = packet->send();
                if (!sendSuccess) {
                    long rand_t = 369 + (rand() % 369);
                    nextSendAttempt = millis() + rand_t;
                    Serial.printf("packet detected, rescheduling after %ums...\n", rand_t);
                    break;
                } else {
                    Serial.printf("DONE\n");
                }
            } else {
                Serial.printf("Sending packet %i to server...", packet->getPacketId());
                StaticJsonDocument<256> jsonDoc;
                byte* data;
                int data_len = packet->getData(data);
                // TODO: fix this, perhaps with type punning?
                long long pm2_5 = 0, pm10 = 0, co = 0, temp = 0, humid = 0;
                pm2_5 = pm2_5 | (data[7] << 56);
                pm2_5 = pm2_5 | (data[6] << 48);
                pm2_5 = pm2_5 | (data[5] << 40);
                pm2_5 = pm2_5 | (data[4] << 32);
                pm2_5 = pm2_5 | (data[3] << 24);
                pm2_5 = pm2_5 | (data[2] << 16);
                pm2_5 = pm2_5 | (data[1] << 8);
                pm2_5 = pm2_5 | (data[0] << 0);
                pm10 = pm10 | (data[15] << 56);
                pm10 = pm10 | (data[14] << 48);
                pm10 = pm10 | (data[13] << 40);
                pm10 = pm10 | (data[12] << 32);
                pm10 = pm10 | (data[11] << 24);
                pm10 = pm10 | (data[10] << 16);
                pm10 = pm10 | (data[9] << 8);
                pm10 = pm10 | (data[8] << 0);
                co = co | (data[23] << 56);
                co = co | (data[22] << 48);
                co = co | (data[21] << 40);
                co = co | (data[20] << 32);
                co = co | (data[19] << 24);
                co = co | (data[18] << 16);
                co = co | (data[17] << 8);
                co = co | (data[16] << 0);
                temp = temp | (data[31] << 56);
                temp = temp | (data[30] << 48);
                temp = temp | (data[29] << 40);
                temp = temp | (data[28] << 32);
                temp = temp | (data[27] << 24);
                temp = temp | (data[26] << 16);
                temp = temp | (data[25] << 8);
                temp = temp | (data[24] << 0);
                humid = humid | (data[39] << 56);
                humid = humid | (data[38] << 48);
                humid = humid | (data[37] << 40);
                humid = humid | (data[36] << 32);
                humid = humid | (data[35] << 24);
                humid = humid | (data[34] << 16);
                humid = humid | (data[33] << 8);
                humid = humid | (data[32] << 0);
                jsonDoc["packetId"] = packet->getPacketId();
                jsonDoc["pm2_5"] = pm2_5;
                jsonDoc["pm10"] = pm10;
                jsonDoc["co"] = co;
                jsonDoc["temp"] = temp;
                jsonDoc["humid"] = humid;
                String jsonStr = "";
                serializeJson(jsonDoc, jsonStr);
                // TODO: maybe this can be optimized further? (ie. initialization of HTTPClient)
                HTTPClient httpClient;
                String endpoint = SERVER_ENDPOINT;
                endpoint.replace("$ACCESS_TOKEN", this->thingsboard_access_token);
                // DEBUGGING
                    // Serial.printf("%s...", jsonStr.c_str());
                //
                httpClient.begin(endpoint);
                int httpResponseCode = httpClient.POST(jsonStr);
                if (httpResponseCode == 200) {
                    Serial.println("sent");
                } else if (httpResponseCode >= 400) {
                    Serial.printf("error(%i)\n", httpResponseCode);
                    Serial.println(httpClient.getString());
                } else {
                    Serial.printf("fatal error(%i)\n", httpResponseCode);
                }
                httpClient.end();
            }
            delete packet;
        }
    }
}

// TODO: this can be simplified
void LoRaSENSE::connectToNetwork() {
    if (ROOTABLE) {
        for (int i = 0; i < wifi_arr_len; ++i) {
            Serial.printf("Connecting to Wi-Fi router \"%s\"", ssid_arr[i]);
            WiFi.begin(ssid_arr[i], pwd_arr[i]);
            for (int time = 0; WiFi.status() != WL_CONNECTED && time < wifiTimeout; time += 500) {
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
        lastWifiAttempt = millis();
    }
    if (WiFi.status() != WL_CONNECTED) {
        //Connect to LoRa mesh
        Serial.println("No valid Wi-Fi router in range.");
        connectToLoRa();
    } else {
        //Node is root
        hopCount = 0;
        connected = true;
        funcOnConnect();
    }
}

void LoRaSENSE::connectToLoRa() {
    WiFi.mode(WIFI_OFF);
    Serial.println("Connecting to LoRa mesh...");
    Packet* rreq = new Packet(RREQ_TYP, id, 0, 0, nullptr, 0);
    packetQueue.push(rreq);
    // rreq.send();
    // Serial.println("RREQ packet sent!");
    rreqSent = true;
    lastRreqSent = millis();
}

void LoRaSENSE::setOnConnect(void funcOnConnect()) {
    this->funcOnConnect = std::bind(funcOnConnect);
}

int LoRaSENSE::getId() {
    return id;
}

int LoRaSENSE::getParentId() {
    return parent_id;
}

int LoRaSENSE::getHopCount() {
    return hopCount;
}

bool LoRaSENSE::isConnected() {
    return connected;
}