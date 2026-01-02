#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <GyverButton.h>
#include <FastBot.h>
#include <ESP8266WiFi.h>
#include <vector>

// === КОНФИГ ===
#define WIFI_SSID "TP-Link_2163"
#define WIFI_PASS "63698077"
#define BOT_TOKEN "8461450359:AAF68YhuKzw33tLcOcrNWusaJfgYaEJkDNc"
#define MY_CHAT_ID "1231597064"

// === ПИНЫ ===
#define BUTTON_PIN D7
#define LED_PIN    D6
#define BUZZER_PIN D5

// === ОБЪЕКТЫ ===
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, D1, D2);

// ВЕРНУЛ КАК БЫЛО: Стандартная инициализация (для кнопки, замыкающей на GND)
GButton btn(BUTTON_PIN); 

FastBot bot(BOT_TOKEN);

// === ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ===
enum Mode { MENU_MAIN, MODE_READ, MODE_WRITE, MODE_WIFI, MODE_GAME };
Mode currentMode = MENU_MAIN;

// Меню
int menuCursor = 0;
const char* menuItems[] = { "Read Msg", "Write Msg", "Wi-Fi Scan", "Flappy Bird" };
const int menuCount = 4;

// Сообщения
String lastMessage = "No messages yet"; 
bool hasUnreadMsg = false;
std::vector<String> msgLines; 
int readPage = 0;             

// Клавиатура
String inputBuffer = ""; 
int kbGroupIdx = 0;      
int kbCharIdx = 0;       
bool isInsideGroup = false; 

// Группы символов
const char* kbGroups[] = {
  "АБВГД", "ЕЖЗИЙ", "КЛМНО", "ПРСТУ", 
  "ФХЦЧШ", "ЩЪЫЬЭ", "ЮЯ.,?", 
  "abcde", "fghij", "klmno", "pqrst", "uvwxyz", 
  "12345", "67890", 
  "SEND" 
};
const int kbGroupCount = 15; 
// === ИГРА ===
float birdY = 32;       // Высота птички
float birdVel = 0;      // Скорость птички
int score = 0;          // Счет
int wallX = 128;        // Координата стены по X
int gapY = 30;          // Координата дырки в стене по Y
const int gapSize = 25; // Размер дырки
bool gameStarted = false;

// Таймеры
unsigned long wifiTimer = 0; 

// === ПРОТОТИПЫ ===
void handleMenu();
void drawMenu();
void drawStatusBar(String title);
void newMsg(FB_msg& msg);
void prepareMsg(String text);
void handleKeyboard();
int utf8Len(String str);
String getErrChar(String str, int idx);

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); 

  u8g2.begin();
  u8g2.enableUTF8Print(); 
  
  // Настройка кнопки
  // ВАЖНО: setType(HIGH_PULL) - это стандарт для ардуино (кнопка к земле)
  btn.setType(HIGH_PULL); 
  btn.setDirection(NORM_OPEN); 
  
  btn.setDebounce(50);      
  btn.setTimeout(400);      // Тайм-аут для распознавания двойного клика
  btn.setClickTimeout(600); // Удержание

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  bot.setChatID(MY_CHAT_ID);
  bot.attach(newMsg);
}

void loop() {
  btn.tick(); 
  bot.tick(); 
  
  if (millis() - wifiTimer > 30000) {
    if (WiFi.status() != WL_CONNECTED) {
       WiFi.reconnect();
    }
    wifiTimer = millis();
  }

  switch (currentMode) {
    case MENU_MAIN:
      handleMenu();
      drawMenu();
      break;
      
    case MODE_READ:
      drawStatusBar("Msg P." + String(readPage + 1));
      u8g2.setFont(u8g2_font_cu12_t_cyrillic); 
      for (int i = 0; i < 4; i++) {
        int lineIndex = readPage * 4 + i;
        if (lineIndex < (int)msgLines.size()) { 
           u8g2.setCursor(0, 25 + (i * 12)); 
           u8g2.print(msgLines[lineIndex]);
        }
      }
      u8g2.sendBuffer();

      if (btn.isClick()) {
        readPage++;
        if (readPage * 4 >= (int)msgLines.size()) readPage = 0; 
        tone(BUZZER_PIN, 1000, 30);
      }
      if (btn.isHolded()) {
        tone(BUZZER_PIN, 500, 100);
        hasUnreadMsg = false;
        digitalWrite(LED_PIN, LOW);
        currentMode = MENU_MAIN;
      }
      break;

    case MODE_WRITE:
      handleKeyboard(); 
      break;

    case MODE_GAME:
      handleGame();
      break;
      
    case MODE_WIFI:
       // Сюда добавим сканер позже
       drawStatusBar("Wi-Fi Scanner");
       u8g2.setFont(u8g2_font_6x10_tr);
       u8g2.drawStr(10, 30, "Scanning...");
       u8g2.sendBuffer();
       
       // Тут будет код сканера...
       
       if (btn.isHolded()) currentMode = MENU_MAIN;
       break;

  }
}

