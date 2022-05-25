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

LoRaSENSE* loraSense;

// empty function for initialization of callback function variables
void empty() {

}

// this function is needed due to limitations on the function pointer 
// accepted by the LoRa.setOnReceive() function
void loraOnReceive(int packetSize) {
    loraSense->onLoraReceive(packetSize);
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
        Serial.println("PacketQueue already empty!");
        throw 0;
    }
}

Packet* PacketQueue::peek(int position) {
    // DEBUG
        // Serial.println("PEEK HAHA");
    //
    if (isEmpty()) {
        Serial.println("PacketQueue empty!");
        throw 0;
    }
    if (position == 0) {
        // DEBUG
            // Serial.println("RETURNING FRONT PEEK");
        //
        return peekFront();
    }
    int i = 0;
    PacketQueueNode* node = this->head;
    while (i < position) {
        node = node->next;
        ++i;
        // DEBUG
            // Serial.println("PEEK " + i);
        //
    }
    Packet* packet = node->packet;
    return packet;
}

Packet* PacketQueue::pop(int position) {
    // DEBUG
        // Serial.println("POP HAHA");
    //
    if (isEmpty()) {
        Serial.println("PacketQueue empty!");
        throw 0;
    }
    if (position == 0) {
        // DEBUG
            // Serial.println("RETURNING FRONT POP");
        //
        return popFront();
    }
    int i = 0;
    PacketQueueNode* prev;
    PacketQueueNode* node = this->head;
    while (i < position) {
        prev = node;
        node = node->next;
        ++i;
        // DEBUG
            // Serial.println("POP " + i);
        //
    }
    Packet* packet = node->packet;
    prev->next = node->next;
    delete node;
    return packet;
}



Packet::Packet() {

}

/**
 * @brief Construct a new Packet:: Packet object with existing payload byte array and payload length
 * 
 * @param payload 
 * @param len 
 */
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

/**
 * @brief Construct a new Packet:: Packet object. Default constructor
 * 
 * @param type 
 * @param sender_id 
 * @param receiver_id 
 * @param source_id 
 * @param data 
 * @param data_len 
 */
