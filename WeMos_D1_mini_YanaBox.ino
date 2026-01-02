#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <GyverButton.h>
#include <FastBot.h>
#include <ESP8266WiFi.h>
#include <vector>
#include <time.h>
#include <EEPROM.h>

// === КОНФИГ ===
#define WIFI_SSID_DEFAULT "TP-LINK_600574"
#define WIFI_PASS_DEFAULT "72842058"
#define WIFI_SSID_2DEFAULT "TP-Link_2163"
#define WIFI_PASS_2DEFAULT "63698077"
#define BOT_TOKEN "8461450359:AAF68YhuKzw33tLcOcrNWusaJfgYaEJkDNc"
#define MY_CHAT_ID "1231597064"

// NTP
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET 25200
#define DAYLIGHT_OFFSET 0
#define LOVE_START_DATE 1665504000L

// Батарея
#define BAT_PIN A0
#define BAT_MIN 500
#define BAT_MAX 900

// Пины
#define BUTTON_PIN D7
#define LED_PIN    D6
#define BUZZER_PIN D5

// Энергосбережение
#define SCREEN_TIMEOUT 30000

// EEPROM
#define EEPROM_SIZE 512
#define TAMA_ADDR 0
#define MAGIC_BYTE 0xAB

// === ОБЪЕКТЫ ===
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, D1, D2);
GButton btn(BUTTON_PIN);
FastBot bot(BOT_TOKEN);

// === БИТМАПЫ ===
const unsigned char epd_bitmap_cat [] PROGMEM = {
	0x04, 0x08, 0x00, 0x0a, 0x14, 0x00, 0xf2, 0x13, 0x00, 0x02, 0x10, 0x00,
	0x02, 0x10, 0x00, 0x12, 0x22, 0x00, 0x11, 0x22, 0x00, 0x01, 0x20, 0x00,
	0xc2, 0x10, 0x03, 0x04, 0x88, 0x04, 0x18, 0x86, 0x02, 0x08, 0x98, 0x02,
	0x08, 0xa0, 0x04, 0x08, 0x40, 0x05, 0x08, 0x44, 0x05, 0x48, 0x42, 0x05,
	0x48, 0xc2, 0x04, 0x48, 0x42, 0x02, 0x48, 0xc6, 0x01, 0xb0, 0x39, 0x00
};
const unsigned char epd_bitmap_cat_angry [] PROGMEM = {
	0x04, 0x08, 0x00, 0x0a, 0x14, 0x00, 0xf2, 0x13, 0x00, 0x22, 0x11, 0x00,
	0xc2, 0x10, 0x00, 0x1a, 0x26, 0x00, 0x11, 0x22, 0x00, 0x01, 0x20, 0x00,
	0xc2, 0x10, 0x03, 0x04, 0x88, 0x04, 0x18, 0x86, 0x02, 0x08, 0x98, 0x02,
	0x08, 0xa0, 0x04, 0x08, 0x40, 0x05, 0x08, 0x44, 0x05, 0x48, 0x42, 0x05,
	0x48, 0xc2, 0x04, 0x48, 0x42, 0x02, 0x48, 0xc6, 0x01, 0xb0, 0x39, 0x00
};
const unsigned char epd_bitmap_cat_sad [] PROGMEM = {
	0x04, 0x08, 0x00, 0x0a, 0x14, 0x00, 0xf2, 0x13, 0x00, 0x02, 0x10, 0x00,
	0x02, 0x10, 0x00, 0x02, 0x20, 0x00, 0x31, 0x23, 0x00, 0xc1, 0x20, 0x00,
	0x22, 0x11, 0x03, 0x04, 0x88, 0x04, 0x18, 0x86, 0x02, 0x08, 0x98, 0x02,
	0x08, 0xa0, 0x04, 0x08, 0x40, 0x05, 0x08, 0x44, 0x05, 0x48, 0x42, 0x05,
	0x48, 0xc2, 0x04, 0x48, 0x42, 0x02, 0x48, 0xc6, 0x01, 0xb0, 0x39, 0x00
};
const unsigned char epd_bitmap_cat_dead [] PROGMEM = {
	0x04, 0x08, 0x00, 0x0a, 0x14, 0x00, 0xf2, 0x13, 0x00, 0x02, 0x10, 0x00,
	0x2a, 0x15, 0x00, 0x12, 0x22, 0x00, 0x29, 0x25, 0x00, 0x01, 0x20, 0x00,
	0xe2, 0x11, 0x03, 0x84, 0x88, 0x04, 0x18, 0x86, 0x02, 0x08, 0x98, 0x02,
	0x08, 0xa0, 0x04, 0x08, 0x40, 0x05, 0x08, 0x44, 0x05, 0x48, 0x42, 0x05,
	0x48, 0xc2, 0x04, 0x48, 0x42, 0x02, 0x48, 0xc6, 0x01, 0xb0, 0x39, 0x00
};
const unsigned char epd_bitmap_cat_sleep [] PROGMEM = {
	0x04, 0x08, 0x00, 0x0a, 0x14, 0x00, 0xf2, 0x13, 0x00, 0x02, 0x10, 0x00,
	0x02, 0x10, 0x00, 0x32, 0x23, 0x00, 0x01, 0x20, 0x00, 0x01, 0x20, 0x00,
	0xc2, 0x10, 0x03, 0x04, 0x88, 0x04, 0x18, 0x86, 0x02, 0x08, 0x98, 0x02,
	0x08, 0xa0, 0x04, 0x08, 0x40, 0x05, 0x08, 0x44, 0x05, 0x48, 0x42, 0x05,
	0x48, 0xc2, 0x04, 0x48, 0x42, 0x02, 0x48, 0xc6, 0x01, 0xb0, 0x39, 0x00
};
const unsigned char epd_bitmap_cat_funny [] PROGMEM = {
	0x04, 0x08, 0x00, 0x0a, 0x14, 0x00, 0xf2, 0x13, 0x00, 0x02, 0x10, 0x00,
	0x02, 0x10, 0x00, 0x12, 0x22, 0x00, 0x11, 0x22, 0x00, 0x01, 0x20, 0x00,
	0x22, 0x11, 0x03, 0xc4, 0x88, 0x04, 0x18, 0x86, 0x02, 0x08, 0x98, 0x02,
	0x08, 0xa0, 0x04, 0x08, 0x40, 0x05, 0x08, 0x44, 0x05, 0x48, 0x42, 0x05,
	0x48, 0xc2, 0x04, 0x48, 0x42, 0x02, 0x48, 0xc6, 0x01, 0xb0, 0x39, 0x00
};
const unsigned char epd_bitmap_poop [] PROGMEM = {
	0x40, 0x22, 0x49, 0x1a, 0x2c, 0x5e, 0x7e
};
const unsigned char epd_bitmap_fish [] PROGMEM = {
	0x38, 0x00, 0x71, 0x00, 0xfb, 0x01, 0x7e, 0x03, 0xfe, 0x03, 0xfb, 0x01,
	0x71, 0x00, 0x0c, 0x00
};
const unsigned char epd_bitmap_gamepad [] PROGMEM = {
	0xf0, 0xff, 0x07, 0x0c, 0x00, 0x18, 0x02, 0x00, 0x20, 0x62, 0x00, 0x23,
	0x61, 0x00, 0x43, 0xf9, 0xc1, 0x4c, 0xf9, 0xc1, 0x4c, 0x61, 0x00, 0x43,
	0x61, 0x36, 0x43, 0x02, 0x00, 0x20, 0x0c, 0x3e, 0x18, 0xf0, 0xc1, 0x07
};
const unsigned char epd_bitmap_toilet_paper [] PROGMEM = {
	0xf8, 0xff, 0x07, 0x04, 0x80, 0x08, 0x02, 0x40, 0x10, 0x02, 0x40, 0x10,
	0x01, 0x20, 0x22, 0x01, 0x20, 0x25, 0x01, 0x20, 0x25, 0x01, 0x20, 0x25,
	0x01, 0x20, 0x25, 0x01, 0x20, 0x22, 0x01, 0x60, 0x10, 0x02, 0x40, 0x10,
	0x02, 0xc0, 0x08, 0x02, 0xc0, 0x07, 0x02, 0x40, 0x00, 0x02, 0x40, 0x00,
	0x02, 0x40, 0x00, 0x02, 0x40, 0x00, 0xfc, 0x3f, 0x00
};

