#include <LoRa.h>

#include <SPI.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <LiquidCrystal_I2C.h>

#define ss 5
#define rst 14
#define dio0 2
#define RXD2 16
#define TXD2 17
#define NPKa 33

HardwareSerial neogps(1);
TinyGPSPlus gps;

int nilaiMo;
int nilaipH;
int nilaiN;
int nilaiP;
int nilaiK;

int Nx, Px, Kx, an1, an2, an3;
String dataSend = "";
String Ns, Ps, Ks, pHs, Moists, lng, lat;

int lcdColumns = 16;
int lcdRows = 2;
String lcdmsg1 = "";
String lcdmsg2 = "";

String lora_message = "";

LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

void scrollText(int row, String message, int delayTime, int lcdColumns) {
  for (int i = 0; i < lcdColumns; i++) {
    message = " " + message;
  }
  message = message + " ";
  for (int pos = 0; pos < message.length(); pos++) {
    lcd.setCursor(0, row);
    lcd.print(message.substring(pos, pos + lcdColumns));
    delay(delayTime);
  }
}

void sensorMoist() {
  int hasilPembacaan = analogRead(13);
  Serial.print("Raw Moisture Reading: ");
  Serial.println(hasilPembacaan);
  int anxm = map(hasilPembacaan, 0, 4095, 0, 1023);
  nilaiMo = map(anxm, 0, 1023, 100, 20);
  Serial.print("Persentase Kelembaban Tanah = ");
  Serial.print(nilaiMo);
  Serial.println("%");
  delay(1000);
}

void sensorpH() {
  float anpH = analogRead(26);
  int anxpH = map(anpH, 0, 4095, 0, 1023);
  float anpH1 = map(anxpH, 0, 950, 10, 35);
  nilaipH = (-0.0693 * anpH1) + 7.3855;
  if (nilaipH < 1) {
    nilaipH = 1;
  } else if (nilaipH > 14) {
    nilaipH = 14;
  }
  Serial.print("Nilai pH = ");
  Serial.println(nilaipH);
  delay(1000);
}

void maptegangan() {
  an1 = analogRead(NPKa);
  int anx1 = map(an1, 0, 4095, 0, 1023);
  Nx = map(anx1, 0, 1023, 412, 60);
  if (Nx < 0) {
    Nx = 0;
  } else if (Nx > 900) {
    Nx = 900;
  }
  String message11 = "N : " + (String)Nx + " PPM  ";
  String message1 = "    SOIL NPK     ";

  an2 = analogRead(NPKa);
  int anx2 = map(an2, 0, 4095, 0, 1023);
  Px = map(anx2, 100, 1023, 39, 5);
  if (Px < 0) {
    Px = 0;
  } else if (Px > 100) {
    Px = 100;
  }
  String message21 = "P : " + (String)Px + " PPM ";
  String message2 = "    SOIL NPK     ";

  an3 = analogRead(NPKa);
  int anx3 = map(an3, 0, 4095, 0, 1023);
  Kx = map(anx3, 30, 1023, 270, 60);
  if (Kx < 0) {
    Kx = 0;
  } else if (Kx > 700) {
    Kx = 700;
  }

  Serial.print("N = ");
  Serial.println(Nx);
  Serial.print("P = ");
  Serial.println(Px);
  Serial.print("K = ");
  Serial.println(Kx);
}

void print_GPS() {
  if (gps.location.isValid()) {
    lat = String(gps.location.lat(), 8);
    Serial.print("Lat: ");
    Serial.println(lat);

    lng = String(gps.location.lng(), 8);
    Serial.print("Lng: ");
    Serial.println(lng);
  } else {
    Serial.println("No Data");
  }
}

void setup() {
  Serial.begin(115200);

  // Begin serial communication Neo7mGPS
  neogps.begin(9600, SERIAL_8N1, RXD2, TXD2);

  while (!Serial)
    ;
  Serial.println("LoRa Sender");

  LoRa.setPins(ss, rst, dio0);  // setup LoRa transceiver module

  int loraRetries = 0;
  while (!LoRa.begin(433E6) && loraRetries < 10) {
    Serial.println(".");
    delay(500);
    loraRetries++;
  }
  if (loraRetries >= 10) {
    Serial.println("LoRa Initialization Failed!");
  } else {
    Serial.println("LoRa Initializing OK!");
  }

  LoRa.setSyncWord(0xA5);

  lcd.init();
  lcd.backlight();
  delay(5000);
}

void loop() {
  int anxx = analogRead(NPKa);
  int anxxx = map(anxx, 0, 4095, 0, 1023);

  if (anxxx < 1010) {
    sensorMoist();
    sensorpH();
    maptegangan();

    lcdmsg1 = "N:" + String(Nx) + " P:" + String(Px) + " K:" + String(Kx);
    lcdmsg2 = "PH:" + String(nilaipH) + " Moist:" + String(nilaiMo) + "%";

    // scrollText(0, lcdmsg1, 300, 16);
    // delay(1000); // Tambahkan delay untuk memberi waktu LCD bernafas
    lcd.setCursor(0, 0);
    lcd.print(lcdmsg1);

    lcd.setCursor(0, 1);
    lcd.print(lcdmsg2);

    boolean newData = false;
    for (unsigned long start = millis(); millis() - start < 1000;) {
      while (neogps.available()) {
        if (gps.encode(neogps.read())) {
          newData = true;
          break;  // Tambahkan break agar tidak stuck
        }
      }
      if (newData) break;  // Tambahkan break agar tidak stuck
    }

    if (newData) {
      Serial.println(gps.satellites.value());
      print_GPS();
    } else {
      Serial.println("No Data");
    }

    lora_message = "#Sensor_Soil_004#" + lng + "#" + lat + "#" + Nx + "#" + Px + "#" + Kx + "#" + nilaipH + "#" + String(nilaiMo);
    Serial.print("Sending packet: ");
    Serial.println(lora_message);

    LoRa.beginPacket();  // Send LoRa packet to receiver
    LoRa.print(lora_message);
    LoRa.endPacket();

    delay(10000);
  }
  else if(anxxx > 1015){
    lcdmsg1 = "N:" + String(0) + " P:" + String(0) + " K:" + String(0);
    lcdmsg2 = "PH:" + String(0) + " Moist:" + String(0) + "%";
    lcd.setCursor(0, 0);
    lcd.print(lcdmsg1);

    lcd.setCursor(0, 1);
    lcd.print(lcdmsg2);
  }
}