Packet::Packet(byte type, int sender_id, int receiver_id, int source_id, byte* data, int data_len) {
    int packet_id = esp_random();
    packet_id /= 10;
    packet_id *= 10;
    if (source_id == 0xAAAAAAAA) {
        packet_id += 0;
    } else if (source_id == 0xBBBBBBBB) {
        packet_id += 1;
    } else if (source_id == 0xCCCCCCCC) {
        packet_id += 2;
    } else if (source_id == 0xDDDDDDDD) {
        packet_id += 3;
    } else {
        packet_id += 4;
    }
    defaultInit(type, packet_id, sender_id, receiver_id, source_id, data, data_len);
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
    // Send packet via LoRa
    LoRa.beginPacket();
    for (int i = 0; i < len; ++i) {
        LoRa.write(payload[i]);
    }
    LoRa.endPacket();
    // Keep LoRa in continuous receive mode after sending
    LoRa.receive();
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
        case NETR_TYP: return "NETR";
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

void Packet::setReceiverId(int receiver_id) {
    if (getType() != RREQ_TYP && getType() != RERR_TYP) {
        payload[12] = (receiver_id >> 24) & 0xFF;
        payload[13] = (receiver_id >> 16) & 0xFF;
        payload[14] = (receiver_id >> 8) & 0xFF;
        payload[15] = receiver_id & 0xFF;
    }
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

void Packet::setRssi(int rssi) {
    this->rssi = rssi;
}

int Packet::getRssi() {
    return this->rssi;
}

void Packet::setSendTime(long send_time) {
    this->send_time = send_time;
}

long Packet::getSendTime() {
    return this->send_time;
}






LoRaSENSE::LoRaSENSE(unsigned int* node_ids, char** node_tokens, char** node_rsta_tokens, char** node_netr_tokens, int nodes, unsigned int id, char** ssid_arr, char** pwd_arr, int wifi_arr_len, bool wifi_only, int min_hop, int max_hop, unsigned long wifi_timeout, unsigned long rreq_timeout, unsigned long rrep_delay_max, unsigned long dack_timeout, unsigned long rreq_limit, unsigned long cycle_time) {
    this->node_ids = node_ids;
    this->node_tokens = node_tokens;
    this->node_rsta_tokens = node_rsta_tokens;
    this->node_netr_tokens = node_netr_tokens;
    this->nodes = nodes;
    this->id = id;
    this->ssid_arr = ssid_arr;
    this->pwd_arr = pwd_arr;
    this->wifi_arr_len = wifi_arr_len;
    this->wifi_only = wifi_only;
    this->min_hop = min_hop;
    this->max_hop = max_hop;
    this->wifiTimeout = wifi_timeout;
    this->rreqTimeout = rreq_timeout;
    this->rrepDelayMax = rrepDelayMax;
    this->rreqLimit = rreqLimit;
    this->cycleTime = cycle_time;
    this->funcAfterInit = &empty;
    this->funcOnConnecting = &empty;
    this->funcOnConnect = &empty;
    this->funcOnSendSuccess = &empty;
    this->funcOnSend = &empty;

    loraSense = this;
}

LoRaSENSE::~LoRaSENSE() {
    delete[] ssid_arr;
    delete[] pwd_arr;
}

void LoRaSENSE::processRreq(Packet* packet) {
    byte* data = new byte[4]; // hop count
    // byte data[4]; 
    data[0] = (hopCount >> 24) & 0xFF;
    data[1] = (hopCount >> 16) & 0xFF;
    data[2] = (hopCount >> 8) & 0xFF;
    data[3] = hopCount & 0xFF;
    Packet* rrep = new Packet(RREP_TYP, this->id, packet->getSenderId(), this->id, data, 4);
    rrep->setSendTime(millis() + (esp_random() % rrepDelayMax));
    this->pushPacketToQueue(rrep);

    delete data;
}

void LoRaSENSE::processRrep(Packet* packet) {
    int sourceId = packet->getSourceId();
    int data_len = packet->getDataLength();
    byte* data = new byte[data_len];

    packet->getData(data);
    int hopCount = 0;
    hopCount = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    Serial.printf("RREP from %s (hop count: %i, RSSI: %i)\n", String(sourceId, HEX), hopCount, packet->getRssi());
    if (sourceId != this->parent_id && hopCount < (this->hopCount - 1) && packet->getRssi() >= RSSI_THRESH) {
        if (hopCount >= min_hop-1 && hopCount <= max_hop-1) {
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
            Data_l* data = new Data_l[2];
            data[0] = connTime;
            data[1] = routeLedger;
            byte* data_b = new byte[2*sizeof(Data_l)];
            int data_len = appendDataToByteArray(data_b, 0, data, 2, sizeof(Data_l));
            Packet* rsta = new Packet(RSTA_TYP, this->id, this->parent_id, this->id, data_b, data_len);
            // nextSendAttempt = millis() + (esp_random() % cycleTime);
            this->pushPacketToQueueFront(rsta);
            delete data;
            funcOnConnect();
        }
    }
    delete data;
}

void LoRaSENSE::processData(Packet* packet) {
    // Send DACK packet
    Packet* dack = new Packet(DACK_TYP, this->id, packet->getSenderId(), 0, nullptr, 0);
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
    Packet* dack = new Packet(DACK_TYP, this->id, packet->getSenderId(), 0, nullptr, 0);
    this->pushPacketToQueueFront(dack);

    // Add received packet to queue, with updated sender and receiver IDs and route ledger
    int data_len = packet->getDataLength();
    byte* data = new byte[data_len];
    packet->getData(data);
    int new_data_len = data_len + sizeof(this->id);
    byte* new_data = new byte[new_data_len];
    Data_l id_data = {this->id};
    for (int i = 0; i < data_len; ++i) {
        new_data[i] = data[i];
    }
    for (int i = 0; i < sizeof(id_data); ++i) {
        new_data[data_len + i] = id_data.data_b[i];
    }
    Packet* newPacket = new Packet(packet, this->id, this->parent_id, new_data, new_data_len);
    this->pushPacketToQueue(newPacket);

    delete data;
    delete new_data;
}

void LoRaSENSE::processDack(Packet* packet) {
    if (packet->getRssi() < RSSI_THRESH) {
        Serial.println("RSSI below threshold");
        reconnect();
    }
    Serial.println("DACK successfully received");
    waitingForAck = false;
    resent = false;
    funcOnSendSuccess();
    Packet* sentPacket = packetQueue.popFront();
    // totalBytesSentSuccessfully += sentPacket->getLength();
    delete sentPacket;
}

void LoRaSENSE::processNetr(Packet* packet) {
    // Send DACK packet
    Packet* dack = new Packet(DACK_TYP, this->id, packet->getSenderId(), 0, nullptr, 0);
    this->pushPacketToQueueFront(dack);

    // Add received packet to queue, with updated sender and receiver IDs
    Packet* newPacket = new Packet(packet, this->id, this->parent_id);
    this->pushPacketToQueue(newPacket);
}

void LoRaSENSE::sendPacketViaLora(Packet* packet, bool waitForAck) {
    // Serial.printf("Sending %s packet with ID: %i\n", packet->getTypeInString(), packet->getPacketId()); 
    bool sendSuccess = packet->send();
    if (sendSuccess) {
        waitingForAck = waitForAck;
        lastSendAttempt = millis();
        // totalSends++;
        funcOnSend();
    } else {
        long rand_delay = (esp_random() % cycleTime);
        // nextSendAttempt = millis() + rand_t;
        packet->setSendTime(millis() + rand_delay);
        Serial.printf("Possible collision detected, rescheduling after %ums...\n", rand_delay);
        return;
    }
    if (waitForAck) {
        Serial.printf("Packet sent, awaiting acknowledgment...\n");
    } else {
        Serial.println("Packet sent.");
        funcOnSendSuccess();
        packetQueue.popFront();
        // totalBytesSentSuccessfully += packet->getLength();
        delete packet;
    }
}

void LoRaSENSE::sendPacketToServer(Packet* packet) {
    DynamicJsonDocument jsonDoc(2048);
    int data_len = packet->getDataLength();
    byte data[data_len];
    packet->getData(data);
    char* accessToken;

    if (packet->getType() == DATA_TYP) {
        Data pm1, pm2_5, pm10, co, temp, humid;
        Data_d lat, lng;
        Data_l sends, bytes;
        pm1.data_b[0] = data[0];
        pm1.data_b[1] = data[1];
        pm1.data_b[2] = data[2];
        pm1.data_b[3] = data[3];
        pm2_5.data_b[0] = data[4];
        pm2_5.data_b[1] = data[5];
        pm2_5.data_b[2] = data[6];
        pm2_5.data_b[3] = data[7];
        pm10.data_b[0] = data[8];
        pm10.data_b[1] = data[9];
        pm10.data_b[2] = data[10];
        pm10.data_b[3] = data[11];
        co.data_b[0] = data[12];
        co.data_b[1] = data[13];
        co.data_b[2] = data[14];
        co.data_b[3] = data[15];
        temp.data_b[0] = data[16];
        temp.data_b[1] = data[17];
        temp.data_b[2] = data[18];
        temp.data_b[3] = data[19];
        humid.data_b[0] = data[20];
        humid.data_b[1] = data[21];
        humid.data_b[2] = data[22];
        humid.data_b[3] = data[23];
        lat.data_b[4] = data[24];
        lat.data_b[5] = data[25];
        lat.data_b[6] = data[26];
        lat.data_b[7] = data[27];
        lat.data_b[0] = data[28];
        lat.data_b[1] = data[29];
        lat.data_b[2] = data[30];
        lat.data_b[3] = data[31];
        lng.data_b[4] = data[32];
        lng.data_b[5] = data[33];
        lng.data_b[6] = data[34];
        lng.data_b[7] = data[35];
        lng.data_b[4] = data[36];
        lng.data_b[5] = data[37];
        lng.data_b[6] = data[38];
        lng.data_b[7] = data[39];
        sends.data_b[0] = data[40];
        sends.data_b[1] = data[41];
        sends.data_b[2] = data[42];
        sends.data_b[3] = data[43];
        bytes.data_b[0] = data[44];
        bytes.data_b[1] = data[45];
        bytes.data_b[2] = data[46];
        bytes.data_b[3] = data[47];
        jsonDoc["packetId"] = packet->getPacketId();
        jsonDoc["sourceId"] = packet->getSourceId();
        jsonDoc["pm1"] = pm1.data_f;
        jsonDoc["pm2_5"] = pm2_5.data_f;
        jsonDoc["pm10"] = pm10.data_f;
        jsonDoc["co"] = co.data_f;
        jsonDoc["temp"] = temp.data_f;
        jsonDoc["humid"] = humid.data_f;
        jsonDoc["lat"] = lat.data_d;
        jsonDoc["lng"] = lng.data_d;
        jsonDoc["sends"] = sends.data_l;
        jsonDoc["bytes"] = bytes.data_l;
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
    } else if (packet->getType() == NETR_TYP) {
        int record_len = (data_len / (2*sizeof(Data_l)));   // number of packet ID-RTT pairs in the packet
        // DEBUG
            // Serial.printf("TEST1: record_len=%i\n", record_len);
        //
        // Data_l packetIds[record_len / 2];
        // Data_l packetRtts[record_len / 2];
        // DEBUG
            // Serial.printf("TEST2: data_len=%i\n", data_len);
        //
        Data_l sends, succSends, origSends, succOrigSends, rtt, origRtt, bytes;
        sends.data_b[0] = data[0];
        sends.data_b[1] = data[1];
        sends.data_b[2] = data[2];
        sends.data_b[3] = data[3];
        succSends.data_b[0] = data[4];
        succSends.data_b[1] = data[5];
        succSends.data_b[2] = data[6];
        succSends.data_b[3] = data[7];
        origSends.data_b[0] = data[8];
        origSends.data_b[1] = data[9];
        origSends.data_b[2] = data[10];
        origSends.data_b[3] = data[11];
        succOrigSends.data_b[0] = data[12];
        succOrigSends.data_b[1] = data[13];
        succOrigSends.data_b[2] = data[14];
        succOrigSends.data_b[3] = data[15];
        rtt.data_b[0] = data[16];
        rtt.data_b[1] = data[17];
        rtt.data_b[2] = data[18];
        rtt.data_b[3] = data[19];
        origRtt.data_b[0] = data[20];
        origRtt.data_b[1] = data[21];
        origRtt.data_b[2] = data[22];
        origRtt.data_b[3] = data[23];
        bytes.data_b[0] = data[24];
        bytes.data_b[1] = data[25];
        bytes.data_b[2] = data[26];
        bytes.data_b[3] = data[27];
        for (int i = 0; i < record_len; ++i) {
            Data_l packetId;
            Data_l packetRtt;
            int j = i * 2*sizeof(Data_l) + (7 * sizeof(Data_l));   // the first 8*sizeof(Data_l) bytes are for sends, succSends, origSends, succOrigSends, rtt, origRtt, bytes 
            packetId.data_b[0] = data[j];
            packetId.data_b[1] = data[j + 1];
            packetId.data_b[2] = data[j + 2];
            packetId.data_b[3] = data[j + 3];
            packetRtt.data_b[0] = data[j + 4];
            packetRtt.data_b[1] = data[j + 5];
            packetRtt.data_b[2] = data[j + 6];
            packetRtt.data_b[3] = data[j + 7];
            // DEBUG
                // Serial.printf("i: %i; j: %i\n", i, j);
                // Serial.printf("%i, %i\n", packetId.data_l, packetRtt.data_l);
            //
            jsonDoc["packetIds"][i] = packetId.data_l;
            jsonDoc["packetRtts"][i] = packetRtt.data_l;
        }
        jsonDoc["nodeId"] = packet->getSourceId();
        jsonDoc["sends"] = sends.data_l;
        jsonDoc["succSends"] = succSends.data_l;
        jsonDoc["origSends"] = origSends.data_l;
        jsonDoc["succOrigSends"] = succOrigSends.data_l;
        jsonDoc["rtt"] = rtt.data_l;
        jsonDoc["origRtt"] = origRtt.data_l;
        jsonDoc["bytes"] = bytes.data_l;
        // DEBUG
            if (!jsonDoc["nodeId"].set(packet->getSourceId())) {
                Serial.printf("nodeId: %i\n", packet->getSourceId());
            }
            if (!jsonDoc["sends"].set(sends.data_l)) {
                Serial.printf("sends: %i\n", sends.data_l);
            }
            if (!jsonDoc["succSends"].set(succSends.data_l)) {
                Serial.printf("succSends: %i\n", succSends.data_l);
            }
            if (!jsonDoc["origSends"].set(origSends.data_l)) {
                Serial.printf("origSends: %i\n", origSends.data_l);
            }
            if (!jsonDoc["succOrigSends"].set(succOrigSends.data_l)) {
                Serial.printf("succOrigSends: %i\n", succOrigSends.data_l);
            }
            if (!jsonDoc["rtt"].set(rtt.data_l)) {
                Serial.printf("rtt: %i\n", rtt.data_l);
            }
            if (!jsonDoc["origRtt"].set(origRtt.data_l)) {
                Serial.printf("origRtt: %i\n", origRtt.data_l);
            }
            if (!jsonDoc["bytes"].set(bytes.data_l)) {
                Serial.printf("bytes: %i\n", bytes.data_l);
            }
        // DEBUG
            // Serial.println("TEST3");
        //
        for (int i = 0; i < this->nodes; ++i) {
            if (packet->getSourceId() == node_ids[i]) {
                accessToken = node_netr_tokens[i];
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
    // totalSends++;
    funcOnSend();
    int httpResponseCode = httpClient->POST(jsonStr);
    if (httpResponseCode == 200) {
        Serial.printf("sent [time: %lu]\n", millis() - beginSendToServer);
        funcOnSendSuccess();
        packetQueue.popFront();
        // totalBytesSentSuccessfully += packet->getLength();
        delete packet;
    } else if (httpResponseCode >= 400) {
        Serial.printf("error(%i)", httpResponseCode);
        Serial.println(httpClient->getString());
    } else {
        Serial.printf("fatal error(%i)", httpResponseCode);
    }
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
    LoRa.receive();
    LoRa.onReceive(loraOnReceive);
    Serial.println("LoRa initialized");

    //Setup Wi-Fi
    WiFi.mode(WIFI_STA);
    Serial.println("WiFi initialized");

    // Finished with initialization
    funcAfterInit();

    //Begin network setup
    funcOnConnecting();
    if (min_hop == 0) {
        connectToWifi(ssid_arr[0], pwd_arr[0]);
        startConnectTime = millis();
        ++wifi_i;
    } else {
        connectToLora();
        startConnectTime = millis();
    }
}

void LoRaSENSE::loop() {

    // Network Connection
    if (connectingToWifi) {
        if ((millis() - lastWifiAttempt) < wifiTimeout && WiFi.status() == WL_CONNECTED) {
            // Connected to Wi-Fi; Node is root
            connectTime = millis() - startConnectTime;
            Serial.println("Connected to Wi-Fi");
            wifi_i = 0;
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
            // Failed to connect to Wi-Fi; Connect to another Wi-Fi as specified in ssid_arr and pwd_arr
            connectToWifi(ssid_arr[wifi_i], pwd_arr[wifi_i]);
            ++wifi_i;
        } else if ((millis() - lastWifiAttempt) >= wifiTimeout && !wifi_only) {
            // Failed to connect to all specified Wi-Fi; Connect to LoRa mesh
            Serial.println("No valid Wi-Fi router in range.");
            connectingToWifi = false;
            wifi_i = 0;
            WiFi.mode(WIFI_OFF);
            connectToLora();
        } else if ((millis() - lastWifiAttempt) >= wifiTimeout) {
            // Unable to connect to all specified Wi-Fi; Try again
            Serial.println("Unable to connect to Wi-Fi. Reconnecting...");
            connectingToWifi = true;
            wifi_i = 0;
            connectToWifi(ssid_arr[0], pwd_arr[0]);
        }
    }
    else if (connectingToLora) {
        if ((millis() - lastRreqSent) > this->rreqTimeout) {
            // Resend RREQ
            Serial.println("No RREP received.");
            Serial.print("Re-");
            connectToLora();
        }
    } 
    
    // Reconnect to Wi-Fi if not connected
    if (!connected && ((millis() - lastWifiAttempt) > this->wifiTimeout) && min_hop == 0) {
        // Reconnect to Wi-Fi
        connectToWifi(ssid_arr[0], pwd_arr[0]);
        ++wifi_i;
    }

    // Process packets in received packets queue
    if (!receivedPackets.isEmpty()) {
        Packet* packet = receivedPackets.peekFront();
        if (packet->checkCRC()) {
            Serial.printf("Processing %s packet %u...\n", 
                packet->getTypeInString(), packet->getPacketId());
            if (packet->getType() == RREQ_TYP && connected) {
                processRreq(packet);
            } else if (packet->getType() == RERR_TYP && packet->getSenderId() == this->getParentId() && connected) {
                processRerr(packet);
            } else if (packet->getType() == RREP_TYP && packet->getReceiverId() == this->getId()) {
                processRrep(packet);
            } else if (packet->getType() == RSTA_TYP && packet->getReceiverId() == this->getId()) {
                processRsta(packet);
            } else if (packet->getType() == DATA_TYP && packet->getReceiverId() == this->getId()) {
                processData(packet);
            } else if (packet->getType() == DACK_TYP && packet->getReceiverId() == this->getId()) {
                processDack(packet);
            } else if (packet->getType() == NETR_TYP && packet->getReceiverId() == this->getId()) {
                processNetr(packet);
            }
        } else {
            Serial.println("Received erroneous packet");
        }
        // Packet MUST be deleted to free memory resources!!
        receivedPackets.popFront();
        delete packet;
    }

    // Check if node is currently waiting for a DACK
    if (waitingForAck) { 
        Packet* packet = packetQueue.peekFront();
        if (millis() > lastSendAttempt + dackTimeout) {
            // ACK timeout
            if (!resent) {
                // Resend data packet
                resent = true;
                Serial.printf("No ACK received for %u. Resending...", packet->getPacketId());
                sendPacketViaLora(packet, true);
            } else {
                // TODO: Network reconstruction
                Serial.printf("Total data sending failure for packet %u.\n", packet->getPacketId());
                reconnect();
            }
        }
    } else {
        // Send packet in queue when NOT waiting for DACK
        sendPacketInQueue();
    }
}

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
    delay(esp_random() % rreqTimeout);  // delay to avoid packet collisions with other reconnecting nodes
    
    // Not all of these are needed to be reset, but I did just to be safe
    parent_id = 0;
    hopCount = INT_MAX;
    connectingToWifi = false;
    connectingToLora = false;
    connected = false;
    waitingForAck = false;
    resent = false;
    startConnectTime = 0;
    connectTime = 0;
    lastRreqSent = 0;
    lastWifiAttempt = 0;
    // nextSendAttempt = 0;
    lastSendAttempt = 0;

    funcOnConnecting();
    if (min_hop == 0) {
        connectToWifi(ssid_arr[0], pwd_arr[0]);
        startConnectTime = millis();
        ++wifi_i;
    } else {
        connectToLora();
        startConnectTime = millis();
    }
}

void LoRaSENSE::onLoraReceive(int packetSize) {
    if (!connectingToWifi) {
        if (packetSize) {
            int rssi = LoRa.packetRssi();
            byte* packetBuf = new byte[packetSize];
            for (int i = 0; LoRa.available(); ++i) {
                packetBuf[i] = LoRa.read();
            }
            Packet* packet = new Packet(packetBuf, packetSize);
            packet->setRssi(rssi);
            Serial.printf("%s packet %u received from %s (source: %s, RSSI: %i)...\n", 
                packet->getTypeInString(), packet->getPacketId(), String(packet->getSenderId(), HEX), String(packet->getSourceId(), HEX), rssi);
            receivedPackets.push(packet);
            delete packetBuf;
        }
    }
}

bool LoRaSENSE::sendPacketInQueue() {
    if (!packetQueue.isEmpty()) {
        // Packet* packet = packetQueue.peekFront();
        Packet* packet;
        int i = 0;
        for (; i < packetQueue.getSize(); ++i) {
            // Serial.println(i);
            packet = packetQueue.peek(i);
            if (packet->getSendTime() < millis()) {
                break;
            }
        }
        // Serial.printf("AFTER %i\n", i);
        if (i == packetQueue.getSize()) {
            // Serial.println("No packets ready to be sent");
            return false;
        } else if (i > 0) {
            packetQueue.pop(i);
            packetQueue.pushFront(packet);
        } 
        if (/*millis() >= nextSendAttempt && */
                (
                    connectingToLora && packet->getType() == RREQ_TYP || 
                    (connected && (hopCount > 0 || (packet->getType() != DATA_TYP && packet->getType() != RSTA_TYP && packet->getType() != NETR_TYP)))
                )
            ) {
            // Send packet via LoRa
            bool waitForAck = false;
            if (packet->getType() != RREQ_TYP && packet->getType() != RERR_TYP) {
                if (packet->getType() != RREQ_TYP && packet->getType() != RERR_TYP && packet->getType() != RREP_TYP && packet->getType() != DACK_TYP) {
                    //DEBUG
                        Serial.printf("Receiver ID: %i\n", packet->getReceiverId());
                    //
                    packet->setReceiverId(this->parent_id); // always set packet receiverId to CURRENT parent ID in case of reconnection to different parent
                    //DEBUG
                        Serial.printf("New Receiver ID: %i\n", packet->getReceiverId());
                    //
                }  
                Serial.printf("[LORA] Sending %s packet %i to node %s...\n", packet->getTypeInString(), packet->getPacketId(), String(packet->getReceiverId(), HEX).c_str());
            } else {
                Serial.printf("[LORA] Broadcasting %s packet %i...\n", packet->getTypeInString(), packet->getPacketId());
            }
            if (packet->getType() == DATA_TYP || packet->getType() == RSTA_TYP || packet->getType() == NETR_TYP) {
                waitForAck = true;
            }
            sendPacketViaLora(packet, waitForAck);
        } else if (hopCount == 0 && (packet->getType() == DATA_TYP || packet->getType() == RSTA_TYP || packet->getType() == NETR_TYP)) {
            // Send packet via Wi-Fi/HTTP
            Serial.printf("[WIFI] Sending %s packet %i to server...\n", packet->getTypeInString(), packet->getPacketId());
            sendPacketToServer(packet);
        } else {
            return false;
        }
        return true;
    }
    return false;
}

void LoRaSENSE::pushPacketToQueue(Packet* packet) {
    Serial.printf("Pushing %s packet %i to queue...", packet->getTypeInString(), packet->getPacketId());
    this->packetQueue.push(packet);
    Serial.printf("DONE. %u packet/s currently in queue\n", this->packetQueue.getSize());
}

void LoRaSENSE::pushPacketToQueueFront(Packet* packet) {
    Serial.printf("Pushing %s packet %i to front of queue...", packet->getTypeInString(), packet->getPacketId());
    this->packetQueue.pushFront(packet);
    Serial.printf("DONE. %i packet/s currently in queue\n", this->packetQueue.getSize());
}

Packet* LoRaSENSE::peekPacketQueue() {
    return this->packetQueue.peekFront();
}

bool LoRaSENSE::packetQueueIsEmpty() {
    return packetQueue.isEmpty();
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

void LoRaSENSE::setOnSendSuccess(void funcOnSendSuccess()) {
    this->funcOnSendSuccess = std::bind(funcOnSendSuccess);
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
    if (hopCount == INT_MAX) return -1;
    else return hopCount;
}

bool LoRaSENSE::isConnected() {
    return connected;
}

unsigned long LoRaSENSE::getConnectTime() {
    return connectTime;
}