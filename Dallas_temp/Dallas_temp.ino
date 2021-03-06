#include <CyberLib.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include "printf.h"

#define CSNPIN 10
#define CEPIN 9

struct sendtemp {
  float outtemp;
  float intemp;
  float pres;
  float hum;
};

// Радио
RF24 radio(CEPIN, CSNPIN);

// Пайпы
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

// Температура за бортом DS18B20
OneWire dswire(2);
DallasTemperature sensors(&dswire);
DeviceAddress dsaddr;

// Давление и температура на борту BMP180
Adafruit_BMP085        bmp;

// Влажность DHT22
DHT                    dht(3, 22);

void setup() {
  Serial.begin(9600);
  printf_begin();
  Serial.println("Begin setup");
  bmp.begin();
  dht.begin();
  sensors.getAddress(dsaddr, 0);
  sensors.setResolution(dsaddr, 12);
  radio.begin();
  // Настройка
  radio.setRetries(15, 15);
  radio.setAutoAck(1);
  radio.setChannel(50);
  radio.setDataRate(RF24_250KBPS);
  // Пайпы на запись и чтение
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1, pipes[1]);
  radio.startListening();
  radio.printDetails();
  Serial.println("Setup finished!");
}

void loop() {
  sendtemp st;
  float tempc;
  sensors.requestTemperatures();
  tempc = sensors.getTempC(dsaddr);
  st.outtemp = tempc;
  Serial.print("Outtemp: ");
  Serial.print(tempc);
  tempc = bmp.readTemperature();
  st.intemp = tempc;
  Serial.print(" Intemp: ");
  Serial.println(tempc);
  tempc = bmp.readPressure() / 133.32;
  st.pres = tempc;
  Serial.print(" Inpres: ");
  Serial.println(tempc);
  tempc = dht.readHumidity();
  st.hum = tempc;
  Serial.print(" Hum: ");
  Serial.println(tempc);
  radio.stopListening();
  bool ok = radio.write(&st, sizeof(sendtemp));
  if (ok)
    Serial.println("Sended!");
  else
    Serial.println("ERROR: not sended!");
  radio.startListening();
  delay_ms(2000);
}

