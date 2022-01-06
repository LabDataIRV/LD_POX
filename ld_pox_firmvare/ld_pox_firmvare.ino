#define DISPLAY_ADDR 0x27 // адрес платы дисплея: 0x27 или 0x3f. Если дисплей не работает - смени адрес! На самом дисплее адрес не указан
#define BTN_PIN 5 // Пин кнопки
#define Td      10 // Период дискретизации (100 Гц)
#define SHOW_BEAT 400 // Период указания пульса
#define MAX_BEATS 10

// Сила тока светодиодов устанавливается так, 
// чтобы увеличить динамический дианазон и не выйти за пределы АЦП
//  MAX30100_LED_CURR_0MA 4_4 7_6 11 14_2 17_4 20_8 24 27_1 30_6 33_8 33_8 37 40_2 43_6 46_8 50
#define IR_LED_CURRENT  MAX30100_LED_CURR_50MA

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(DISPLAY_ADDR, 20, 4);

#include "GyverButton.h"
GButton button(BTN_PIN, LOW_PULL, NORM_OPEN);
//button.setDebounce(50);        // настройка антидребезга (по умолчанию 80 мс)
//button.setTimeout(300);        // настройка таймаута на удержание (по умолчанию 500 мс)
#include "MAX30100_PulseOximeter.h"
PulseOximeter pox;

uint32_t tsLastSampling = 0;
uint32_t tsLastBeat = 0;
byte mode = 0;
/*
  0 Ожидание действия
  1 Комбо экран
  2 Монитор сатурации
  3 Монитор ЧСС
  4 Передача данных
*/
// переменные для вывода
byte dispspO2 = 0;
byte dispHR = 0;
uint16_t ir = 0;
bool beat = false; // Нужно обработать сокращение
bool beat_show = false; // Нужно показывать сокражение
int beat_count = 0; // Число ударов
// массивы графиков
int sPO2_arr[15], HR_arr[15];

byte maxspO2 = 100;
byte minspO2 = 90;
byte maxHR = 200;
byte minHR = 59;
// символы
// график

