#include <Arduino.h>

//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

//Libraries for Wi-Fi
#include <WiFi.h>

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Color definitions
// #define BLACK 0x0000
// #define BLUE 0x001F
// #define RED 0xF800
// #define GREEN 0x07E0
// #define CYAN 0x07FF
// #define MAGENTA 0xF81F
// #define YELLOW 0xFFE0
// #define WHITE 0xFFFF

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

//OLED pins
#define OLED_SDA 21
#define OLED_SCL 22 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

//Wi-Fi credentials
#define WIFI_SSID "mirasbahay"
#define WIFI_PWD "carlopiadredcels"

//Constants
#define WIFI_TIMEOUT 5000

//Screen
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

//State variables
bool connected = false;
long start_connect;

void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);

  //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("LoRaSENSE Node");
  display.display();

  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);
  
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("Initialization OK!");
  display.setCursor(0,10);
  display.print("Initialization OK!");
  display.display();
  delay(3000);

  //Setup Wi-Fi
  WiFi.mode(WIFI_STA);
  
  // start_connect = millis();
}

void loop() {
  if (!connected) {
    //Connect to Wi-Fi first
    Serial.printf("Connecting to Wi-Fi router \"%s\"", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PWD);
    for (int time = 0; WiFi.status() != WL_CONNECTED && time < WIFI_TIMEOUT; time += 500) {
      delay(500);
      Serial.print(".");
    }
    if (WiFi.status() != WL_CONNECTED) {
      //Connect to LoRa mesh
      
    } else {
      //Node is root
      connected = true;
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.print("ROOT");
      display.display();
      Serial.println("connected!");
    }
  }
}