// === РЕЖИМЫ ===
enum Mode { 
  MENU_MAIN, MODE_READ, MODE_WRITE, MODE_WIFI, MODE_WIFI_PASS, MODE_GAME,
  MODE_MAGIC_CAT, MODE_LOVE_COUNTER, MODE_TAMAGOTCHI, MODE_TAMA_ACTIONS
};
Mode currentMode = MENU_MAIN;

// === СТРУКТУРА ТАМАГОЧИ ===
struct Tamagotchi {
  byte magicByte;
  int hunger;
  int happiness;
  int health;
  int sleepiness;
  int age;
  bool poop;
  unsigned long lastFeed;
  unsigned long lastPlay;
  unsigned long lastClean;
  unsigned long lastSleep;
  unsigned long birthTime;
  bool isDead;
} pet;

// === ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ===
int menuCursor = 0;
const char* menuItems[] = { "Read Msg", "Write Msg", "Wi-Fi Scan", "Flappy Bird", "Magic Cat", "Love Days", "Tamagotchi" };
const int menuCount = 7;

String lastMessage = "No messages yet";
bool hasUnreadMsg = false;
std::vector<String> msgLines;
int readPage = 0;

String inputBuffer = "";
String targetSSID = "";
int kbGroupIdx = 0;
int kbCharIdx = 0;
bool isInsideGroup = false;
bool isRusLayout = true;

const char* kbGroupsEN[] = {
  "abcdef", "ghijkl", "mnopqr", "stuvwxyz", 
  "1234567890", ".,?!+-_@=", "SEND", "<", "RU", "EXIT"
};
const char* kbGroupsRU[] = {
  "абвгде", "жзийкл", "мнопрст", "уфхцчш", "щъыьэюя",
  "1234567890", ".,?!+-_@=", "SEND", "<", "EN", "EXIT"
};

float birdY = 32;
float birdVel = 0;
int score = 0;
int wallX = 128;
int gapY = 30;
const int gapSize = 25;
bool gameStarted = false;

int wifiScanState = 0;
int wifiScroll = 0;
unsigned long wifiTimer = 0;

int magicCatState = 0;
unsigned long magicThinkStart = 0;
String magicAnswer = "";
const char* magicAnswers[] = {
  "Да!", "Нет", "Возможно", "Точно да", 
  "Не уверен", "Спроси позже", "Ни за что", 
  "100% да", "Не думаю", "Определённо"
};

int tamaActionCursor = 0;
const char* tamaActions[] = { "Кормить", "Играть", "Лечить", "Убрать", "Спать", "Сброс" };
const int tamaActionsCount = 6;

unsigned long lastActivityTime = 0;
unsigned long lastSaveTime = 0;
bool screenOff = false;

// === ПРОТОТИПЫ ===
void handleMenu();
void drawMenu();
void drawStatusBar(String title);
void newMsg(FB_msg& msg);
void prepareMsg(String text);
void handleKeyboard();
void handleWiFiScanner();
void handleGame();
void handleMagicCat();
void handleLoveCounter();
void handleTamagotchi();
void handleTamaActions();
void connectToWiFi(String ssid, String pass);
int utf8Len(String str);
String getUtf8Char(String str, int idx);
int getBatteryLevel();
void updateTamagotchi();
void saveTamagotchi();
void loadTamagotchi();
void resetTamagotchi();
const unsigned char* getCatSprite();
int getDaysTogether();
void resetActivity();
void checkScreenTimeout();

