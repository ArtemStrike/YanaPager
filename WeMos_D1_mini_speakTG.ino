#include <FastBot.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ D1, /* data=*/ D2);

// === НАСТРОЙКИ ===
#define WIFI_SSID "TP-Link_2163"                                  // Имя Wi-Fi
#define WIFI_PASS "63698077"                                  // Пароль Wi-Fi
#define BOT_TOKEN "8461450359:AAF68YhuKzw33tLcOcrNWusaJfgYaEJkDNc"  // Токен бота
#define MY_CHAT_ID "1231597064"                                     // Твой ID (цифры)

// === ПИНЫ ===
#define BUZZER_PIN D5  // Пищалка
#define LED_PIN D6     // Светодиод
#define BUTTON_PIN D7  // Кнопка

FastBot bot(BOT_TOKEN);

// Переменные
String message = "Дарова братан чё ты как ты"; 
bool isMessageReceived = false;

void setup() {
  Serial.begin(115200);
  u8g2.begin();
  u8g2.enableUTF8Print();
  // Настройка пинов
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Выключаем всё на старте
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Подключение к Wi-Fi
  connectWiFi();
  for(int i=0; i<=100; i+=5) { showLoading(i); delay(100); }
  // Настройка бота
  bot.setChatID(MY_CHAT_ID);
  bot.attach(newMsg);  // Подключаем функцию обработки сообщений
  bot.sendMessage("Wemos в сети! Отправь цифру 1-9.");
}

// Функция обработки входящих сообщений
void newMsg(FB_msg& msg) {
  // Проверяем, что пишет хозяин
  if (msg.chatID == MY_CHAT_ID) {

    message = msg.text;
    isMessageReceived = true;
    digitalWrite(LED_PIN, HIGH);  // ЗАЖИГАЕМ СВЕТОДИОД
    bot.sendMessage("Сообщение отправлено!");
  } else {
    bot.sendMessage("У вас нет доступа");
  }
}

void loop() {
  bot.tick();  // Обязательная проверка обновлений бота
  u8g2.clearBuffer();

  // Если нажата кнопка (LOW) И есть непрочитанное сообщение
  if (!digitalRead(BUTTON_PIN) && isMessageReceived) {

    // Сначала гасим светодиод, типа прочитали
    digitalWrite(LED_PIN, LOW);
    
    scrollText(message, 15);

    // Сбрасываем флаг, сообщение прочитано
    isMessageReceived = false;

    // Антидребезг (чтобы не сработало дважды за одно нажатие)
    delay(500);
  }
}

void scrollText(String text, int speed) {
  u8g2.setFont(u8g2_font_cu12_t_cyrillic); // Обязательно тот же шрифт
  
  // getUTF8Width правильно считает ширину русских букв
  int textWidth = u8g2.getUTF8Width(text.c_str());
  
  for (int x = 128; x > -textWidth; x--) {
    u8g2.clearBuffer();
    u8g2.setCursor(x, 40);
    u8g2.print(text); // print умеет печатать русские, если включен enableUTF8Print
    u8g2.sendBuffer();
    delay(speed);
  }
}


void showLoading(int percent) {
  u8g2.clearBuffer();
  
  // 1. Рисуем рамку бара
  u8g2.drawFrame(10, 30, 108, 10); // x=10, y=30, ширина 108, высота 10
  
  // 2. Вычисляем ширину заливки (математика!)
  // Макс ширина внутри рамки = 108 - 4 (отступы) = 104 примерно
  int width = map(percent, 0, 100, 0, 104);
  
  // 3. Рисуем "палку" внутри
  u8g2.drawBox(12, 32, width, 6); 
  
  // 4. Пишем проценты
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.setCursor(50, 20);
  u8g2.print("Loading ");
  u8g2.print(percent);
  u8g2.print("%");
  
  u8g2.sendBuffer();
}

void connectWiFi() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr); 
  u8g2.drawStr(0,15,"Connect WIFI");
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.setCursor(0, 30);
  u8g2.print("name: ");
  u8g2.print(WIFI_SSID);
  u8g2.setCursor(0, 45);
  u8g2.print("pass: ");
  u8g2.print(WIFI_PASS);
  u8g2.setFont(u8g2_font_open_iconic_all_2x_t); // Шрифт с иконками
  u8g2.drawGlyph(58, 64, 80); // 80 - это код значка Wi-Fi в этом шрифте
  u8g2.sendBuffer();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    
    // 1. Делаем "ПУМ" (короткий писк)
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    
    Serial.print(".");
    
    // 2. Ждем 2 секунды (но проверяем статус, чтобы не тупить лишнее время)
    for (int i = 0; i < 20; i++) {
      if (WiFi.status() == WL_CONNECTED) break; // Выход, если подключились раньше
      delay(100);
    }
  }
  // Успех: три быстрых писка
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }

  Serial.println("Connected!");
}
