// Arduini Mega as CLIENT

#include <CyberLib.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "printf.h"
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <SdFat.h>
#include <XModem.h>

// Порты LCD
#define LCD1_I2C_ADDR    0x27
#define LCD2_I2C_ADDR    0x23
#define BACKLIGHT     3
#define LCD_EN  2
#define LCD_RW  1
#define LCD_RS  0
#define LCD_D4  4
#define LCD_D5  5
#define LCD_D6  6
#define LCD_D7  7

LiquidCrystal_I2C      lcd1(LCD1_I2C_ADDR, LCD_EN, LCD_RW, LCD_RS, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
LiquidCrystal_I2C      lcd2(LCD2_I2C_ADDR, LCD_EN, LCD_RW, LCD_RS, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// Пины радио
#if  defined (__AVR_ATmega2560__) || defined (__AVR_ATmega2561__)
#define CSNPIN 49
#define RF_OFF D49_High
#define RF_ON D49_Low
#define CEPIN 48
#else
#define CSNPIN 8
#define RF_OFF D8_High
#define RF_ON D8_Low
#define CEPIN 9
#endif


struct sendtemp {
  uint32_t dt;       // Время unixtime
  float    outtemp;  // Температура на улице
  float    intemp;   // Температура внутри
  float    pres;     // Давление
  float    humin;    // Влажность относительная
  float    humout;   // Влажность относительная
};


// Карта памяти

SdFat SD;

//File logfile;        // Файл лога
bool isSD = false;     // Флаг присутствия карты

#define SDCS 4      // CS SD-карты


// Радио
RF24 radio(CEPIN, CSNPIN);

// Пайпы
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

void setup() {
  Serial.begin(115200);
  // Настройка выводов
  D4_Out;
  D10_Out;
#if  defined (__AVR_ATmega2560__) || defined (__AVR_ATmega2561__)
  D49_Out;
  D53_Out;
#else
  D8_Out;
#endif
  // Отключить Ethernet
  D10_High;
  // Включаем дисплей
  lcd1.begin(20, 4);
  lcd1.setBacklightPin(BACKLIGHT, POSITIVE);
  lcd1.setBacklight(HIGH);
  lcd1.clear();
  lcd1.home();
  lcd1.setCursor(0, 1);
  lcd1.print(F("T('C):"));
  lcd1.setCursor(0, 2);
  lcd1.print(F("H (%):"));
  lcd1.setCursor(0, 3);
  lcd1.print(F("P(Hg):"));
  lcd2.begin(20, 4);
  lcd2.setBacklightPin(BACKLIGHT, POSITIVE);
  lcd2.setBacklight(HIGH);
  lcd2.clear();
  lcd2.home();
  lcd2.setCursor(0, 0);
  lcd2.print(F("SD:--"));
  lcd2.setCursor(10, 0);
  lcd2.print(F("Wr:--"));
  lcd2.setCursor(0, 1);
  lcd2.print(F("Log:------------"));
  printf_begin();
  // Включаем карту
  CheckSD();
  // Включаем радио
  radio.begin();
  // Настройка
  radio.setRetries(15, 15);
  radio.setAutoAck(1);
  radio.setChannel(50);
  radio.setDataRate(RF24_250KBPS);
  // Пайпы на запись и чтение
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1, pipes[0]);
  radio.startListening();
  radio.printDetails();
  Serial.println(F("This is CLIENT"));
}

void loop() {
  // Работает только RF24
  if ( radio.available() )
  {
    bool done = false;
    sendtemp st;
    while (!done)
    {
      done = radio.read( &st, sizeof(sendtemp) );
      DateTime MSK (st.dt);
      char dt[21] = "";
      sprintf(dt, "%04d/%02d/%02d  %02d:%02d:%02d", MSK.year(),
              MSK.month(),
              MSK.day(),
              MSK.hour(),
              MSK.minute(),
              MSK.second());
      // Вывод в порт
      Serial.println(F("Recived!"));
      Serial.print(F("Time: "));
      Serial.println(dt);
      Serial.print(F("Outtemp: "));
      Serial.println(st.outtemp);
      Serial.print(F("Intemp: "));
      Serial.println(st.intemp);
      Serial.print(F("Inpres: "));
      Serial.println(st.pres);
      Serial.print(F("Inhum: "));
      Serial.println(st.humin);
      Serial.print(F("Outhum: "));
      Serial.println(st.humout);
      // Вывод на дисплей
      lcd1.setCursor(0, 0);
      lcd1.print(dt);
      // Под температуру
      char tmpc[7] = "";
      // Под всё остальное
      char prnt[15] = "";
      // Вывод температуры внутри и снаружи.
      lcd1.setCursor(7, 1);
      if (st.intemp == -127.0) {
        lcd1.print(F("ERROR!"));
      } else {
        dtostrf(st.intemp, 6, 2, tmpc);
        lcd1.print(tmpc);
      }
      lcd1.print("/");
      if (st.outtemp == -127.0) {
        lcd1.print(F("ERROR!"));
      } else {
        dtostrf(st.outtemp, 6, 2, tmpc);
        lcd1.print(tmpc);
      }
      // Вывод относительной влажности
      //lcd1.setCursor(6, 2);
      //dtostrf(st.hum, 14, 2, prnt);
      //lcd1.print(prnt);
      lcd1.setCursor(7, 2);
      dtostrf(st.humin, 6, 2, tmpc);
      lcd1.print(tmpc);
      lcd1.print("/");
      dtostrf(st.humout, 6, 2, tmpc);
      lcd1.print(tmpc);
      // Вывод давления
      lcd1.setCursor(6, 3);
      dtostrf(st.pres, 14, 2, prnt);
      lcd1.print(prnt);
      // Запись на карту памяти
      if (isSD == true) {
        Serial.print(F("Writing..."));
        // Выключить RF24
        RF_OFF;
        // Включить SD-карту
        D4_Low;
        // Файл для записи
        char fn[13] = "";
        sprintf(fn, "%04d%02d%02d.txt", MSK.year(),
                MSK.month(),
                MSK.day());
        lcd2.setCursor(4, 1);
        lcd2.print(fn);
        File logfile = SD.open(fn, FILE_WRITE);
        if (logfile == true) {
          // Валидация полученных данных
          if (!(st.intemp == 85.0) & !(st.intemp == -127) & !(st.outtemp == 85.0) & !(st.outtemp == -127) & !(isnan(st.humout))) {
            logfile.print(dt);
            logfile.print(" ");
            logfile.print(st.outtemp);
            logfile.print(" ");
            logfile.print(st.intemp);
            logfile.print(" ");
            logfile.print(st.pres);
            logfile.print(" ");
            logfile.print(st.humin);
            logfile.print(" ");
            logfile.println(st.humout);
            logfile.close();
            // print to the serial port too:
            Serial.println(F("OK!"));
            lcd2.setCursor(13, 0);
            lcd2.print(F("OK "));
          } else {
            Serial.println(F("Bad data!"));
            lcd2.setCursor(13, 0);
            lcd2.print(F("Bad"));
            logfile.close();
          };
        } else {
          // При ошибке записи запускаем отсчет
          Serial.println(F("ERR!"));
          lcd2.setCursor(13, 0);
          lcd2.print(F("ERR"));
          isSD = false;
        }
        // Выключить SD-карту
        D4_High;
        // Включить RF24
        RF_ON;
      }
    }
  };
  delay(20);
  CheckSD();
  if (isSD == true) {
    if (Serial.available() > 0) {
      char filename[13] = ""; // Буфер для имени
      byte ifn = 0;           // Счетчик
      // Выключаем радио для приёма
      D4_High;
      RF_ON;
      radio.stopListening();
      RF_OFF;
      D4_Low;
      // Читаем имя файла
      while (Serial.available() > 0) {
        filename[ifn] = Serial.read();
        // Если дошли до перевода строки
        if (filename[ifn] == 0x0A) {
          filename[ifn] = 0x00;
          Serial.flush();
        }
        ifn++;
        // Дошли до максимальной длинны имени
        if (ifn > 12) {
          Serial.flush();
        }
      } // Чтение файла
      // Поиск файла на флешке
      Serial.print(filename);
      if (SD.exists(filename)) {
        Serial.println(F(" found! Starting transfer..."));
        lcd2.setCursor(3, 0);
        lcd2.print("TRANS");
        File dataFile = SD.open(filename);
        XModem xmodem(&Serial, ModeYModem);
        xmodem.sendFile(dataFile, filename);
        dataFile.close();
        lcd2.setCursor(3, 0);
        lcd2.print(F("OK   "));
      } else {
        Serial.println(F(" not found! ERROR!"));
      }
      D4_High;
      RF_ON;
      radio.startListening();
      RF_OFF;
      D4_Low;
    }
  }
};

// Проверка карты памяти
void CheckSD() {
  // Попытки
  static byte tr = 0;
  unsigned long curt = millis();
  static unsigned long oldt = 0;
  // Позиция вывода
  lcd2.setCursor(3, 0);
  // Если ошибка карты
  if (isSD == false) {
    // Проверка интервала
    if (curt - oldt > 1000) {
      oldt = curt;
      // Если счетчик досчитал
      if (tr < 1) {
        lcd2.print(F("Init "));
        lcd2.setCursor(3, 0);
        // Выключить RF24
        RF_OFF;
        // Включить SD-карту
        D4_Low;
        if (!SD.begin(SDCS)) {
          // SD-карта не включилась. Ошибка.
          isSD = false;
          Serial.println(F("SD not found!"));
          lcd2.print(F("ERR  "));
          tr = 5;
        } else {
          // SD-карта на местое. Работаем.
          isSD = true;
          Serial.println(F("SD found!"));
          lcd2.print(F("OK   "));
        }
        // Выключить SD-карту
        D4_High;
        // Включить RF24
        RF_ON;
      } else {
        // Вывод ошибки и счетчика
        lcd2.print(F("ERR "));
        lcd2.print(tr);
        tr--;
      }
    }
  }
}