String getTime() {
  time_t now = time(nullptr);
  if (now < 100000) return "--:--";  // Время не синхронизировано
  
  struct tm* timeinfo = localtime(&now);
  char buffer[6];
  sprintf(buffer, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
  return String(buffer);
}


// === SETUP ===
void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BAT_PIN, INPUT);
  digitalWrite(LED_PIN, LOW);

  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setContrast(64);
  
  btn.setType(HIGH_PULL);
  btn.setDirection(NORM_OPEN);
  btn.setDebounce(50);
  btn.setTimeout(300);
  btn.setClickTimeout(300);

  EEPROM.begin(EEPROM_SIZE);
  loadTamagotchi();

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  if (WiFi.SSID() == "") {
     WiFi.begin(WIFI_SSID_DEFAULT, WIFI_PASS_DEFAULT);
  } else {
     WiFi.begin();
  }
  
  configTime(GMT_OFFSET, DAYLIGHT_OFFSET, NTP_SERVER);
  
  bot.setChatID(MY_CHAT_ID);
  bot.setLimit(60);
  bot.attach(newMsg);
  
  resetActivity();
}

// === LOOP ===
void loop() {
  btn.tick();
  
  // === bot.tick() ВСЕГДА (даже во сне) ===
  if (currentMode != MODE_GAME && currentMode != MODE_MAGIC_CAT) {
    bot.tick();
  }
  
  // Wi-Fi реконнект
  if (millis() - wifiTimer > 60000) {
    if (WiFi.status() != WL_CONNECTED && currentMode != MODE_WIFI && currentMode != MODE_WIFI_PASS) {
       WiFi.reconnect();
    }
    wifiTimer = millis();
  }

  // Обновление тамагочи
  static unsigned long lastTamaUpdate = 0;
  if (millis() - lastTamaUpdate > 30000) {
    updateTamagotchi();
    lastTamaUpdate = millis();
  }
  
  // Проверка таймаута экрана
  checkScreenTimeout();
  
  // === ЕСЛИ ЭКРАН СПИТ - НЕ РИСУЕМ, НО ОБРАБАТЫВАЕМ КНОПКУ ===
  if (screenOff) {
    if (btn.isPress() || btn.isClick() || btn.isHolded()) {
      resetActivity();  // Будим экран
    }
    delay(50);
    return; // Выходим БЕЗ отрисовки
  }

  // === ДАЛЬШЕ ТОЛЬКО ЕСЛИ ЭКРАН ВКЛЮЧЕН ===
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
        resetActivity();
      }
      if (btn.isHolded()) {
        tone(BUZZER_PIN, 500, 100);
        hasUnreadMsg = false;
        digitalWrite(LED_PIN, LOW);
        currentMode = MENU_MAIN;
        resetActivity();
      }
      break;

    case MODE_WRITE:
    case MODE_WIFI_PASS:
      handleKeyboard();
      break;

    case MODE_GAME:
      handleGame();
      break;
      
    case MODE_WIFI:
      handleWiFiScanner();
      break;
      
    case MODE_MAGIC_CAT:
      handleMagicCat();
      break;
      
    case MODE_LOVE_COUNTER:
      handleLoveCounter();
      break;
      
    case MODE_TAMAGOTCHI:
      handleTamagotchi();
      break;
      
    case MODE_TAMA_ACTIONS:
      handleTamaActions();
      break;
  }
}

// === ФУНКЦИИ ===

void resetActivity() {
  lastActivityTime = millis();
  if (screenOff) {
    u8g2.setPowerSave(0);
    screenOff = false;
  }
}

void checkScreenTimeout() {
  if (!screenOff && millis() - lastActivityTime > SCREEN_TIMEOUT) {
    u8g2.setPowerSave(1);
    screenOff = true;
    digitalWrite(LED_PIN, LOW);
  }
}

void loadTamagotchi() {
  EEPROM.get(TAMA_ADDR, pet);
  
  if (pet.magicByte != MAGIC_BYTE || pet.birthTime == 0 || pet.birthTime > millis()) {
    resetTamagotchi();
  }
}

void saveTamagotchi() {
  pet.magicByte = MAGIC_BYTE;
  EEPROM.put(TAMA_ADDR, pet);
  EEPROM.commit();
}

void resetTamagotchi() {
  pet.magicByte = MAGIC_BYTE;
  pet.hunger = 50;
  pet.happiness = 80;
  pet.health = 100;
  pet.sleepiness = 30;
  pet.age = 0;
  pet.poop = false;
  pet.lastFeed = millis();
  pet.lastPlay = millis();
  pet.lastClean = millis();
  pet.lastSleep = millis();
  pet.birthTime = millis();
  pet.isDead = false;
  saveTamagotchi();
}

void updateTamagotchi() {
  if (pet.isDead) return;
  
  unsigned long now = millis();
  bool changed = false;
  
  if (now - pet.lastFeed > 7200000UL) {
    pet.hunger = constrain(pet.hunger + 10, 0, 100);
    pet.lastFeed = now;
    changed = true;
  }
  
  if (now - pet.lastPlay > 14400000UL) {
    pet.happiness = constrain(pet.happiness - 15, 0, 100);
    pet.lastPlay = now;
    changed = true;
  }
  
  if (now - pet.lastSleep > 10800000UL) {
    pet.sleepiness = constrain(pet.sleepiness + 20, 0, 100);
    pet.lastSleep = now;
    changed = true;
  }
  
  if (pet.hunger > 80) {
    pet.health = constrain(pet.health - 5, 0, 100);
    changed = true;
  }
  
  if (pet.hunger >= 100 && (now - pet.lastFeed > 259200000UL)) {
    pet.isDead = true;
    tone(BUZZER_PIN, 100, 1000);
    changed = true;
  }
  
  if (!pet.poop && random(100) < 3) {
    pet.poop = true;
    changed = true;
  }
  
  int newAge = (now - pet.birthTime) / 86400000UL;
  if (newAge != pet.age) {
    pet.age = newAge;
    changed = true;
  }
  
  if (currentMode == MODE_TAMAGOTCHI || currentMode == MODE_TAMA_ACTIONS) {
    if (pet.hunger > 70 || pet.health < 30) {
      digitalWrite(LED_PIN, (millis()/1000) % 2);
    } else {
      digitalWrite(LED_PIN, LOW);
    }
  }
  
  if (changed && (now - lastSaveTime > 300000UL)) {
    saveTamagotchi();
    lastSaveTime = now;
  }
}

