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
// #define RST 14
#define RST 23
#define DIO0 26

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 433E6

// empty function for initialization of callback function variables
void empty() {

}

/**
 * @brief Appends an array of Data unions to a given byte array.
 * 
 * @param byte_arr the byte array, MUST be instantiated with a capacity that can hold the appended Data bytes
 * @param last_byte_arr_i the byte_arr index of its last element
 * @param data_arr the Data union array to be appended to byte_arr
 * @param data_len the length of data_arr
 * @param data_size the size of each element of new_bytes_arr, MUST be either the size of a float or a double
 * @return int last array index added to byte_arr
 */
int appendDataToByteArray(byte* byte_arr, int last_byte_arr_i, void* data_arr, int data_len, int data_size) {
  // byte_arr = new byte[data_size * data_len];
  int j = last_byte_arr_i;
  for (int i = 0; i < data_len; ++i) {
    byte* data;
    if (data_size == sizeof(Data)) {
      data = ((Data*)data_arr)[i].data_b;
    } else if (data_size == sizeof(Data_d)) {
      data = ((Data_d*)data_arr)[i].data_b;
    } else {
      throw "Not valid data size!";
    }
    for (; j < (last_byte_arr_i + (data_size * (i+1))); ++j) {
      byte_arr[j] = data[(j - last_byte_arr_i) % data_size];
    }
  }
  return j;
}





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

void PacketQueue::pushFront(Packet* packet) {
    PacketQueueNode* newNode = new PacketQueueNode(packet);
    newNode->next = this->head;
    this->head = newNode;
}

Packet* PacketQueue::peekFront() {
    if (this->head != nullptr) {
        Packet* packet = this->head->packet;
        return packet;
    } else {
        throw 0;
    }
}

