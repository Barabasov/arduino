#include <SoftwareSerial.h>

#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

// Init LCD
#define LCD_I2C_ADDR  0x27
#define BACKLIGHT     3
#define LCD_EN        2
#define LCD_RW        1
#define LCD_RS        0
#define LCD_D4        4
#define LCD_D5        5
#define LCD_D6        6
#define LCD_D7        7

// Объект для работы с дисплеем
LiquidCrystal_I2C lcd(LCD_I2C_ADDR,
                      LCD_EN,
                      LCD_RW,
                      LCD_RS,
                      LCD_D4,
                      LCD_D5,
                      LCD_D6,
                      LCD_D7);

// Программный Serial
SoftwareSerial swSerial(10, 11); // RX - 10, TX - 11

// Инициализая
void setup() {
  // ЖК дисплей
  lcd.begin(20,4);
  lcd.setBacklightPin(BACKLIGHT,POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.clear();
//  lcd.home();
  // Шаблон показаний
  lcd.setCursor(0,0);
  lcd.print("T1=_____C T2=_____C");
  lcd.setCursor(0,1);
  lcd.print("T3=_____C T4=_____C");
  // Последовательный порт
  Serial.begin(9600);
  swSerial.begin(9600);
}

void loop() {
  // Курсор на первый символ
  lcd.home();
  // Переменные
  int i; // Счетчик
  // Serial
  String RecvStr="";
  String Eps="";
  String Tmp="";
  // swSerial
  String swRecvStr="";
  String swEps="";
  String swTmp="";
  // Чтение из порта
  if (Serial.available() > 0) {
    char RecvChar = Serial.read();
    // Отладочный вывод
    Serial.println(Serial.available());
    Serial.println(RecvChar);
    // Проверка на начало последовательности
    if (RecvChar == '=') {
      // Вся ли строка в буфере?
      if (Serial.available() >= 14) {
        // Если вся, то читаем всю.
        for (i=0;i<14;i++) {
          RecvStr += char(Serial.read());
        }
        // Выводим последовательность
        RecvStr.trim();
        // Выделяем и выводим e
        Eps=RecvStr.substring(5,7);
        // Выделяем и выводим тепературу
        Tmp=RecvStr.substring(7,11);
        PrintResult(Tmp, 0, 3);
      }
    }
  }
  // Чтение из swSeial
  if (swSerial.available() > 0) {
    char RecvChar = swSerial.read();
    // Отладочный вывод
    swSerial.println(swSerial.available());
    swSerial.println(RecvChar);
    // Проверка на начало последовательности
    if (RecvChar == '=') {
      // Вся ли строка в буфере?
      if (swSerial.available() >= 14) {
        // Если вся, то читаем всю.
        for (i=0;i<14;i++) {
          swRecvStr += char(swSerial.read());
        }
        // Выводим последовательность
        RecvStr.trim();
        // Выделяем и выводим e
        swEps=swRecvStr.substring(5,7);
        // Выделяем и выводим тепературу
        swTmp=swRecvStr.substring(7,11);
        PrintResult(swTmp, 0, 13);        
      }
    }
  }
  // Задержка между замерами
  delay(100);
}

// Вывод на экран
void PrintResult(String Input, int lcdLine, int lcdPos){
  float DigTmp;
  char  TmpOut[6]="";
  swSerial.println(Input);
  swSerial.println(sizeof(Input));
  switch (Input[0]){
    case '-':
    // Отрицательные значения
      if (Input[1]=='F'){
        Input[1]='0';
      }
      DigTmp=float(Input.toInt())/10;
      swSerial.println(Input);
      dtostrf(DigTmp, 5, 1, TmpOut);
      swSerial.println(TmpOut);
      lcd.setCursor(lcdPos,lcdLine);
      lcd.print(TmpOut);
      break;
    case '>':
    // Превышение диапазона
      lcd.setCursor(lcdPos,lcdLine);
      lcd.print(">>>>>");
      break;
    case '<':
    // Ниже диапазона
      lcd.setCursor(lcdPos,lcdLine);
      lcd.print("<<<<<");
      break;
    default:
    // Положительные в диапазоне
      lcd.setCursor(lcdPos,lcdLine);
      lcd.print(" ");
      lcd.print(Input);
  }
}