const unsigned char* getCatSprite() {
  if (pet.isDead) return epd_bitmap_cat_dead;
  if (pet.sleepiness > 80) return epd_bitmap_cat_sleep;
  if (pet.hunger > 70) return epd_bitmap_cat_sad;
  if (pet.health < 30) return epd_bitmap_cat_angry;
  if (pet.happiness > 80) return epd_bitmap_cat_funny;
  return epd_bitmap_cat;
}

void newMsg(FB_msg& msg) {
  if (msg.chatID == MY_CHAT_ID) {
    lastMessage = msg.text;
    prepareMsg(lastMessage);
    hasUnreadMsg = true;
    
    // === БУДИМ ЭКРАН ПРИ СООБЩЕНИИ ===
    resetActivity();
    
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_PIN, HIGH);
      tone(BUZZER_PIN, 2000, 100);
      delay(150);
      digitalWrite(LED_PIN, LOW);
      delay(150);
    }
    digitalWrite(LED_PIN, HIGH);
  }
}


void prepareMsg(String text) {
  msgLines.clear();
  
  const int MAX_CHARS = 16; // ~16 символов на строку для u8g2_font_cu12_t_cyrillic
  
  String currentLine = "";
  int wordStart = 0;
  
  for (int i = 0; i <= text.length(); i++) {
    char c = (i < text.length()) ? text[i] : ' ';
    
    // Если пробел, перенос или конец текста
    if (c == ' ' || c == '\n' || i == text.length()) {
      // Извлекаем слово
      String word = text.substring(wordStart, i);
      wordStart = i + 1;
      
      if (c == '\n') {
        // Принудительный перенос
        if (currentLine.length() > 0) {
          msgLines.push_back(currentLine);
          currentLine = "";
        }
        if (word.length() > 0) {
          msgLines.push_back(word);
        }
        continue;
      }
      
      if (word.length() == 0) continue; // Пропускаем пустые
      
      // Пробуем добавить слово к текущей строке
      String testLine = currentLine;
      if (testLine.length() > 0) testLine += " ";
      testLine += word;
      
      // Подсчитываем видимые символы (UTF-8 учет)
      int visibleChars = 0;
      for (int j = 0; j < testLine.length(); j++) {
        if ((testLine[j] & 0xC0) != 0x80) { // Не продолжение UTF-8
          visibleChars++;
        }
      }
      
      if (visibleChars <= MAX_CHARS) {
        // Влезает - добавляем
        currentLine = testLine;
      } else {
        // Не влезает
        if (currentLine.length() > 0) {
          // Сохраняем текущую строку
          msgLines.push_back(currentLine);
        }
        
        // Проверяем влезает ли само слово
        int wordChars = 0;
        for (int j = 0; j < word.length(); j++) {
          if ((word[j] & 0xC0) != 0x80) wordChars++;
        }
        
        if (wordChars <= MAX_CHARS) {
          // Слово влезает - начинаем новую строку
          currentLine = word;
        } else {
          // Слово слишком длинное - режем по символам
          String chunk = "";
          int chunkChars = 0;
          for (int j = 0; j < word.length(); j++) {
            chunk += word[j];
            if ((word[j] & 0xC0) != 0x80) chunkChars++;
            
            if (chunkChars >= MAX_CHARS) {
              msgLines.push_back(chunk);
              chunk = "";
              chunkChars = 0;
            }
          }
          currentLine = chunk;
        }
      }
    }
  }
  
  // Добавляем последнюю строку
  if (currentLine.length() > 0) {
    msgLines.push_back(currentLine);
  }
}




int getBatteryLevel() {
  int raw = analogRead(BAT_PIN);
  int pct = map(raw, BAT_MIN, BAT_MAX, 0, 100);
  return constrain(pct, 0, 100);
}

void drawStatusBar(String title) {
  u8g2.clearBuffer();
  u8g2.drawLine(0, 10, 128, 10);
  
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.setCursor(2, 8);
  u8g2.print(title);
  
  u8g2.setFont(u8g2_font_open_iconic_embedded_1x_t);
  if (WiFi.status() == WL_CONNECTED) {
    u8g2.drawGlyph(80, 9, 80);
  } else {
     u8g2.setFont(u8g2_font_5x8_tr);
     u8g2.drawStr(80, 8, "x");
  }

  // Время справа
  u8g2.setFont(u8g2_font_5x8_tr);
  String currentTime = getTime();
  u8g2.setCursor(96, 8);
  u8g2.print(currentTime);
}

void handleMenu() {
  if (btn.isClick()) {
    menuCursor++;
    if (menuCursor >= menuCount) menuCursor = 0;
    tone(BUZZER_PIN, 1000, 30);
    digitalWrite(LED_PIN, HIGH);
    delay(30);
    digitalWrite(LED_PIN, LOW);
    resetActivity();
  }
  if (btn.isHolded()) {
    tone(BUZZER_PIN, 1500, 100);
    resetActivity();
    switch (menuCursor) {
      case 0: currentMode = MODE_READ; break;
      case 1: currentMode = MODE_WRITE; inputBuffer = ""; kbGroupIdx = 0; break;
      case 2: currentMode = MODE_WIFI; wifiScanState = 0; break;
      case 3: currentMode = MODE_GAME; gameStarted = false; break;
      case 4: 
        currentMode = MODE_MAGIC_CAT; 
        magicCatState = 0; 
        magicThinkStart = 0;
        break;
      case 5: currentMode = MODE_LOVE_COUNTER; break;
      case 6: currentMode = MODE_TAMAGOTCHI; break;
    }
  }
}