void newMsg(FB_msg& msg) {
  if (msg.chatID == MY_CHAT_ID) {
    lastMessage = msg.text;
    prepareMsg(lastMessage); 
    hasUnreadMsg = true;
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 2000, 100);
  }
}

void prepareMsg(String text) {
  msgLines.clear();
  readPage = 0;
  int lineLen = 18; 
  int len = text.length();
  for (int i = 0; i < len; i += lineLen) {
    if (i + lineLen < len) {
       msgLines.push_back(text.substring(i, i + lineLen));
    } else {
       msgLines.push_back(text.substring(i));
    }
  }
}

void drawStatusBar(String title) {
  u8g2.clearBuffer(); 
  u8g2.drawLine(0, 10, 128, 10);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.setCursor(2, 8); 
  u8g2.print(title);
  
  u8g2.setFont(u8g2_font_open_iconic_embedded_1x_t); 
  if (WiFi.status() == WL_CONNECTED) {
    u8g2.drawGlyph(118, 9, 80); 
  } else {
     u8g2.setFont(u8g2_font_5x8_tr);
     u8g2.drawStr(118, 8, "x"); 
  }
  
  u8g2.drawFrame(100, 0, 14, 8); 
  u8g2.drawBox(114, 2, 2, 4);    
  u8g2.drawBox(102, 2, 10, 4);   
}

void handleMenu() {
  // В меню используем isClick, чтобы не ждать тайм-аут двойного клика (так быстрее)
  if (btn.isClick()) {
    menuCursor++;
    if (menuCursor >= menuCount) menuCursor = 0;
    tone(BUZZER_PIN, 1000, 30);
  }
  if (btn.isHolded()) {
    tone(BUZZER_PIN, 1500, 100);
    switch (menuCursor) {
      case 0: currentMode = MODE_READ; break;
      case 1: currentMode = MODE_WRITE; break;
      case 2: currentMode = MODE_WIFI; break;
      case 3: currentMode = MODE_GAME; break;
    }
  }
}

void drawMenu() {
  drawStatusBar("Main Menu"); 
  u8g2.setFont(u8g2_font_6x10_tr);
  for (int i = 0; i < menuCount; i++) {
    int y = 25 + (i * 12);
    if (i == menuCursor) u8g2.drawStr(5, y, ">");
    u8g2.drawStr(15, y, menuItems[i]);
  }
  if (hasUnreadMsg) u8g2.drawDisc(100, 22, 3); 
  u8g2.sendBuffer();
}

// === КЛАВИАТУРА ===
void handleKeyboard() {
  drawStatusBar("Write Msg");
  
  u8g2.setFont(u8g2_font_cu12_t_cyrillic);
  u8g2.setCursor(0, 25);
  u8g2.print(inputBuffer + "_"); 
  u8g2.drawLine(0, 32, 128, 32); 
  
  if (!isInsideGroup) {
    // === ВЫБОР ГРУППЫ ===
    u8g2.setFont(u8g2_font_cu12_t_cyrillic); 
    u8g2.setCursor(40, 52); 
    u8g2.print(kbGroups[kbGroupIdx]);

    u8g2.drawFrame(35, 40, 60, 16);

    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.setCursor(0, 63);
    u8g2.print("1x:Next 2x:Del Hold:Ok");
    
    // 1. Двойной клик - СТЕРЕТЬ
    if (btn.isDouble()) {
       if (inputBuffer.length() > 0) {
           char last = inputBuffer.charAt(inputBuffer.length()-1);
           inputBuffer.remove(inputBuffer.length()-1);
           if ((uint8_t)last >= 0x80) inputBuffer.remove(inputBuffer.length()-1);
           tone(BUZZER_PIN, 500, 50);
           delay(50);
           tone(BUZZER_PIN, 300, 50);
       }
    }
    
    // 2. Одиночный клик - ЛИСТАТЬ (используем isSingle, чтобы не путать с двойным)
    if (btn.isSingle()) { 
       kbGroupIdx++;
       if (kbGroupIdx >= kbGroupCount) kbGroupIdx = 0;
       tone(BUZZER_PIN, 1000, 30);
    }

    // 3. Удержание - ВХОД
    if (btn.isHolded()) {
       if (String(kbGroups[kbGroupIdx]) == "SEND") {
           if (inputBuffer.length() > 0) {
              bot.sendMessage(inputBuffer);
              inputBuffer = "";
              currentMode = MENU_MAIN;
              tone(BUZZER_PIN, 2000, 300);
           }
       } 
       else {
           isInsideGroup = true;
           kbCharIdx = 0;
           tone(BUZZER_PIN, 1500, 100);
       }
    }
  } 
  else {
    // === ВНУТРИ ГРУППЫ ===
    String group = kbGroups[kbGroupIdx];
    
    u8g2.setFont(u8g2_font_cu12_t_cyrillic);
    u8g2.setCursor(2, 45);
    u8g2.print(group);

    String currentC = getErrChar(group, kbCharIdx);
    u8g2.drawFrame(54, 38, 20, 24); 
    u8g2.setCursor(60, 55);
    u8g2.print(currentC);
    
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.setCursor(80, 63);
    u8g2.print("Hold:Pick");

    if (btn.isClick()) {
       kbCharIdx++;
       if (kbCharIdx >= utf8Len(group)) kbCharIdx = 0;
       tone(BUZZER_PIN, 1200, 30);
    }
    
    if (btn.isHolded()) {
       inputBuffer += getErrChar(group, kbCharIdx);
       isInsideGroup = false; 
       tone(BUZZER_PIN, 1800, 100);
    }
  }
  
  u8g2.sendBuffer();
}