Packet* PacketQueue::popFront() {
    if (this->head != nullptr) {
        PacketQueueNode* headNode = this->head;
        Packet* packet = headNode->packet;
        this->head = headNode->next;
        delete headNode;
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
    if (getType() == RREQ_TYP || getType() == RERR_TYP || getType() == DACK_TYP) {
        this->data_len = 0;
    } else {
        this->data_len = this->len - 20;
    }
}

Packet::Packet(byte type, int sender_id, int receiver_id, int source_id, byte* data, int data_len) {
    defaultInit(type, rand(), sender_id, receiver_id, source_id, data, data_len);
}

Packet::Packet(Packet* packet, int sender_id, int receiver_id) {
    int data_len = packet->getDataLength();
    byte data[data_len];
    packet->getData(data);
    defaultInit(packet->getType(), packet->getPacketId(), sender_id, receiver_id, packet->getSourceId(), data, data_len);
}

Packet::Packet(Packet* packet, int sender_id, int receiver_id, byte* data, int data_len) {
    defaultInit(packet->getType(), packet->getPacketId(), sender_id, receiver_id, packet->getSourceId(), data, data_len);
}

Packet::~Packet() {
    delete payload;
}

void Packet::defaultInit(byte type, int packet_id, int sender_id, int receiver_id, int source_id, byte* data, int data_len) {
    int len = 12 + data_len; // minimum header (RREQ/RERR) + data
    if (type != RREQ_TYP && type != RERR_TYP) {
        len += 4;
        if (type != DACK_TYP) {
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
        if (type != DACK_TYP) {
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
    
    delete raw_payload;
}

bool Packet::checkCRC() {
    byte raw_payload[len-2];
    uint16_t crc_orig = (payload[2] << 8) | payload[3];

    raw_payload[0] = payload[0];
    raw_payload[1] = payload[1];
    for (int i = 2; i < len-2; ++i) {
        raw_payload[i] = payload[i+2];
    }
    uint16_t crc = crc16_CCITT(raw_payload, 6);

    return crc_orig == crc;
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

char* Packet::getTypeInString() {
    byte type = this->getType();
    switch (type) {
        case RREQ_TYP: return "RREQ";
        case RREP_TYP: return "RREP";
        case RERR_TYP: return "RERR";
        case DATA_TYP: return "DATA";
        case DACK_TYP: return "DACK";
        case RSTA_TYP: return "RSTA";
    }
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

// TODO: Maybe make this a direct reference instead of full-copy?
// OR memory allocation should take place outside this function
void Packet::getData(byte* data) {
    for (int i = 0; i < data_len; ++i) {
        data[i] = payload[i+20];
    }
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

int Packet::getDataLength() {
    return this->data_len;
}






LoRaSENSE::LoRaSENSE(unsigned int* node_ids, char** node_tokens, char** node_rsta_tokens, int nodes, unsigned int id, char** ssid_arr, char** pwd_arr, int wifi_arr_len, long timeout) {
    this->node_ids = node_ids;
    this->node_tokens = node_tokens;
    this->node_rsta_tokens = node_rsta_tokens;
    this->nodes = nodes;
    this->id = id;
    this->ssid_arr = ssid_arr;
    this->pwd_arr = pwd_arr;
    this->wifi_arr_len = wifi_arr_len;
    this->wifiTimeout = timeout;
    this->funcAfterInit = &empty;
    this->funcOnConnecting = &empty;
    this->funcOnConnect = &empty;
    this->funcOnSend = &empty;
}

LoRaSENSE::~LoRaSENSE() {
    delete[] ssid_arr;
    delete[] pwd_arr;
}

void LoRaSENSE::processRreq(Packet* packet) {
    byte data[4]; // hop count
    data[0] = (hopCount >> 24) & 0xFF;
    data[1] = (hopCount >> 16) & 0xFF;
    data[2] = (hopCount >> 8) & 0xFF;
    data[3] = hopCount & 0xFF;
    Packet* rrep = new Packet(RREP_TYP, this->id, packet->getSenderId(), this->id, data, 4);
    this->pushPacketToQueue(rrep);
    Serial.println("RREP packet sent");
}

void LoRaSENSE::processRrep(Packet* packet, int rssi) {
    int sourceId = packet->getSourceId();
    int data_len = packet->getDataLength();
    byte data[data_len];
    packet->getData(data);
    int hopCount = 0;
    hopCount = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    Serial.printf("RREP from %s (hop count: %i, RSSI: %i)\n", String(sourceId, HEX), hopCount, rssi);
    if (hopCount < (this->hopCount - 1) && rssi >= RSSI_THRESH) {
        #ifdef MIN_HOP
        if (hopCount >= MIN_HOP-1) {
        #endif
        // Connected to network via LoRa
        connectTime = millis() - this->startConnectTime;
        this->parent_id = sourceId;
        this->hopCount = hopCount + 1;
        connectingToLora = false;
        connected = true;
        Serial.printf("New parent '%s'\n", String(sourceId, HEX));
        Serial.printf("Hop count: %i\n", this->hopCount);
        Data_l connTime = {connectTime};
        Data_l routeLedger = {this->id};
        Data_l data[] = {connTime, routeLedger};
        byte data_b[2*sizeof(Data_l)];
        int data_len = appendDataToByteArray(data_b, 0, data, 2, sizeof(Data_l));
        Packet* rsta = new Packet(RSTA_TYP, this->id, this->parent_id, this->id, data_b, data_len);
        this->pushPacketToQueue(rsta);
        funcOnConnect();
        #ifdef MIN_HOP
        }
        #endif
    }
}

void LoRaSENSE::processData(Packet* packet) {
    // Send DACK packet
    Packet* dack;
    if (packet->checkCRC()) {
        dack = new Packet(DACK_TYP, this->id, packet->getSenderId(), 0, nullptr, 0);
    }
    this->pushPacketToQueueFront(dack);

    // Add received packet to queue, with updated sender and receiver IDs
    Packet* newPacket = new Packet(packet, this->id, this->parent_id);
    this->pushPacketToQueue(newPacket);
}

void LoRaSENSE::processRerr(Packet* packet) {
    Serial.println("ROUTE ERROR packet received");
    reconnect();
}

void LoRaSENSE::processRsta(Packet* packet) {
    // Send DACK packet
    Packet* dack;
    if (packet->checkCRC()) {
        dack = new Packet(DACK_TYP, this->id, packet->getSenderId(), 0, nullptr, 0);
    }
    this->pushPacketToQueueFront(dack);

    // Add received packet to queue, with updated sender and receiver IDs and route ledger
    int data_len = packet->getDataLength();
    byte data[data_len];
    packet->getData(data);
    int new_data_len = data_len + sizeof(this->id);
    byte new_data[new_data_len];
    Data_l id_data = {this->id};
    for (int i = 0; i < data_len; ++i) {
        new_data[i] = data[i];
    }
    for (int i = 0; i < sizeof(id_data); ++i) {
        new_data[data_len + i] = id_data.data_b[i];
    }
    Packet* newPacket = new Packet(packet, this->id, this->parent_id, new_data, new_data_len);
    this->pushPacketToQueue(newPacket);
}

void LoRaSENSE::processDack(Packet* packet, int rssi) {
    if (rssi < RSSI_THRESH) {
        Serial.println("RSSI below threshold");
        reconnect();
    }
    Serial.println("DACK successfully received");
    waitingForAck = false;
    resent = false;
    Packet* sentPacket = packetQueue.popFront();
    delete sentPacket;
    funcOnSend();
}

void LoRaSENSE::sendPacketViaLora(Packet* packet, bool waitForAck) {
    bool sendSuccess = packet->send();
    if (sendSuccess) {
        waitingForAck = waitForAck;
        lastSendAttempt = millis();
    } else {
        long rand_t = (rand() % RESEND_MAX_TIME);
        nextSendAttempt = millis() + rand_t;
        Serial.printf("Possible collision detected, rescheduling after %ums...\n", rand_t);
    }
    if (waitForAck) {
        Serial.printf("Packet sent, awaiting acknowledgment...\n");
    } else {
        Serial.println("Packet sent.");
        packetQueue.popFront();
        delete packet;
    }
}

void LoRaSENSE::sendPacketToServer(Packet* packet) {
    StaticJsonDocument<256> jsonDoc;
    int data_len = packet->getDataLength();
    byte data[data_len];
    packet->getData(data);
    char* accessToken;

    if (packet->getType() == DATA_TYP) {
        Data pm2_5, pm10, co, temp, humid;
        Data_d lat, lng;
        pm2_5.data_b[0] = data[0];
        pm2_5.data_b[1] = data[1];
        pm2_5.data_b[2] = data[2];
        pm2_5.data_b[3] = data[3];
        pm10.data_b[0] = data[4];
        pm10.data_b[1] = data[5];
        pm10.data_b[2] = data[6];
        pm10.data_b[3] = data[7];
        co.data_b[0] = data[8];
        co.data_b[1] = data[9];
        co.data_b[2] = data[10];
        co.data_b[3] = data[11];
        temp.data_b[0] = data[12];
        temp.data_b[1] = data[13];
        temp.data_b[2] = data[14];
        temp.data_b[3] = data[15];
        humid.data_b[0] = data[16];
        humid.data_b[1] = data[17];
        humid.data_b[2] = data[18];
        humid.data_b[3] = data[19];
        lat.data_b[0] = data[20];
        lat.data_b[1] = data[21];
        lat.data_b[2] = data[22];
        lat.data_b[3] = data[23];
        lat.data_b[4] = data[24];
        lat.data_b[5] = data[25];
        lat.data_b[6] = data[26];
        lat.data_b[7] = data[27];
        lng.data_b[0] = data[28];
        lng.data_b[1] = data[29];
        lng.data_b[2] = data[30];
        lng.data_b[3] = data[31];
        lng.data_b[4] = data[32];
        lng.data_b[5] = data[33];
        lng.data_b[6] = data[34];
        lng.data_b[7] = data[35];
        jsonDoc["packetId"] = packet->getPacketId();
        jsonDoc["sourceId"] = packet->getSourceId();
        jsonDoc["pm2_5"] = pm2_5.data_f;
        jsonDoc["pm10"] = pm10.data_f;
        jsonDoc["co"] = co.data_f;
        jsonDoc["temp"] = temp.data_f;
        jsonDoc["humid"] = humid.data_f;
        jsonDoc["lat"] = lat.data_d;
        jsonDoc["lng"] = lng.data_d;
        for (int i = 0; i < this->nodes; ++i) {
            if (packet->getSourceId() == node_ids[i]) {
                accessToken = node_tokens[i];
            }
        }
    } else if (packet->getType() == RSTA_TYP) {
        int ledger_len = (data_len / sizeof(Data_l)) - 1; // first 4 bytes are for connection time
        Data_l connTime;
        Data_l routeLedger[ledger_len];
        connTime.data_b[0] = data[0];
        connTime.data_b[1] = data[1];
        connTime.data_b[2] = data[2];
        connTime.data_b[3] = data[3];
        jsonDoc["connTime"] = connTime.data_l;
        for (int i = 0; i < ledger_len; ++i) {
            Data_l routeNode = routeLedger[i];
            for (int j = 0; j < sizeof(Data_l); ++j) {
                routeNode.data_b[j] = data[(i+1)*sizeof(Data_l) + j];
            }
            jsonDoc["route"][i] = routeNode.data_l;
        }
        jsonDoc["packetId"] = packet->getPacketId();
        jsonDoc["sourceId"] = packet->getSourceId();
        for (int i = 0; i < this->nodes; ++i) {
            if (packet->getSourceId() == node_ids[i]) {
                accessToken = node_rsta_tokens[i];
            }
        }
    }

    String jsonStr = "";
    serializeJson(jsonDoc, jsonStr);
    // DEBUG
        Serial.print(jsonStr);
        Serial.print("...");
    //
    String endpoint = SERVER_ENDPOINT;
    endpoint.replace("$ACCESS_TOKEN", accessToken);
    httpClient->begin(endpoint);
    beginSendToServer = millis();
    int httpResponseCode = httpClient->POST(jsonStr);
    if (httpResponseCode == 200) {
        Serial.printf("sent");
        packetQueue.popFront();
        delete packet;
        funcOnSend();
    } else if (httpResponseCode >= 400) {
        Serial.printf("error(%i)", httpResponseCode);
        Serial.println(httpClient->getString());
    } else {
        Serial.printf("fatal error(%i)", httpResponseCode);
    }
    Serial.printf(" [time: %lu]\n", millis() - beginSendToServer);
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

    // Finished with initialization
    funcAfterInit();

    //Begin network setup
    funcOnConnecting();
    #ifdef MIN_HOP
    if (MIN_HOP == 0) {
    #endif
        connectToWifi(ssid_arr[0], pwd_arr[0]);
        startConnectTime = millis();
        ++wifi_i;
    #ifdef MIN_HOP
    } else {
        connectToLora();
        startConnectTime = millis();
    }
    #endif
}

void LoRaSENSE::loop() {

    if (connectingToWifi) {
        if ((millis() - lastWifiAttempt) < wifiTimeout && WiFi.status() == WL_CONNECTED) {
            // Connected to Wi-Fi; Node is root
            connectTime = millis() - startConnectTime;
            Serial.println("Connected to Wi-Fi");
            hopCount = 0;
            connected = true;
            connectingToWifi = false;
            httpClient = new HTTPClient();
            Data_l connTime = {connectTime};
            Data_l routeLedger = {this->id};
            Data_l data[] = {connTime, routeLedger};
            byte* data_b = new byte[2*sizeof(Data_l)];
            int data_len = appendDataToByteArray(data_b, 0, data, 2, sizeof(Data_l));
            Packet* rsta = new Packet(RSTA_TYP, this->id, this->id, this->id, data_b, data_len);
            this->pushPacketToQueue(rsta);
            funcOnConnect();
        } else if ((millis() - lastWifiAttempt) >= wifiTimeout && wifi_i < wifi_arr_len) {
            connectToWifi(ssid_arr[wifi_i], pwd_arr[wifi_i]);
            ++wifi_i;
        }
        else if ((millis() - lastWifiAttempt) >= wifiTimeout) {
            // Wi-Fi connect failed. Connect to LoRa mesh
            Serial.println("No valid Wi-Fi router in range.");
            connectingToWifi = false;
            wifi_i = 0;
            WiFi.mode(WIFI_OFF);
            connectToLora();
        }
    }
    else if (connectingToLora) {
        if ((millis() - lastRreqSent) > RREQ_TIMEOUT) {
            // Resend RREQ
            Serial.println("No RREP received.");
            Serial.print("Re-");
            connectToLora();
        }
    } 
    
    if (!connected && ((millis() - lastWifiAttempt) > WIFI_RECONN)
        #ifdef MIN_HOP
        && MIN_HOP == 0
        #endif
    ) {
        // Reconnect to Wi-Fi
        connectToWifi(ssid_arr[0], pwd_arr[0]);
        ++wifi_i;
    }

    // Continuous listen for packets
    if (!connectingToWifi) {
        int packetSize = LoRa.parsePacket();
        if (packetSize) {
            int rssi = LoRa.packetRssi();
            byte packetBuf[packetSize];
            for (int i = 0; LoRa.available(); ++i) {
                packetBuf[i] = LoRa.read();
            }
            Packet* packet = new Packet(packetBuf, packetSize);
            if (packet->checkCRC()) {
                if (packet->getReceiverId() == this->id) {
                    Serial.printf("%s packet %u received from %s (source: %s, RSSI: %i)...\n", 
                        packet->getTypeInString(), packet->getPacketId(), String(packet->getSenderId(), HEX), String(packet->getSourceId(), HEX), rssi);
                }
                if (packet->getType() == RREQ_TYP && connected) {
                    processRreq(packet);
                } else if (packet->getType() == RERR_TYP && packet->getSenderId() == this->getParentId() && connected) {
                    processRerr(packet);
                } else if (packet->getType() == RREP_TYP && packet->getReceiverId() == this->getId()) {
                    processRrep(packet, rssi);
                } else if (packet->getType() == RSTA_TYP && packet->getReceiverId() == this->getId()) {
                    processRsta(packet);
                } else if (packet->getType() == DATA_TYP && packet->getReceiverId() == this->getId()) {
                    processData(packet);
                } else if (packet->getType() == DACK_TYP && packet->getReceiverId() == this->getId()) {
                    processDack(packet, rssi);
                }
            } else {
                Serial.println("Received erroneous packet");
            }
            // Packet MUST be deleted to free memory resources!!
            delete packet;
        }
    }

    // Check if node is currently waiting for a DACK/NACK
    if (waitingForAck) { 
        if (millis() > lastSendAttempt + ACK_TIMEOUT) {
            // ACK timeout
            if (!resent) {
                // Resend data packet
                resent = true;
                Packet* packet = packetQueue.peekFront();
                Serial.printf("No ACK received for %u. Resending...", packet->getPacketId());
                sendPacketViaLora(packet, true);
            } else {
                // TODO: Network reconstruction
                Serial.println("Total data sending failure.");
                reconnect();
            }
        }
    } else {
        sendPacketInQueue();
    }
}

// TODO: this can be simplified
void LoRaSENSE::connectToWifi(char* ssid, char* pwd) {
    WiFi.mode(WIFI_STA);
    Serial.printf("Connecting to Wi-Fi router \"%s\"...", ssid);
    WiFi.begin(ssid, pwd);
    lastWifiAttempt = millis();
    connectingToWifi = true;
}

void LoRaSENSE::connectToLora() {
    Serial.println("Connecting to LoRa mesh...");
    Packet* rreq = new Packet(RREQ_TYP, id, 0, 0, nullptr, 0);
    this->pushPacketToQueueFront(rreq);
    lastRreqSent = millis();
    connectingToLora = true;
}

void LoRaSENSE::reconnect() {
    Serial.println("Reconnecting...");
    
    // Broadcast RERR packet
    Packet* rerr = new Packet(RERR_TYP, this->id, 0, 0, nullptr, 0);
    this->pushPacketToQueueFront(rerr);
    while(!sendPacketInQueue());    // force send RERR packet

    // Reconnection process
    
    // Not all of these are needed to be reset, but I did just to be on the safe side
    hopCount = INT_MAX;
    connectingToWifi = false;
    connectingToLora = false;
    connected = false;
    waitingForAck = false;
    resent = false;
    startConnectTime = 0;
    connectTime = 0;
    wifiTimeout = 0;
    lastRreqSent = 0;
    lastWifiAttempt = 0;
    nextSendAttempt = 0;
    lastSendAttempt = 0;
    //

    funcOnConnecting();
    #ifdef MIN_HOP
    if (MIN_HOP == 0) {
    #endif
        connectToWifi(ssid_arr[0], pwd_arr[0]);
        startConnectTime = millis();
        ++wifi_i;
    #ifdef MIN_HOP
    } else {
        connectToLora();
        startConnectTime = millis();
    }
    #endif
}

bool LoRaSENSE::sendPacketInQueue() {
    if (!packetQueue.isEmpty()) {
        Packet* packet = packetQueue.peekFront();
        if (millis() >= nextSendAttempt && 
                (
                    connectingToLora && packet->getType() == RREQ_TYP || 
                    (connected && (hopCount > 0 || (packet->getType() != DATA_TYP && packet->getType() != RSTA_TYP)))
                )
            ) {
            // Send packet via LoRa
            bool waitForAck = false;
            if (packet->getType() != RREQ_TYP && packet->getType() != RERR_TYP) {
                Serial.printf("Sending %s packet %i to node %s...", packet->getTypeInString(), packet->getPacketId(), String(packet->getReceiverId(), HEX).c_str());
            } else {
                Serial.printf("Broadcasting %s packet %i...", packet->getTypeInString(), packet->getPacketId());
            }
            if (packet->getType() == DATA_TYP || packet->getType() == RSTA_TYP) {
                waitForAck = true;
            }
            sendPacketViaLora(packet, waitForAck);
        } else if (hopCount == 0 && (packet->getType() == DATA_TYP || packet->getType() == RSTA_TYP)) {
            // Send packet via Wi-Fi/HTTP
            Serial.printf("Sending %s packet %i to server...", packet->getTypeInString(), packet->getPacketId());
            sendPacketToServer(packet);
        } else {
            return false;
        }
        return true;
    }
    return false;
}

void LoRaSENSE::pushPacketToQueue(Packet* packet) {
    Serial.printf("Pushing %s packet %u to queue...", packet->getTypeInString(), packet->getPacketId());
    this->packetQueue.push(packet);
    Serial.printf("DONE. %u packet/s currently in queue\n", this->packetQueue.getSize());
}

void LoRaSENSE::pushPacketToQueueFront(Packet* packet) {
    Serial.printf("Pushing %s packet %u to front of queue...", packet->getTypeInString(), packet->getPacketId());
    this->packetQueue.pushFront(packet);
    Serial.printf("DONE. %u packet/s currently in queue\n", this->packetQueue.getSize());
}

void LoRaSENSE::setAfterInit(void funcAfterInit()) {
    this->funcAfterInit = std::bind(funcAfterInit);
}

void LoRaSENSE::setOnConnecting(void funcOnConnecting()) {
    this->funcOnConnecting = std::bind(funcOnConnecting);
}

void LoRaSENSE::setOnConnect(void funcOnConnect()) {
    this->funcOnConnect = std::bind(funcOnConnect);
}

void LoRaSENSE::setOnSend(void funcOnSend()) {
    this->funcOnSend = std::bind(funcOnSend);
}

unsigned int LoRaSENSE::getId() {
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

unsigned long LoRaSENSE::getConnectTime() {
    return connectTime;
}