void drawMenu() {
  drawStatusBar("Main Menu");
  u8g2.setFont(u8g2_font_6x10_tr);
  
  int startIdx = max(0, menuCursor - 2);
  for (int i = 0; i < 5 && startIdx + i < menuCount; i++) {
    int y = 25 + (i * 10);
    if (startIdx + i == menuCursor) u8g2.drawStr(5, y, ">");
    u8g2.drawStr(15, y, menuItems[startIdx + i]);
  }
  
  if (hasUnreadMsg) {
     u8g2.drawDisc(100, 22, 3);
     u8g2.setFont(u8g2_font_5x8_tr);
     u8g2.drawStr(106, 25, "New!");
  }
  
  if (pet.hunger > 70 || pet.health < 30) {
    u8g2.drawStr(100, 60, "!");
  }
  
  u8g2.sendBuffer();
}

void handleMagicCat() {
  u8g2.clearBuffer();
  drawStatusBar("Magic Cat");
  
  if (magicCatState == 0) {
    // Текст слева
    u8g2.setFont(u8g2_font_cu12_t_cyrillic);
    u8g2.setCursor(5, 23);
    u8g2.print("Задумай");
    u8g2.setCursor(5, 36);
    u8g2.print("вопрос");
    
    // Котик справа
    u8g2.drawXBMP(95, 15, 19, 20, epd_bitmap_cat);
    
    // Подсказка внизу
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.setCursor(15, 60);
    u8g2.print("Hold=");
    u8g2.setFont(u8g2_font_cu12_t_cyrillic);
    u8g2.print("Гадать");
    
    u8g2.sendBuffer();
    
    if (btn.isHolded()) {
      magicCatState = 1;
      magicThinkStart = millis();
      tone(BUZZER_PIN, 1000, 100);
      resetActivity();
    }
    if (btn.isClick()) {
      currentMode = MENU_MAIN;
      tone(BUZZER_PIN, 800, 50);
      resetActivity();
    }
  } 
  else if (magicCatState == 1) {
    u8g2.setFont(u8g2_font_cu12_t_cyrillic);
    u8g2.setCursor(30, 35);
    u8g2.print("Думаю");
    for (int i = 0; i < (millis()/300) % 4; i++) {
      u8g2.print(".");
    }
    u8g2.sendBuffer();
    
    int brightness = (sin(millis() / 100.0) + 1) * 512;
    analogWrite(LED_PIN, constrain(brightness, 0, 1023));
    
    if (millis() - magicThinkStart > 3000) {
      magicAnswer = magicAnswers[random(10)];
      magicCatState = 2;
      digitalWrite(LED_PIN, LOW);
      tone(BUZZER_PIN, 1000, 200);
      delay(100);
      tone(BUZZER_PIN, 1500, 200);
      resetActivity();
    }
  }
  else if (magicCatState == 2) {
    u8g2.setFont(u8g2_font_unifont_t_cyrillic);
    int w = u8g2.getStrWidth(magicAnswer.c_str());
    u8g2.setCursor((128 - w) / 2, 35);
    u8g2.print(magicAnswer);
    
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.setCursor(25, 55);
    u8g2.print("Click=Again");
    u8g2.sendBuffer();
    
    if (btn.isClick()) {
      magicCatState = 0;
      magicThinkStart = 0;
      tone(BUZZER_PIN, 1200, 50);
      resetActivity();
    }
    if (btn.isHolded()) {
      currentMode = MENU_MAIN;
      tone(BUZZER_PIN, 800, 50);
      resetActivity();
    }
  }
}

int getDaysTogether() {
  time_t now = time(nullptr);
  if (now < 100000) return 0;
  long days = (now - LOVE_START_DATE) / 86400L;
  return days > 0 ? days : 0;
}

void handleLoveCounter() {
  drawStatusBar("Love Days");
  
  int days = getDaysTogether();
  int years = days / 365;
  int months = (days % 365) / 30;
  int remainDays = days % 30;
  
  u8g2.setFont(u8g2_font_unifont_t_cyrillic);
  u8g2.setCursor(25, 25);
  u8g2.print("Мы вместе:");
  
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.setCursor(30, 45);
  u8g2.print(String(days));
  
  // === ИСПОЛЬЗУЕМ КИРИЛЛИЧЕСКИЙ ШРИФТ! ===
  u8g2.setFont(u8g2_font_6x13_t_cyrillic);
  u8g2.setCursor(10, 60);
  if (years > 0) {
    u8g2.print(String(years) + " г. " + String(months) + " м. " + String(remainDays) + " д.");
  } else if (months > 0) {
    u8g2.print(String(months) + " м. " + String(remainDays) + " д.");
  } else {
    u8g2.print(String(days) + " дней");
  }
  
  if (days % 100 == 0 || days % 365 == 0) {
    digitalWrite(LED_PIN, (millis()/500) % 2);
  }
  
  u8g2.sendBuffer();
  
  if (btn.isClick() || btn.isHolded()) {
    currentMode = MENU_MAIN;
    digitalWrite(LED_PIN, LOW);
    tone(BUZZER_PIN, 800, 50);
    resetActivity();
  }
}

// === TAMAGOTCHI - ЧИСТЫЙ LAYOUT ===
void handleTamagotchi() {
  u8g2.clearBuffer();
  drawStatusBar("Tamagotchi");
  
  // Статы компактно
  u8g2.setFont(u8g2_font_5x8_tr);
  
  // Левая колонка
  u8g2.setCursor(2, 25);
  u8g2.print("Hun:" + String(pet.hunger));
  u8g2.setCursor(2, 35);
  u8g2.print("Hap:" + String(pet.happiness));
  u8g2.setCursor(2, 45);
  u8g2.print("HP:" + String(pet.health));
  
  // Котик В ЦЕНТРЕ
  const unsigned char* sprite = getCatSprite();
  u8g2.drawXBMP(54, 25, 19, 20, sprite);
  
  // Правая колонка
  u8g2.setCursor(95, 25);
  u8g2.print("Slp:" + String(pet.sleepiness));
  u8g2.setCursor(95, 35);
  u8g2.print("Age:" + String(pet.age) + "d");
  
  // Какашка если есть (под котом)
  if (pet.poop) {
    u8g2.drawXBMP(60, 48, 7, 7, epd_bitmap_poop);
  }
  
  // DEAD надпись если умер
  if (pet.isDead) {
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.setCursor(45, 60);
    u8g2.print("DEAD");
  } else {
    // Подсказка внизу
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.setCursor(20, 60);
    u8g2.print("Click=Actions");
  }
  
  u8g2.sendBuffer();
  
  if (btn.isClick()) {
    currentMode = MODE_TAMA_ACTIONS;
    tamaActionCursor = 0;
    tone(BUZZER_PIN, 1200, 50);
    resetActivity();
  }
  if (btn.isHolded()) {
    currentMode = MENU_MAIN;
    digitalWrite(LED_PIN, LOW);
    tone(BUZZER_PIN, 800, 50);
    resetActivity();
  }
}


