#include <iarduino_RTC.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <DS3231.h> //это чисто чтобы показывать температуру
DS3231  rtc(SDA, SCL); //только для температуры
#define printByte(args) write(args); //это поможет нарисовать колокольчик
iarduino_RTC time(RTC_DS3231);
LiquidCrystal_I2C lcd(0x27, 20, 4);
//------------------------------------------------------------------------------
int buttonPinChangeTimeMinus = 3; //объявляем контакты кнопок
int buttonPinSetTime = 4;
int buttonPinChangeTimePlus = 5;
boolean statusButtonSetTime = LOW; //объявляем статусы кнопок
boolean statusButtonChangeTimePlus = LOW;
boolean statusButtonChangeTimeMinus = LOW;
int howManyclickButtonHours; //переменные установки времени и даты
int howManyclickButtonMin;
int howManyclickButtonDay;
int howManyclickButtonWeekDay;
int howManyclickButtonMonth;
int howManyclickButtonYear;
//------------переменные вывода на экран значений времени при настройке---------
int Hour = 0;
int Min = 0;
int Sec = 0;
int day1 = 1;
int mount = 1;
int year1 = 1;
//-----------------------------флаги-------------------------------------------
boolean flag = false; //флаг который участвует в фиксировании нажатии кнопки
boolean flagBacklight = false; //управление подсветкой
unsigned long timeFlagBackLight = 0; //время подсветки экрана
int flagAlarm = 0; //если 1 будильник включён, если 0 выключен
boolean tikTakFlag = true; //управление подсветкой во время работы будильника
//---------------------------переменные для будильника--------------
int alarmHour = 6;
int alarmMin = 0;
int alarmWeek1 = 0; //нижняя граница включения будильника по дням
int alarmWeek2 = 6; //верхняя граница включения будильника по дням
int alarmWeekClick = 0; //если 0 будильник включается каждый день(границы 0-6),
//если 1 - только по будням(границы 1-5),
//если 2 - по выходным (границы только 0 или 6)
int pilikPin = 7; //пин для запуска платы динамика
//----------------------символы колокольчика-----------------
uint8_t AlarmPic1[8] = {0x4, 0x4, 0xA, 0x11, 0x11, 0x1F, 0x8, 0x10};
uint8_t AlarmPic2[8] = {0x4, 0x4, 0xA, 0x11, 0x11, 0x1F, 0x2, 0x1};

void setup() {
  Serial.begin(9600);
  rtc.begin();
  lcd.init(); //инициализация платы экрана
  time.begin(); //инициализация платы времени
  howManyclickButtonHours = time.Hours; //присвоение переменным установки даты
  howManyclickButtonMin = time.minutes; //и времени из платы времени(удобство)
  howManyclickButtonDay = time.day;
  howManyclickButtonWeekDay = time.weekday;
  howManyclickButtonMonth = time.month;
  howManyclickButtonYear = time.year;
  pinMode(buttonPinSetTime, INPUT);
  pinMode(buttonPinChangeTimePlus, INPUT);
  pinMode(pilikPin, OUTPUT);
  lcd.createChar(1, AlarmPic1);
  lcd.createChar(2, AlarmPic2);
  lcd.setCursor(10, 0);
  lcd.setCursor(14, 0);
  flagAlarm = EEPROM.read(0); //считываем из энергонезависимой памяти значения будильника
  alarmHour = EEPROM.read(1);
  alarmMin = EEPROM.read(2);
  alarmWeek1 = EEPROM.read(3);
  alarmWeek2 = EEPROM.read(4);
  alarmWeekClick = EEPROM.read(5);
  satatusAlarm(); //пропишем на экран состояние будильника
}
/*------------------------------------------------------------------------------
  тут происходит отображение времени и управление подсветкой -------------------*/
