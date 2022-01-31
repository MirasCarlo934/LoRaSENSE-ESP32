#include <Arduino.h>
#include "LoRaSENSE.h"

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//OLED pins
#define OLED_SDA 21
#define OLED_SCL 22 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

//Constants
#define WIFI_TIMEOUT 5000

//Screen
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

//Wi-Fi credentials
const int wifi_arr_len = 1;
char *ssid_arr[wifi_arr_len] = {"mirasbahay"};
char *pwd_arr[wifi_arr_len] = {"carlopiadredcels"};

class LoRaSENSE LoRaSENSE(ssid_arr, pwd_arr, wifi_arr_len, WIFI_TIMEOUT);

void onConnect() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print(LoRaSENSE.getHopCount());
  display.display();
}

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

  LoRaSENSE.setOnConnect(&onConnect);
  LoRaSENSE.setup();
  Serial.println("Initialization OK!");
  display.setCursor(0,10);
  display.print("Initialization OK!");
  display.display();
  delay(3000);
}

void loop() {
  LoRaSENSE.loop();
}