void handleTamaActions() {
  u8g2.clearBuffer();
  drawStatusBar("Actions");
  
  u8g2.setFont(u8g2_font_cu12_t_cyrillic);
  
  // === ПОКАЗЫВАЕМ 4 ПУНКТА ВМЕСТО 5 ===
  int startIdx = max(0, tamaActionCursor - 1);  // Центрируем текущий пункт
  if (startIdx > 2) startIdx = 2;  // Чтобы последние пункты были видны
  
  for (int i = 0; i < 4 && startIdx + i < 6; i++) {
    int y = 22 + (i * 14);  // ОТСТУП 12px ВМЕСТО 10px
    int idx = startIdx + i;
    
    if (idx == tamaActionCursor) {
      u8g2.setCursor(2, y);
      u8g2.print(">");
    }
    
    u8g2.setCursor(12, y);
    switch(idx) {
      case 0: u8g2.print("Кормить"); break;
      case 1: u8g2.print("Играть"); break;
      case 2: u8g2.print("Лечить"); break;
      case 3: u8g2.print("Убрать"); break;
      case 4: u8g2.print("Спать"); break;
      case 5: u8g2.print("Сброс"); break;
    }
  }
  
  u8g2.sendBuffer();
  
  // Остальное без изменений...

  
  if (btn.isClick()) {
    tamaActionCursor++;
    if (tamaActionCursor >= 6) tamaActionCursor = 0;
    tone(BUZZER_PIN, 1000, 30);
    resetActivity();
  }
  
  if (btn.isHolded()) {
    tone(BUZZER_PIN, 1500, 100);
    resetActivity();
    
    // === ПОДТВЕРЖДЕНИЕ СБРОСА ===
    if (tamaActionCursor == 5) {
      u8g2.clearBuffer();
      drawStatusBar("Confirm?");
      u8g2.setFont(u8g2_font_cu12_t_cyrillic);
      u8g2.setCursor(15, 35);
      u8g2.print("Сбросить?");
      u8g2.setFont(u8g2_font_5x8_tr);
      u8g2.setCursor(10, 55);
      u8g2.print("Hold=YES Click=NO");
      u8g2.sendBuffer();
      
      delay(500); // Задержка чтобы не сработало сразу
      unsigned long waitStart = millis();
      bool confirmed = false;
      
      while(millis() - waitStart < 5000) { // 5 секунд на ответ
        btn.tick();
        
        if (btn.isHolded()) {
          confirmed = true;
          break;
        }
        if (btn.isClick()) {
          tone(BUZZER_PIN, 800, 100);
          currentMode = MODE_TAMAGOTCHI;
          return;
        }
        delay(10);
      }
      
      if (confirmed) {
        u8g2.clearBuffer();
        drawStatusBar("Reset");
        u8g2.setFont(u8g2_font_ncenB10_tr);
        u8g2.drawStr(30, 40, "RESET!");
        u8g2.sendBuffer();
        
        tone(BUZZER_PIN, 500, 300);
        delay(400);
        tone(BUZZER_PIN, 400, 300);
        delay(400);
        tone(BUZZER_PIN, 300, 500);
        delay(600);
        
        resetTamagotchi();
      }
      
      currentMode = MODE_TAMAGOTCHI;
      return;
    }
    
    // === АНИМАЦИИ С ПРАВИЛЬНЫМ LAYOUT ===
    u8g2.clearBuffer();
    drawStatusBar("Actions");
    
    const unsigned char* sprite = getCatSprite();
    
    switch (tamaActionCursor) {
      case 0: // Кормить
        if (!pet.isDead) {
          // Кот слева, рыбка справа
          u8g2.drawXBMP(35, 18, 19, 20, sprite);
          u8g2.drawXBMP(70, 22, 10, 8, epd_bitmap_fish);
          
          // Текст внизу
          u8g2.setFont(u8g2_font_cu12_t_cyrillic);
          u8g2.setCursor(30, 50);
          u8g2.print("Ням-ням!");
          u8g2.sendBuffer();
          
          for (int i = 0; i < 3; i++) {
            tone(BUZZER_PIN, 2000, 100);
            delay(300);
            tone(BUZZER_PIN, 2200, 100);
            delay(300);
          }
          
          pet.hunger = constrain(pet.hunger - 30, 0, 100);
          pet.lastFeed = millis();
          saveTamagotchi();
        }
        break;
        
      case 1: // Играть
        if (!pet.isDead) {
          // Кот сверху
          u8g2.drawXBMP(54, 15, 19, 20, sprite);
          
          // Геймпад под котом
          u8g2.drawXBMP(52, 38, 23, 12, epd_bitmap_gamepad);
          
          // Текст внизу
          u8g2.setFont(u8g2_font_cu12_t_cyrillic);
          u8g2.setCursor(35, 60);
          u8g2.print("Играем!");
          u8g2.sendBuffer();
          
          for (int i = 0; i < 4; i++) {
            tone(BUZZER_PIN, 1500, 150);
            delay(200);
            tone(BUZZER_PIN, 1800, 150);
            delay(200);
          }
          
          pet.happiness = constrain(pet.happiness + 20, 0, 100);
          pet.lastPlay = millis();
          saveTamagotchi();
        }
        break;
        
      case 2: // Лечить
        if (!pet.isDead) {
          // Кот слева, плюс справа
          u8g2.drawXBMP(35, 18, 19, 20, sprite);
          u8g2.setFont(u8g2_font_ncenB18_tr);
          u8g2.drawStr(70, 32, "+");
          
          // Текст внизу
          u8g2.setFont(u8g2_font_cu12_t_cyrillic);
          u8g2.setCursor(35, 55);
          u8g2.print("Лечим!");
          u8g2.sendBuffer();
          
          for (int i = 0; i < 3; i++) {
            tone(BUZZER_PIN, 2500, 200);
            delay(400);
          }
          
          pet.health = constrain(pet.health + 30, 0, 100);
          saveTamagotchi();
        }
        break;
        
      case 3: // Убрать
        if (pet.poop) {
          // Кот слева, туалетная бумага справа
          u8g2.drawXBMP(30, 18, 19, 20, sprite);
          u8g2.drawXBMP(70, 18, 22, 19, epd_bitmap_toilet_paper);
          
          // Текст внизу
          u8g2.setFont(u8g2_font_cu12_t_cyrillic);
          u8g2.setCursor(25, 55);
          u8g2.print("Убираем!");
          u8g2.sendBuffer();
          
          tone(BUZZER_PIN, 1000, 100);
          delay(500);
          tone(BUZZER_PIN, 1200, 100);
          delay(500);
          
          pet.poop = false;
          pet.lastClean = millis();
          saveTamagotchi();
        }
        break;
        
      case 4: // Спать
        if (!pet.isDead) {
          // Кот слева, плюс справа
          u8g2.drawXBMP(35, 18, 19, 20, sprite);
          // Z Z Z справа о т кота
          u8g2.setFont(u8g2_font_ncenB14_tr);
          u8g2.drawStr(58, 36, "Z");
          u8g2.drawStr(70, 32, "Z");
          u8g2.drawStr(82, 28, "Z");

          // Текст внизу
          u8g2.setFont(u8g2_font_cu12_t_cyrillic);
          u8g2.setCursor(35, 55);
          u8g2.print("Сплю...");
          u8g2.sendBuffer();
          
          for (int i = 0; i < 3; i++) {
            tone(BUZZER_PIN, 800, 400);
            delay(600);
          }
          
          pet.sleepiness = 0;
          pet.lastSleep = millis();
          saveTamagotchi();
        }
        break;
    }
    
    currentMode = MODE_TAMAGOTCHI;
  }
}