//-----отображение времени (раз в секунду обращаемся к плате времени)--------
void loop() {
  if (millis() % 1000 == 0) {
    timePrint();
  }
  statusButtonSetTime = digitalRead(buttonPinSetTime); //опрашиваем кнопки
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  statusButtonChangeTimeMinus = digitalRead(buttonPinChangeTimeMinus);
  //---------------установка времени-----------------------------------------------
  if (statusButtonSetTime == HIGH) { //по кнопке переходим в функцию установки времени
    lcd.backlight(); //включаем подсветку
    setsTime();
  }
  //----------------управление подсветкой----------------------------------------
  if (statusButtonChangeTimePlus == HIGH && flagBacklight == false) {
    delay(300);
    statusButtonChangeTimePlus = LOW;
    lcd.backlight();
    timeFlagBackLight = millis();
    flagBacklight = true;
  }

  if (statusButtonChangeTimePlus == HIGH && flagBacklight == true) { //если нажмём, то светить будем до следующего
    delay(300);                                                //нажатия
    statusButtonChangeTimePlus = LOW;
    flagBacklight = false;
  }
  if (millis() - timeFlagBackLight > 10000 && flagBacklight == true) { //светим 10 секунд
    lcd.noBacklight();
    Serial.println(millis() - timeFlagBackLight);
    flagBacklight = false;
  }
  //---------------------------будильник------------------------------------------
  if (millis() % 1000 == 100) { //большая буква Н важно! это значит что часы выдаются в формате 0-23
    //а если h, то часы будут выдаваться в формате 0-12
    if (time.Hours == alarmHour && time.seconds == 00 && time.minutes == alarmMin  && flagAlarm == 1) { //проверяем будильник
      if (time.weekday >= alarmWeek1 && time.weekday <= alarmWeek2) { //срабатываем либо по будням, либо каждый день
        alarm();
      }
      if (alarmWeek1 == 10 && time.weekday == 0 || alarmWeek1 == 10 && time.weekday == 6) { //срабатываем только по выходным
        alarm();
      }
    }
  }
  //--------------------------Температура------------------------------------------------
/*  while (statusButtonChangeTimeMinus == HIGH) {
    temperature();
    statusButtonChangeTimeMinus = digitalRead(buttonPinChangeTimeMinus);
  }*/
}
void setsTime() {
  //-------------------------------Установка времени и будильника-----------------
  delay(300);
  lcd.clear();
  while (statusButtonSetTime == HIGH) { //специальный цикл, который помогает избежать случайного перескока дальше
    statusButtonSetTime = digitalRead(buttonPinSetTime); //пока не отпустили кнопу
    delay(100);
  }
  //------------------установка часов--------------------------------------
  statusButtonSetTime = LOW;
  howManyclickButtonHours = time.Hours; //присвоим действующее значение времени переменной настройки
  while (statusButtonSetTime == LOW) {  //Hours обязательно с большой буквы
    setHours();
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(100);
  }
  while (statusButtonSetTime == HIGH) {
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(100);
  }
  statusButtonSetTime = LOW;
  //--------------установка минут-------------------------------------------------
  howManyclickButtonMin = time.minutes;  //присвоим действующее значение времени переменной настройки
  while (statusButtonSetTime == LOW) {
    setMin();
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(100);
  }
  while (statusButtonSetTime == HIGH) {
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(100);
  }
  statusButtonSetTime = LOW;
  lcd.clear();
  //------------установка дня месяца-------------------------------------------------
  howManyclickButtonDay = time.day;
  while (statusButtonSetTime == LOW) {
    setDay();
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(100);
  }
  while (statusButtonSetTime == HIGH) {
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(100);
  }
  statusButtonSetTime = LOW;
  //---------установка дня недели-------------------------------------------------
  howManyclickButtonWeekDay = time.weekday;
  while (statusButtonSetTime == LOW) {
    setWeekDay();
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(100);
  }
  while (statusButtonSetTime == HIGH) {
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(100);
  }
  statusButtonSetTime = LOW;
  //----------установка месяца---------------------------------------------------
  howManyclickButtonMonth = time.month;
  while (statusButtonSetTime == LOW) {
    setMonth();
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(100);
  }
  while (statusButtonSetTime == HIGH) {
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(100);
  }
  statusButtonSetTime = LOW;
  //-----------установка года--------------------------------------------------------
  while (statusButtonSetTime == LOW) {
    setYear();
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(100);
  }
  while (statusButtonSetTime == HIGH) {
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(100);
  }
  statusButtonSetTime = LOW;
  lcd.clear();
  //-----------вкл/выкл будильника-------------------------------------------------
  while (statusButtonSetTime == LOW) {
    setAlarmOnOff();
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(100);
  }
  EEPROM.write(0, flagAlarm);
  while (statusButtonSetTime == HIGH) {
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(100);
  }
  statusButtonSetTime = LOW;
  //---------установка часа будильника----------------------------------------------
  while (statusButtonSetTime == LOW) {
    setAlarmHour();
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(100);
  }
  EEPROM.write(1, alarmHour);
  while (statusButtonSetTime == HIGH) {
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(100);
  }
  statusButtonSetTime = LOW;
  lcd.clear();
  //---------установка минут будильника-------------------------------------------
  while (statusButtonSetTime == LOW) {
    setAlarmMin();
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(100);
  }
  EEPROM.write(2, alarmMin);
  while (statusButtonSetTime == HIGH) {
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(100);
  }
  statusButtonSetTime = LOW;
  //--------установка в какие дни включать будильник--------------------------------
  while (statusButtonSetTime == LOW) {
    setAlarmWeek();
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(200);
  }
  EEPROM.write(3, alarmWeek1);
  EEPROM.write(4, alarmWeek2);
  EEPROM.write(5, alarmWeekClick);
  while (statusButtonSetTime == HIGH) {
    statusButtonSetTime = digitalRead(buttonPinSetTime);
    delay(100);
  }
  statusButtonSetTime = LOW;
  //----------перед выходом из установки почистим экран-------------------------------------
  //    выключим подсветку, выставим режим работы будильника
  lcd.noBacklight();
  satatusAlarm();
}
void timePrint() {
  //----------------функция отображения времени-------------------------------------
  lcd.setCursor(0, 0);
  lcd.print(time.gettime("H:i:s"));
  lcd.setCursor(0, 1);
  lcd.print(time.gettime("d M Y D"));
  delay(1);
  temperature();
  
  //--------мигаем колокольчиком-------
  if (tikTakFlag == true) {
    lcd.setCursor(9, 0);
    lcd.printByte(1);
    tikTakFlag = false;
  } else {
    lcd.setCursor(9, 0);
    lcd.printByte(2);
    tikTakFlag = true;
  }
}
void setHours() {
  //--------------счётчик нажатий------------------------------------------
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  statusButtonChangeTimeMinus = digitalRead(buttonPinChangeTimeMinus);
  if (statusButtonChangeTimePlus == HIGH && flag == false) {
    flag = true;
    howManyclickButtonHours = (howManyclickButtonHours + 1) % 24;
  }
  if (statusButtonChangeTimeMinus == HIGH && flag == false) {//тут сделано вычитание по
    flag = true;                                             //по другой кнопке
    howManyclickButtonHours = (howManyclickButtonHours - 1) % 24;
    if (howManyclickButtonHours == -1) { //переход через ноль
      howManyclickButtonHours = 23;
    }
  }
  delay(10);
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  statusButtonChangeTimeMinus = digitalRead(buttonPinChangeTimeMinus);
  if (statusButtonChangeTimePlus == LOW && statusButtonChangeTimeMinus == LOW) {
    flag = false;
  }
  //-------------присвоение часа плате времени----------------------------
  time.settime(-1, -1, howManyclickButtonHours);
  lcd.setCursor(0, 0);
  lcd.print("Hours:");
  lcd.print(" ");
  lcd.print(howManyclickButtonHours);
  lcd.print(" ");
}
void setMin() {
  //-----------------------------------------------------------------------------------
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  if (statusButtonChangeTimePlus == HIGH && flag == false) {
    flag = true;
    howManyclickButtonMin = (howManyclickButtonMin + 1) % 60;
  }
  if (statusButtonChangeTimeMinus == HIGH && flag == false) {
    flag = true;
    howManyclickButtonMin = (howManyclickButtonMin - 1) % 60;
    if (howManyclickButtonMin == -1) {
      howManyclickButtonMin = 59;
    }
  }
  delay(10);
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  statusButtonChangeTimeMinus = digitalRead(buttonPinChangeTimeMinus);
  if (statusButtonChangeTimePlus == LOW && statusButtonChangeTimeMinus == LOW) {
    flag = false;
  }
  //-----------------------------------------------------------------------------------
  time.settime(-1, howManyclickButtonMin);
  lcd.setCursor(0, 1);
  lcd.print("Minuts:");
  lcd.print(" ");
  lcd.print(howManyclickButtonMin);
  lcd.print(" ");
}
void setDay() {
  //-----------------------------------------------------------------------------------
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  if (statusButtonChangeTimePlus == HIGH && flag == false) {
    flag = true;
    howManyclickButtonDay = (howManyclickButtonDay + 1) % 32;
  }
  if (statusButtonChangeTimeMinus == HIGH && flag == false) {
    flag = true;
    howManyclickButtonDay = (howManyclickButtonDay - 1) % 32;
    if (howManyclickButtonDay == -1) {
      howManyclickButtonDay = 31;
    }
  }
  delay(10);
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  statusButtonChangeTimeMinus = digitalRead(buttonPinChangeTimeMinus);
  if (statusButtonChangeTimePlus == LOW && statusButtonChangeTimeMinus == LOW) {
    flag = false;
  }
  //-----------------------------------------------------------------------------------
  time.settime(-1, -1, -1, howManyclickButtonDay);
  lcd.setCursor(0, 0);
  lcd.print("Day:");
  lcd.print(" ");
  lcd.print(howManyclickButtonDay);
  lcd.print(" ");
}
void setWeekDay() {
  //-----------------------------------------------------------------------------------
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  if (statusButtonChangeTimePlus == HIGH && flag == false) {
    flag = true;
    howManyclickButtonWeekDay = (howManyclickButtonWeekDay + 1) % 7;
  }
  if (statusButtonChangeTimeMinus == HIGH && flag == false) {
    flag = true;
    howManyclickButtonWeekDay = (howManyclickButtonWeekDay - 1) % 7;
    if (howManyclickButtonWeekDay == -1) {
      howManyclickButtonWeekDay = 7;
    }
  }
  delay(10);
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  statusButtonChangeTimeMinus = digitalRead(buttonPinChangeTimeMinus);
  if (statusButtonChangeTimePlus == LOW && statusButtonChangeTimeMinus == LOW) {
    flag = false;
  }
  //-----------------------------------------------------------------------------------
  time.settime(-1, -1, -1, -1, -1, -1, howManyclickButtonWeekDay);
  lcd.setCursor(10, 0);
  lcd.print(time.gettime("D"));
}
void setMonth() {
  //-----------------------------------------------------------------------------------
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  if (statusButtonChangeTimePlus == HIGH && flag == false) {
    flag = true;
    howManyclickButtonMonth = (howManyclickButtonMonth + 1) % 13;
  }
  if (statusButtonChangeTimeMinus == HIGH && flag == false) {
    flag = true;
    howManyclickButtonMonth = (howManyclickButtonMonth - 1) % 13;
    if (howManyclickButtonMonth == -1) {
      howManyclickButtonMonth = 13;
    }
  }
  delay(10);
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  statusButtonChangeTimeMinus = digitalRead(buttonPinChangeTimeMinus);
  if (statusButtonChangeTimePlus == LOW && statusButtonChangeTimeMinus == LOW) {
    flag = false;
  }
  //-----------------------------------------------------------------------------------
  time.settime(-1, -1, -1, -1, howManyclickButtonMonth);
  lcd.setCursor(0, 1);
  lcd.print("Month:");
  lcd.print(" ");
  lcd.print(howManyclickButtonMonth);
  lcd.print(" ");
}
void setYear() {
  //-----------------------------------------------------------------------------------
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  if (statusButtonChangeTimePlus == HIGH && flag == false) {
    flag = true;
    howManyclickButtonYear = (howManyclickButtonYear + 1) % 99;
  }
  if (statusButtonChangeTimeMinus == HIGH && flag == false) {
    flag = true;
    howManyclickButtonYear = (howManyclickButtonYear - 1) % 99;
    if (howManyclickButtonYear == -1) {
      howManyclickButtonYear = 99;
    }
  }
  delay(10);
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  statusButtonChangeTimeMinus = digitalRead(buttonPinChangeTimeMinus);
  if (statusButtonChangeTimePlus == LOW && statusButtonChangeTimeMinus == LOW) {
    flag = false;
  }
  //-----------------------------------------------------------------------------------
  time.settime(-1, -1, -1, -1, -1, howManyclickButtonYear);
  lcd.setCursor(8, 0);
  lcd.print("Year:");
  lcd.print(" ");
  lcd.print(howManyclickButtonYear);
  lcd.print(" ");
}
void setAlarmOnOff() {
  //-----------------------------------------------------------------------------------
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  if (statusButtonChangeTimePlus == HIGH && flag == false) {
    flag = true;
    flagAlarm = (flagAlarm + 1) % 2;
  }
  delay(10);
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  if (statusButtonChangeTimePlus == LOW) {
    flag = false;
  }
  //-----------------------------------------------------------------------------------
  lcd.setCursor(0, 0);
  lcd.print("Alarm:");
  lcd.print(" ");
  if (flagAlarm == 1) {
    lcd.print("ON");
    lcd.print(" ");
  }
  if (flagAlarm == 0) {
    lcd.print("OFF");
    lcd.print(" ");
  }
}
void setAlarmHour() {
  //-----------------------------------------------------------------------------------
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  if (statusButtonChangeTimePlus == HIGH && flag == false) {
    flag = true;
    alarmHour = (alarmHour + 1) % 24;
  }
  if (statusButtonChangeTimeMinus == HIGH && flag == false) {
    flag = true;
    alarmHour = (alarmHour - 1) % 24;
    if (alarmHour == -1) {
      alarmHour = 23;
    }
  }
  delay(10);
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  statusButtonChangeTimeMinus = digitalRead(buttonPinChangeTimeMinus);
  if (statusButtonChangeTimePlus == LOW && statusButtonChangeTimeMinus == LOW) {
    flag = false;
  }
  //-----------------------------------------------------------------------------------

  lcd.setCursor(0, 1);
  lcd.print("Hours:");
  lcd.print(" ");
  lcd.print(alarmHour);
  lcd.print(" ");
}
void setAlarmMin() {
  //---------------------------------------------------------------------------------
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  if (statusButtonChangeTimePlus == HIGH && flag == false) {
    flag = true;
    alarmMin = (alarmMin + 1) % 60;
  }
  if (statusButtonChangeTimeMinus == HIGH && flag == false) {
    flag = true;
    alarmMin = (alarmMin - 1) % 60;
    if (alarmMin == -1) {
      alarmMin = 59;
    }
  }
  delay(10);
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  statusButtonChangeTimeMinus = digitalRead(buttonPinChangeTimeMinus);
  if (statusButtonChangeTimePlus == LOW && statusButtonChangeTimeMinus == LOW) {
    flag = false;
  }
  //-----------------------------------------------------------------------------------

  lcd.setCursor(0, 0);
  lcd.print("Minuts:");
  lcd.print(" ");
  lcd.print(alarmMin);
  lcd.print(" ");
}
void setAlarmWeek() {
  //-----------------------------------------------------------------------------------
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  if (statusButtonChangeTimePlus == HIGH && flag == false) {
    flag = true;
    alarmWeekClick = (alarmWeekClick + 1) % 3;
  }
  delay(10);
  statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
  if (statusButtonChangeTimePlus == LOW) {
    flag = false;
  }
  //-----------------------------------------------------------------------------------
  lcd.setCursor(0, 1);
  if (alarmWeekClick == 0) {
    lcd.print("Everyday");
    lcd.print(" ");
    alarmWeek1 = 0;
    alarmWeek2 = 6;
  }
  if (alarmWeekClick == 1) {
    lcd.print("Weekdays");
    lcd.print(" ");
    alarmWeek1 = 1;
    alarmWeek2 = 5;
  }
  if (alarmWeekClick == 2) {
    lcd.print("Weekend");
    lcd.print(" ");
    alarmWeek1 = 10;
    alarmWeek2 = 10;
  }
}
void alarm() {
  //-----------------будильник---------------------------------------
  //будильник будет работать или до нажатия кнопки или одну минуту
  lcd.clear();
  unsigned long timeBzzz = millis();
  unsigned long lastTimeBzzz = timeBzzz;
  unsigned long lastTimeBzzzPilik = timeBzzz;
  while (statusButtonChangeTimePlus == LOW && timeBzzz - lastTimeBzzz < 60000) {
    digitalWrite(pilikPin, HIGH);
    if (timeBzzz - lastTimeBzzzPilik > 10500) {
      digitalWrite(pilikPin, LOW);
      delay(100);
      lastTimeBzzzPilik = timeBzzz;
    }
    lcd.backlight();
    delay(100);
    lcd.noBacklight();
    delay(100);
    statusButtonChangeTimePlus = digitalRead(buttonPinChangeTimePlus);
    timeBzzz = millis();
  }
  digitalWrite(pilikPin, LOW);
  delay(300);
  statusButtonChangeTimePlus = LOW;
  satatusAlarm();  //после ококнчания работы будильника еще раз пропишем режим работы будильника
}
void temperature() {
 // lcd.clear();
  lcd.setCursor(3, 2);
  lcd.print("Temperature:");
  lcd.setCursor(5, 3);
  lcd.print(rtc.getTemp());
  lcd.print(" C");
  //delay(5000);
  //----------перед выходом из экрана температуры почистим экран-------------------------------------
  //    выставим режим работы будильника
 // satatusAlarm();
}
void satatusAlarm() {
  lcd.clear();
  lcd.setCursor(10, 0);
  if (flagAlarm == 1) {
    lcd.print("ON");
    lcd.print(" ");
  }
  if (flagAlarm == 0) {
    lcd.print("OFF");
  }
  lcd.setCursor(14, 0);
  if (alarmWeekClick == 0) {
    lcd.print("Ed");
  }
  if (alarmWeekClick == 1) {
    lcd.print("Wd");
  }
  if (alarmWeekClick == 2) {
    lcd.print("Wk");
  }
}