int utf8Len(String str) {
  int len = 0;
  for (int i=0; i < str.length(); i++) {
    if ((str[i] & 0xC0) != 0x80) len++; 
  }
  return len;
}

String getErrChar(String str, int idx) {
  int p = 0;
  for (int i=0; i < str.length(); i++) {
    if ((str[i] & 0xC0) != 0x80) { 
       if (p == idx) {
          String res = "";
          res += str[i];
          while (i+1 < str.length() && (str[i+1] & 0xC0) == 0x80) {
             res += str[++i];
          }
          return res;
       }
       p++;
    }
  }
  return "?";
}

void handleGame() {
  static unsigned long lastGameFrame = 0;

  // === МЕНЮ СТАРТА ===
  if (!gameStarted) {
     u8g2.clearBuffer();
     u8g2.setFont(u8g2_font_ncenB14_tr); // Красивый шрифт
     u8g2.drawStr(25, 30, "Flappy");
     u8g2.setFont(u8g2_font_5x8_tr);
     u8g2.drawStr(30, 50, "Click to Start");
     u8g2.sendBuffer();
     
     if (btn.isClick()) {
        gameStarted = true;
        birdY = 32;
        birdVel = -1.5; // Чуть подкидываем на старте
        wallX = 128;
        score = 0;
        lastGameFrame = millis();
        tone(BUZZER_PIN, 1000, 100);
     }
     if (btn.isHolded()) currentMode = MENU_MAIN;
     return;
  }

  // === ПРОВЕРКА КЛИКА (Всегда, чтобы не пропустить) ===
  if (btn.isClick()) {
     birdVel = -3.0; // Сила прыжка (Подбери под себя: -2.5 мягко, -4.0 резко)
     tone(BUZZER_PIN, 600, 20);
  }

  // === ОБНОВЛЕНИЕ ФИЗИКИ (Раз в 30 мс = ~33 FPS) ===
  if (millis() - lastGameFrame > 30) { 
    lastGameFrame = millis();

    // Гравитация
    birdVel += 0.4; 
    birdY += birdVel;
    
    // Движение стены
    wallX -= 3; // Скорость скролла
    if (wallX < -10) {
       wallX = 128;
       gapY = random(10, 40); // Высота дырки
       score++;
       tone(BUZZER_PIN, 2000, 50);
    }
  }

  // === ПРОВЕРКА СТОЛКНОВЕНИЙ ===
  bool collision = false;
  // Пол и потолок
  if (birdY > 62 || birdY < 0) collision = true; 
  
  // Стена
  if (wallX < 24 && wallX > 6) { // Если стена проходит через птицу (X птицы = 20)
     if (birdY < gapY || birdY > gapY + gapSize) {
        collision = true;
     }
  }

  if (collision) {
     tone(BUZZER_PIN, 100, 500);
     u8g2.clearBuffer();
     u8g2.setFont(u8g2_font_ncenB14_tr);
     u8g2.drawStr(10, 30, "GAME OVER");
     u8g2.setFont(u8g2_font_6x10_tr);
     u8g2.setCursor(35, 50);
     u8g2.print("Score: " + String(score));
     u8g2.sendBuffer();
     delay(1500); 
     gameStarted = false;
     return;
  }

  // === ОТРИСОВКА ===
  u8g2.clearBuffer();
  
  // Птичка (круг)
  u8g2.drawDisc(20, (int)birdY, 3);
  u8g2.drawPixel(21, (int)birdY - 1); // Глаз
  u8g2.drawLine(16, (int)birdY, 12, (int)birdY-2); // Крыло (для красоты)
  
  // Трубы
  u8g2.drawBox(wallX, 0, 12, gapY); // Верхняя
  u8g2.drawBox(wallX, gapY + gapSize, 12, 64 - (gapY + gapSize)); // Нижняя
  
  // Рамка вокруг труб (чтобы выделялись)
  u8g2.drawFrame(wallX, 0, 12, gapY);
  u8g2.drawFrame(wallX, gapY + gapSize, 12, 64 - (gapY + gapSize));

  // Счет
  u8g2.setFont(u8g2_font_ncenB14_tr); // Крупный счет
  u8g2.setCursor(100, 15);
  u8g2.print(score);
  
  // Пол
  u8g2.drawLine(0, 63, 128, 63);

  u8g2.sendBuffer();
  
  if (btn.isHolded()) {
     gameStarted = false;
     currentMode = MENU_MAIN;
  }
}