void connectToWiFi(String ssid, String pass) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(10, 30, "Connecting...");
    u8g2.setCursor(10, 45);
    u8g2.print(ssid);
    u8g2.sendBuffer();

    WiFi.begin(ssid, pass);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }

    u8g2.clearBuffer();
    if (WiFi.status() == WL_CONNECTED) {
        u8g2.drawStr(10, 30, "Success!");
        tone(BUZZER_PIN, 2000, 200);
    } else {
        u8g2.drawStr(10, 30, "Failed!");
        tone(BUZZER_PIN, 200, 500);
    }
    u8g2.sendBuffer();
    delay(2000);
    currentMode = MENU_MAIN;
    resetActivity();
}

void handleWiFiScanner() {
  if (wifiScanState == 0) {
    WiFi.scanNetworks(true);
    wifiScanState = 1;
    wifiScroll = 0;
  }
  
  drawStatusBar("Scan Networks");
  
  if (wifiScanState == 1) {
    int n = WiFi.scanComplete();
    if (n == -2) WiFi.scanNetworks(true);
    else if (n == -1) {
       u8g2.setFont(u8g2_font_6x10_tr);
       u8g2.drawStr(10, 30, "Scanning...");
    } else {
       wifiScanState = 2;
    }
  }
  
  if (wifiScanState == 2) {
    int n = WiFi.scanComplete();
    if (n == 0) {
      u8g2.drawStr(10, 30, "No networks");
    } else {
      u8g2.setFont(u8g2_font_5x8_tr);
      for (int i = 0; i < 5; i++) {
        int idx = wifiScroll + i;
        if (idx < n) {
          int y = 22 + (i * 9);
          String ssid = WiFi.SSID(idx);
          if (ssid.length() > 14) ssid = ssid.substring(0, 14) + "..";
          String lock = (WiFi.encryptionType(idx) == ENC_TYPE_NONE) ? "" : "*";
          
          if (i == 0) u8g2.drawStr(0, y, ">");
          
          u8g2.setCursor(8, y);
          u8g2.print(ssid + lock);
          u8g2.setCursor(110, y);
          u8g2.print(WiFi.RSSI(idx));
        }
      }
    }

    if (btn.isClick()) {
      wifiScroll++;
      if (wifiScroll >= n) wifiScroll = 0;
      tone(BUZZER_PIN, 1000, 30);
      resetActivity();
    }
    
    if (btn.isHolded()) {
        if (n > 0) {
            targetSSID = WiFi.SSID(wifiScroll);
            WiFi.scanDelete();
            inputBuffer = "";
            currentMode = MODE_WIFI_PASS;
            tone(BUZZER_PIN, 1500, 100);
            resetActivity();
        }
    }
  }
  u8g2.sendBuffer();
}

