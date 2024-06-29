#include<ModbusMaster.h>
#include<SoftwareSerial.h>
#include <LoRa.h>
#include <SPI.h>

#define RXD0 3
#define TXD0 1

#define ss 5
#define rst 14
#define dio0 2

ModbusMaster node;

String Pubdata;



struct Weather{
    float humid;
    float wind_speed;
    float temp;
    float pressure;
    float rain;
    int wind_dir;
    float gust;
    float intensity;
  }weather;

float convert32(String value){
  // Convert the hex string to a 32-bit integer
  uint32_t intValue = (uint32_t) strtoul(value.c_str(), NULL, 16);

  // Interpret the integer as a float
  float floatValue;
  memcpy(&floatValue, &intValue, sizeof(floatValue));

  return floatValue;
}

void setup() {

  Serial.begin(9600, SERIAL_8E1, RXD0, TXD0);
  node.begin(1, Serial);

  LoRa.setPins(ss, rst, dio0); 
  while (!LoRa.begin(433E6))     //433E6 - Asia, 866E6 - Europe, 915E6 - North America
  {
    Serial.println(".");
    delay(500);
  }
  LoRa.setSyncWord(0xA5);

}

void loop() {

  uint8_t result;
  
  result = node.readHoldingRegisters(0,96);
  
  if (result == node.ku8MBSuccess)
  {
      String data1 = String(node.getResponseBuffer(4),HEX);
      String data2 = String(node.getResponseBuffer(5),HEX);

      weather.temp = convert32(data2+data1);

      data1 = String(node.getResponseBuffer(6),HEX);
      data2 = String(node.getResponseBuffer(7),HEX);

      weather.humid = convert32(data2+data1);

      data1 = String(node.getResponseBuffer(2),HEX);
      data2 = String(node.getResponseBuffer(3),HEX);

      weather.wind_speed = convert32(data2+data1);

      data1 = String(node.getResponseBuffer(8),HEX);
      data2 = String(node.getResponseBuffer(9),HEX);

      weather.pressure = convert32(data2+data1);

      weather.rain = node.getResponseBuffer(11);

      weather.wind_dir = node.getResponseBuffer(1);

      data1 = String(node.getResponseBuffer(73),HEX);
      data2 = String(node.getResponseBuffer(74),HEX);

      weather.gust = convert32(data2+data1);

      data1 = String(node.getResponseBuffer(12),HEX);
      data2 = String(node.getResponseBuffer(13),HEX);

      weather.intensity = convert32(data2+data1);

      Pubdata = "#AWS#"+String(weather.temp)+"#"+String(weather.humid)+"#"+String(weather.wind_speed)+"#"+String(weather.pressure)+"#"+String(weather.rain)+"#"+String(weather.intensity )+"#"+String(weather.wind_dir)+"#"+String(weather.gust);

      LoRa.beginPacket(); 
      LoRa.print(Pubdata);
      LoRa.endPacket();
      
      delay(10000);
  }

  // Serial.print("\n");
  delay(1000);
  
}