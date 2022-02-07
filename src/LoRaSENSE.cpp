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

LoRaSENSE::LoRaSENSE(int id, char** ssid_arr, char** pwd_arr, int wifi_arr_len, long timeout) {
    this->id = id;
    this->ssid_arr = ssid_arr;
    this->pwd_arr = pwd_arr;
    this->wifi_arr_len = wifi_arr_len;
    this->timeout = timeout;
}

LoRaSENSE::~LoRaSENSE() {

}

Packet LoRaSENSE::constructRreqPacket() {
    Packet packet;
    byte raw_header[10];
    byte header[12];
    // byte header_a[2];
    // byte header_b[4];
    int packet_id = rand();

    raw_header[0] = RREQ_TYP << 5;
    raw_header[1] = 0;
    raw_header[2] = (packet_id >> 24) & 0xFF;
    raw_header[3] = (packet_id >> 16) & 0xFF;
    raw_header[4] = (packet_id >> 8) & 0xFF;
    raw_header[5] = packet_id & 0xFF;
    raw_header[6] = (id >> 24) & 0xFF;
    raw_header[7] = (id >> 16) & 0xFF;
    raw_header[8] = (id >> 8) & 0xFF;
    raw_header[9] = id & 0xFF;
    header[0] = RREQ_TYP << 5;
    header[1] = 0;
    header[4] = (packet_id >> 24) & 0xFF;
    header[5] = (packet_id >> 16) & 0xFF;
    header[6] = (packet_id >> 8) & 0xFF;
    header[7] = packet_id & 0xFF;
    header[8] = (id >> 24) & 0xFF;
    header[9] = (id >> 16) & 0xFF;
    header[10] = (id >> 8) & 0xFF;
    header[11] = id & 0xFF;

    uint16_t crc = crc16_CCITT(raw_header, 6);
    header[2] = (crc >> 8) & 0xFF;
    header[3] = crc & 0xFF;
    packet.payload = header;
    packet.len = 12;

    return packet;
}

// Packet LoRaSENSE::constructRreqPacket() {
//     Packet packet;
//     String header_a;
//     String header_b;
//     String raw_header;
//     String header;
//     int packet_id = rand();
    
//     /*
//         header_a = TYP + 13(0) + CHECKSUM
//         header_b = IDs
//         raw_header = header_a + header_b + body (for crc processing)
//     */
//     header_a.concat(RREQ_TYP);
//     // Serial.printf("%i ", header_a.getBytes()[0]);
//     header_a.concat(0);
//     header_b.concat(packet_id);
//     header_b.concat(id);
//     raw_header.concat(header_a);
//     raw_header.concat(header_b);
    
//     // process CRC
//     uint8_t* raw_header_uint8 = reinterpret_cast<uint8_t*>(&raw_header[0]);
//     uint16_t crc = crc16_CCITT(raw_header_uint8, raw_header.length());
//     header_a.concat(crc);
//     header.concat(header_a);
//     header.concat(header_b);

//     byte header_bytes[header.length()];
//     header.getBytes(header_bytes, header.length());
//     for (int i = 0; i < header.length(); ++i) {
//         Serial.printf("%i ", header_bytes[i]);
//     }
//     Serial.println("  TEST");

//     return packet;
// }

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
        Serial.println("No valid Wi-Fi router in range. Connecting to LoRa mesh");
        WiFi.mode(WIFI_OFF);
        Packet rreq = constructRreqPacket();
        // for (int i = 0; i < rreq.len; ++i) {
        //     Serial.printf("%u ", rreq.payload[i]);
        //     if ((i % 4) == 3) {
        //         Serial.printf("\n");
        //     }
        // }
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