void handleKeyboard() {
  String status = (currentMode == MODE_WRITE) ? "Write Msg" : ("Pass: " + targetSSID);
  status += (isRusLayout ? " [RU]" : " [EN]");
  drawStatusBar(status);
  
  u8g2.setFont(u8g2_font_unifont_t_cyrillic);
  u8g2.setCursor(0, 25);
  u8g2.print(inputBuffer + "_");
  u8g2.drawLine(0, 27, 128, 27);

  const char** currentGroups = isRusLayout ? kbGroupsRU : kbGroupsEN;
  int currentCount = 10;
  if (kbGroupIdx >= currentCount) kbGroupIdx = 0;

  if (!isInsideGroup) {
    String grpName = currentGroups[kbGroupIdx];
    
    if (grpName == "<") grpName = "[ DELETE ]";
    if (grpName == "RU") grpName = "[ LANG: RU ]";
    if (grpName == "EN") grpName = "[ LANG: EN ]";
    if (grpName == "SEND") grpName = "[ SEND >> ]";
    if (grpName == "EXIT") grpName = "[ << MENU ]";

    u8g2.setFont(u8g2_font_unifont_t_cyrillic);
    
    int boxW = 0;
    int boxX = 0;
    int textX = 0;
    
    if (grpName.charAt(0) != '[' && grpName != "<" && grpName != "RU" && grpName != "EN" && grpName != "EXIT") {
        boxW = 80;
        boxX = (128 - boxW) / 2;
        int w = u8g2.getStrWidth(grpName.c_str());
        if (w == 0) w = 40;
        textX = (128 - w) / 2;
    } else {
        int w = u8g2.getStrWidth(grpName.c_str());
        boxW = w + 8;
        boxX = (128 - boxW) / 2;
        textX = (128 - w) / 2;
    }
    
    if (textX < 0) textX = 0;
    if (boxX < 0) boxX = 0;
    
    u8g2.setCursor(textX, 50);
    u8g2.print(grpName);
    u8g2.drawFrame(boxX, 36, boxW, 18);

    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.setCursor(0, 63);
    u8g2.print("Click:Next Hold:In");
    
    if (btn.isClick()) {
       kbGroupIdx++;
       if (kbGroupIdx >= currentCount) kbGroupIdx = 0;
       tone(BUZZER_PIN, 1000, 30);
       resetActivity();
    }

    if (btn.isHolded()) {
       String rawGrp = String(currentGroups[kbGroupIdx]);
       resetActivity();
       
       if (rawGrp == "RU" || rawGrp == "EN") {
           isRusLayout = !isRusLayout;
           kbGroupIdx = 0;
           tone(BUZZER_PIN, 2000, 100);
       }
       else if (rawGrp == "EXIT") {
           currentMode = MENU_MAIN;
           inputBuffer = "";
           tone(BUZZER_PIN, 500, 100);
       }
       else if (rawGrp == "<") {
           if (inputBuffer.length() > 0) {
               char last = inputBuffer.charAt(inputBuffer.length()-1);
               inputBuffer.remove(inputBuffer.length()-1);
               if ((uint8_t)last >= 0x80 && inputBuffer.length() > 0) inputBuffer.remove(inputBuffer.length()-1);
               tone(BUZZER_PIN, 500, 50); delay(50); tone(BUZZER_PIN, 300, 50);
           }
       }
       else if (rawGrp == "SEND") {
           if (currentMode == MODE_WRITE) {
               if (inputBuffer.length() > 0) {
                  bot.sendMessage(inputBuffer);
                  inputBuffer = "";
                  currentMode = MENU_MAIN;
                  tone(BUZZER_PIN, 2000, 300);
               }
           } else if (currentMode == MODE_WIFI_PASS) {
               connectToWiFi(targetSSID, inputBuffer);
           }
       } 
       else {
           isInsideGroup = true;
           kbCharIdx = 0;
           tone(BUZZER_PIN, 1500, 100);
       }
    }
  } else {
    String group = currentGroups[kbGroupIdx];
    String currentC = getUtf8Char(group, kbCharIdx);
    
    u8g2.setFont(u8g2_font_unifont_t_cyrillic);
    u8g2.drawFrame(54, 35, 20, 20);
    u8g2.setCursor(58, 51);
    u8g2.print(currentC);
    
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.setCursor(0, 63);
    u8g2.print(String(kbCharIdx+1) + "/" + String(utf8Len(group)));
    
    if (btn.isClick()) {
       kbCharIdx++;
       if (kbCharIdx >= utf8Len(group)) kbCharIdx = 0;
       tone(BUZZER_PIN, 1200, 30);
       resetActivity();
    }
    
    if (btn.isHolded()) {
       inputBuffer += getUtf8Char(group, kbCharIdx);
       isInsideGroup = false;
       tone(BUZZER_PIN, 1800, 100);
       resetActivity();
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

String getUtf8Char(String str, int idx) {
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

  if (!gameStarted) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_ncenB14_tr);
      u8g2.drawStr(25, 30, "Flappy");
      u8g2.setFont(u8g2_font_5x8_tr);
      u8g2.drawStr(30, 50, "Click to Start");
      u8g2.sendBuffer();
      
      if (btn.isClick()) {
        gameStarted = true;
        birdY = 32;
        birdVel = -1.5;
        wallX = 128;
        score = 0;
        lastGameFrame = millis();
        tone(BUZZER_PIN, 1000, 100);
        resetActivity();
      }
      if (btn.isHolded()) {
        currentMode = MENU_MAIN;
        resetActivity();
      }
      return;
  }

  if (btn.isClick()) {
      birdVel = -3.0;
      tone(BUZZER_PIN, 600, 20);
      resetActivity();
  }

  if (millis() - lastGameFrame > 30) {
    lastGameFrame = millis();
    birdVel += 0.4;
    birdY += birdVel;
    wallX -= 3;
    if (wallX < -10) {
       wallX = 128;
       gapY = random(10, 40);
       score++;
       tone(BUZZER_PIN, 2000, 50);
    }
  }

  bool collision = false;
  if (birdY > 62 || birdY < 0) collision = true;
  if (wallX < 24 && wallX > 6) {
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
      resetActivity();
      return;
  }

  u8g2.clearBuffer();
  u8g2.drawDisc(20, (int)birdY, 3);
  u8g2.drawBox(wallX, 0, 12, gapY);
  u8g2.drawBox(wallX, gapY + gapSize, 12, 64 - (gapY + gapSize));
  u8g2.drawFrame(wallX, 0, 12, gapY);
  u8g2.drawFrame(wallX, gapY + gapSize, 12, 64 - (gapY + gapSize));
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.setCursor(100, 15);
  u8g2.print(score);
  u8g2.drawLine(0, 63, 128, 63);
  u8g2.sendBuffer();
  
  if (btn.isHolded()) {
      gameStarted = false;
      currentMode = MENU_MAIN;
      resetActivity();
  }
}