byte row8[8] = {0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
byte row7[8] = {0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
byte row6[8] = {0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
byte row5[8] = {0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
byte row4[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111};
byte row3[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111};
byte row2[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111};
byte row1[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111};
/*
byte row8[8] = {0b11111,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000};
byte row7[8] = {0b00000,  0b11111,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000};
byte row6[8] = {0b00000,  0b00000,  0b11111,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000};
byte row5[8] = {0b00000,  0b00000,  0b00000,  0b11111,  0b00000,  0b00000,  0b00000,  0b00000};
byte row4[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b00000,  0b00000,  0b00000};
byte row3[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b00000,  0b00000};
byte row2[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b00000};
byte row1[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111};
*/
void onBeatDetected() { // Обработка прерывания по удару сердца
    beat = true;
    beat_show = true;
    tsLastBeat = millis();
    beat_count++;
}

void BeatTick(){  // Обработка сокращения
   if (beat) {
     byte spO2 = 0;
     byte HR = 0;
     spO2 = constrain(pox.getSpO2(), 0, 250);
     HR = constrain(round(pox.getHeartRate()), 0, 254);
     if (spO2>0) dispspO2 = spO2;
     if (HR>0) dispHR = HR;
     
     for (byte i = 0; i < 14; i++) {
        sPO2_arr[i] = sPO2_arr[i + 1];
        HR_arr[i] = HR_arr[i + 1]; 
     }
     sPO2_arr[14] = dispspO2;
     HR_arr[14] = dispHR;
     //if (beat_count>= MAX_BEATS) beat_count = 0;
   }
   // Обработка показа сокращения
   if ((beat_show)and(mode == 1)and(millis()-tsLastBeat<SHOW_BEAT)) {
      lcd.setCursor(0, 2);
      lcd.print("-Pulse-");
   }
   if ((beat_show)and(mode == 1)and(millis()-tsLastBeat>SHOW_BEAT)) {
      lcd.setCursor(0, 2);
      lcd.print("       ");
      beat_show = false;
   }
}
void Waiting() { // Режим ожидания
    lcd.setCursor(0, 0);lcd.print("LD.Pulsoximeter ^  ");
    lcd.setCursor(0, 1);lcd.print("<-Press a key   |  ");
    lcd.setCursor(0, 2);lcd.print("                |   ");
    lcd.setCursor(0, 3);lcd.print("Put finger on sensor ");
} 
void Transmit() { // Режим передачи
    lcd.setCursor(0, 0);lcd.print("Transmit mode   ^  ");
    lcd.setCursor(0, 1);lcd.print("<-Totch to stop |  ");
    lcd.setCursor(0, 2);lcd.print("                |   ");
    lcd.setCursor(0, 3);lcd.print("Put finger on sensor ");
} 
void ComboShow(){  // Обновление мультиэкрана
    lcd.setCursor(0, 0);
    lcd.print("                    ");
    lcd.setCursor(0, 0);
    lcd.print("HR: "+String(round(dispHR))+" bpm");
    lcd.setCursor(0, 1);
    lcd.print("                    ");
    lcd.setCursor(0, 1);
    lcd.print("SpO2: "+String(dispspO2)+" %");
    lcd.setCursor(0, 3);
    lcd.print("                    ");
    lcd.setCursor(0, 3);
    lcd.print("Beats: "+String(beat_count));
}
void ReadingTick(){ // Чтение пульсовой волны
    if (millis() - tsLastSampling > Td) { 
      tsLastSampling = millis();
      ir = 65536 - pox.getIR();
      Serial.println(ir,DEC); // отправка в порт
    }
}
void ButtonTick(){ // Чтение нажаний кнопки
    button.tick();
    if (button.isSingle()) { // Если кнопка нажата
        if (mode == 0) {
          pox.resume();
        } 
        mode++; // Смена режимов отображения
        if (mode == 4) mode = 1;
        if (mode > 4) {mode == 0; Waiting(); pox.shutdown();}
        lcd.clear();
    }
    if (button.isDouble()) { // Если двойное нажание
        mode = 0; // Ожидание
        lcd.clear();
        pox.shutdown();
    }
    if (button.isHolded()) { // Если нажание с фиксацией
        if (mode == 0) {
          pox.resume();
        }
        mode = 4; // Передача
        lcd.clear();
        Transmit();  
    }
}
void LCDinit(){ // Инициализация экрана
  lcd.begin();
  lcd.backlight();
  lcd.clear();
}
void POXinit(){ // Инициализация пульсоксиметра
  if (!pox.begin()) {
        Serial.println("Sensor FAILED!");
        lcd.setCursor(0, 0);
        lcd.print("ERROR");
        lcd.setCursor(1, 0);
        lcd.print("Sensor FAILED!");
        for(;;);
  } else {
        Serial.println("SUCCESS sensor");
  }
  pox.setIRLedCurrent(IR_LED_CURRENT);
  pox.setOnBeatDetectedCallback(onBeatDetected);
  pox.shutdown(); //Режим ожидания
  mode = 0;
}
void loadPlot() { // Инициализация графика
  lcd.createChar(0, row8);
  lcd.createChar(1, row1);
  lcd.createChar(2, row2);
  lcd.createChar(3, row3);
  lcd.createChar(4, row4);
  lcd.createChar(5, row5);
  lcd.createChar(6, row6);
  lcd.createChar(7, row7);
}
void drawPlot(byte pos, byte row, byte width, byte height, int min_val, int max_val, int *plot_array, String label) { //Отрисовка графика
  //lcd.clear();
  int max_value = -32000;
  int min_value = 32000;

  for (byte i = 0; i < 15; i++) {
    if (plot_array[i] > max_value) max_value = plot_array[i];
    if (plot_array[i] < min_value) min_value = plot_array[i];
  }
  //min_val = min_value; max_val = max_value;
  lcd.setCursor(16, 0); lcd.print("    ");
  lcd.setCursor(16, 0); lcd.print(max_value);
  lcd.setCursor(16, 1); lcd.print(label);
  lcd.setCursor(16, 2); lcd.print("    ");
  lcd.setCursor(16, 2); lcd.print(plot_array[14]);
  lcd.setCursor(16, 3); lcd.print("    ");
  lcd.setCursor(16, 3); lcd.print(min_value);
  
  for (byte i = 0; i < width; i++) {                  // каждый столбец параметров
    int fill_val = plot_array[i];
    fill_val = constrain(fill_val, min_val, max_val);
    byte infill, fract;
    // найти количество целых блоков с учётом минимума и максимума для отображения на графике
    if (plot_array[i] > min_val)
      infill = floor((float)(plot_array[i] - min_val) / (max_val - min_val) * height * 10);
    else infill = 0;
    fract = (float)(infill % 10) * 8 / 10;                   // найти количество оставшихся полосок
    infill = infill / 10;

    for (byte n = 0; n < height; n++) {     // для всех строк графика
      if (n < infill && infill > 0) {       // пока мы ниже уровня
        lcd.setCursor(i, (row - n));        // заполняем полными ячейками
        lcd.write(0);
      }
      if (n >= infill) {                    // если достигли уровня
        lcd.setCursor(i, (row - n));
        if (fract > 0) lcd.write(fract);          // заполняем дробные ячейки
        else lcd.write(16);                       // если дробные == 0, заливаем пустой
        for (byte k = n + 1; k < height; k++) {   // всё что сверху заливаем пустыми
          lcd.setCursor(i, (row - k));
          lcd.write(16);
        }
        break;
      }
    }
  }
}

void setup() {
  Serial.begin(9600);
  LCDinit();
  loadPlot();
  POXinit();
  mode = 0;
}

void loop() {
    pox.update(); // Отработка пульсоксиметра
    ButtonTick();
    // Обработка режимов
    switch (mode) { // Выбранный режим
       case 0: // Режим ожидания
               Waiting();
               break;
       case 1: // Комбо панель
               ComboMode();
               break;
       case 2: // Монитор сатурации
               spMonitor();
               break;
       case 3: // Монитор ЧСС
               HRMonitor();
               break;
       case 4: // Режим передачи сигнала
               TransmitMode();
               break;
    } 
}
void ComboMode() {
  BeatTick();
  if (beat){
    ComboShow();
    beat = false;
  }
}
void spMonitor() {
   BeatTick();
   if (beat){
     drawPlot(0, 3, 15, 4, minspO2, maxspO2, (int*)sPO2_arr, "spO2");
     beat = false;
   }
}
void HRMonitor() {
  BeatTick();
   if (beat){
     drawPlot(0, 3, 15, 4, minHR, maxHR, (int*)HR_arr, "HR");
     beat = false;
   }
}
void TransmitMode() {
  ReadingTick();
}
