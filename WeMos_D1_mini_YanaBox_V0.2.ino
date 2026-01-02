#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <GyverButton.h>
#include <FastBot.h>
#include <ESP8266WiFi.h>
#include <vector>

// === КОНФИГ ===
#define WIFI_SSID_DEFAULT "TP-Link_2163"
#define WIFI_PASS_DEFAULT ""

#define BOT_TOKEN ""
#define MY_CHAT_ID "1231597064"

// Настройки батареи
#define BAT_PIN A0
#define BAT_MIN 500  
#define BAT_MAX 900  

// === ПИНЫ ===
#define BUTTON_PIN D7
#define LED_PIN    D6
#define BUZZER_PIN D5

// === ОБЪЕКТЫ ===
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, D1, D2);

GButton btn(BUTTON_PIN);
FastBot bot(BOT_TOKEN);

// === ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ===
enum Mode { MENU_MAIN, MODE_READ, MODE_WRITE, MODE_WIFI, MODE_WIFI_PASS, MODE_GAME };
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

// Клавиатура и Ввод
String inputBuffer = "";
String targetSSID = ""; 
int kbGroupIdx = 0;       
int kbCharIdx = 0;       
bool isInsideGroup = false;

// ... В начало файла, где глобальные переменные ...

// Флаг языка: false = Английский, true = Русский
bool isRusLayout = true; 

// === РАСКЛАДКА EN ===
const char* kbGroupsEN[] = {
  "abcdef", 
  "ghijkl",   
  "mnopqr",   
  "stuvwxyz", 
  "1234567890",
  ".,?!+-_@=",
  "SEND", 
  "<",      // Backspace
  "RU"      // Кнопка смены языка
};

// === РАСКЛАДКА RU (Маленькие буквы!) ===
const char* kbGroupsRU[] = {
  "абвгде", 
  "жзийкл",
  "мнопрст",
  "уфхцчш",
  "щъыьэюя",
  "1234567890",
  ".,?!+-_@=",
  "SEND",
  "<",      // Backspace
  "EN"      // Кнопка смены языка
};

// Вспомогательные функции для текущей раскладки
const char** getKurrLayout() {
  return isRusLayout ? kbGroupsRU : kbGroupsEN;
}

int getKurrLayoutSize() {
  return isRusLayout ? (sizeof(kbGroupsRU)/sizeof(char*)) : (sizeof(kbGroupsEN)/sizeof(char*));
}


// === ИГРА ===
float birdY = 32;       
float birdVel = 0;      
int score = 0;          
int wallX = 128;        
int gapY = 30;          
const int gapSize = 25; 
bool gameStarted = false;

// === WI-FI SCANNER ===
int wifiScanState = 0; 
int wifiScroll = 0;
unsigned long wifiTimer = 0;

// === ПРОТОТИПЫ ===
void handleMenu();
void drawMenu();
void drawStatusBar(String title);
void newMsg(FB_msg& msg);
void prepareMsg(String text);
void handleKeyboard();
void handleWiFiScanner();
void handleGame();
void connectToWiFi(String ssid, String pass);
int utf8Len(String str);
String getErrChar(String str, int idx);
int getBatteryLevel();

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BAT_PIN, INPUT);
  digitalWrite(LED_PIN, LOW);

  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setBusClock(400000); 
  
  btn.setType(HIGH_PULL);
  btn.setDirection(NORM_OPEN);
  btn.setDebounce(50);      
  btn.setTimeout(300);       
  btn.setClickTimeout(300);  

  WiFi.mode(WIFI_STA);
  if (WiFi.SSID() == "") {
     WiFi.begin(WIFI_SSID_DEFAULT, WIFI_PASS_DEFAULT);
  } else {
     WiFi.begin(); 
  }
  
  bot.setChatID(MY_CHAT_ID);
  bot.attach(newMsg);
}

void loop() {
  btn.tick();
  bot.tick();
  
  if (millis() - wifiTimer > 30000) {
    if (WiFi.status() != WL_CONNECTED && currentMode != MODE_WIFI && currentMode != MODE_WIFI_PASS) {
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
    case MODE_WIFI_PASS: 
      handleKeyboard();
      break;

    case MODE_GAME:
      handleGame();
      break;
      
    case MODE_WIFI:
      handleWiFiScanner();
      break;
  }
}

// === ЛОГИКА ===

void newMsg(FB_msg& msg) {
  if (msg.chatID == MY_CHAT_ID) {
    lastMessage = msg.text;
    prepareMsg(lastMessage);
    hasUnreadMsg = true;
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 2000, 100);
    delay(100);
    tone(BUZZER_PIN, 1500, 100);
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

int getBatteryLevel() {
  int raw = analogRead(BAT_PIN);
  // Убрал проверку < 100, теперь просто честно отображает уровень
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
    u8g2.drawGlyph(95, 9, 80); 
  } else {
     u8g2.setFont(u8g2_font_5x8_tr);
     u8g2.drawStr(95, 8, "x");
  }

  // Батарея
  int bat = getBatteryLevel();
  u8g2.drawFrame(108, 0, 16, 8);
  u8g2.drawBox(124, 2, 2, 4);
  int w = map(bat, 0, 100, 0, 14);
  if (w > 0) u8g2.drawBox(110, 2, w, 4);
}

void handleMenu() {
  if (btn.isClick()) {
    menuCursor++;
    if (menuCursor >= menuCount) menuCursor = 0;
    tone(BUZZER_PIN, 1000, 30);
  }
  if (btn.isHolded()) {
    tone(BUZZER_PIN, 1500, 100);
    switch (menuCursor) {
      case 0: currentMode = MODE_READ; break;
      case 1: 
        currentMode = MODE_WRITE; 
        inputBuffer = ""; 
        break;
      case 2: 
        currentMode = MODE_WIFI; 
        wifiScanState = 0; 
        break;
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
  if (hasUnreadMsg) {
     u8g2.drawDisc(100, 22, 3);
     u8g2.setFont(u8g2_font_5x8_tr);
     u8g2.drawStr(106, 25, "New!");
  }
  u8g2.sendBuffer();
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
        u8g2.drawStr(90 + (attempts%3)*4, 30, "."); 
        u8g2.sendBuffer();
    }

    u8g2.clearBuffer();
    if (WiFi.status() == WL_CONNECTED) {
        u8g2.drawStr(10, 30, "Success!");
        u8g2.drawStr(10, 45, "IP Obtained");
        tone(BUZZER_PIN, 2000, 200);
        tone(BUZZER_PIN, 2500, 200);
    } else {
        u8g2.drawStr(10, 30, "Failed!");
        u8g2.drawStr(10, 45, "Wrong Pass?");
        tone(BUZZER_PIN, 200, 500);
    }
    u8g2.sendBuffer();
    delay(2000);
    currentMode = MENU_MAIN;
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
    }
    
    if (btn.isHolded()) {
        if (n > 0) {
            targetSSID = WiFi.SSID(wifiScroll); 
            WiFi.scanDelete(); 
            inputBuffer = ""; 
            currentMode = MODE_WIFI_PASS;
            tone(BUZZER_PIN, 1500, 100);
        }
    }
  }
  u8g2.sendBuffer();
}

void handleKeyboard() {
  String status = (currentMode == MODE_WRITE) ? "Write Msg" : ("Pass: " + targetSSID);
  status += (isRusLayout ? " [RU]" : " [EN]"); 
  drawStatusBar(status);
  
  // Поле ввода
  u8g2.setFont(u8g2_font_unifont_t_cyrillic); 
  u8g2.setCursor(0, 25);
  u8g2.print(inputBuffer + "_");
  u8g2.drawLine(0, 27, 128, 27);

  const char** currentGroups = getKurrLayout();
  int currentCount = getKurrLayoutSize();
  if (kbGroupIdx >= currentCount) kbGroupIdx = 0;

  if (!isInsideGroup) {
    // === ВЫБОР ГРУППЫ ===
    String grpName = currentGroups[kbGroupIdx];
    
    // Красивые названия
    if (grpName == "<") grpName = "[ DELETE ]";
    if (grpName == "RU") grpName = "[ LANG: RU ]";
    if (grpName == "EN") grpName = "[ LANG: EN ]";
    if (grpName == "SEND") grpName = "[ SEND >> ]";

    // --- ЛОГИКА РАМКИ И ЦЕНТРОВКИ ---
    u8g2.setFont(u8g2_font_unifont_t_cyrillic);
    
    int boxW = 0;
    int boxX = 0;
    int textX = 0;
    
    // Если это группа букв (длинная строка, не спецкнопка)
    // "абвгде" - 6 chars, но в UTF-8 это 12 байт.
    // Простой хак: если это НЕ спецкнопка (которая начинается с [) и не <
    if (grpName.charAt(0) != '[' && grpName != "<" && grpName != "RU" && grpName != "EN") {
        // ЭТО ГРУППА БУКВ -> ФИКСИРОВАННАЯ ШИРОКАЯ РАМКА
        boxW = 80;        // Ширина рамки (подбери по вкусу, 80 обычно хватает)
        boxX = (128 - boxW) / 2; // Центр экрана
        
        // Текст тоже центрируем в рамке
        // Для русских букв getStrWidth может врать, поэтому считаем вручную или просто рисуем
        // Но с Unifont обычно работает, если библиотека скомпилирована верно.
        // Если глючит - просто рисуем от boxX + отступ.
        int w = u8g2.getStrWidth(grpName.c_str());
        if (w == 0) w = 40; // Если глюк и вернуло 0 - ставим заглушку
        
        textX = (128 - w) / 2; 
    } else {
        // ЭТО СПЕЦКНОПКА -> АВТО РАМКА (как было, работает норм)
        int w = u8g2.getStrWidth(grpName.c_str());
        boxW = w + 8;
        boxX = (128 - boxW) / 2;
        textX = (128 - w) / 2;
    }
    
    // Рисуем
    if (textX < 0) textX = 0;
    if (boxX < 0) boxX = 0;
    
    u8g2.setCursor(textX, 50);
    u8g2.print(grpName);
    u8g2.drawFrame(boxX, 36, boxW, 18); // Чуть выше рамку (18px)

    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.setCursor(0, 63);
    u8g2.print("Click:Next Hold:In");
    
    // ЛИСТАНИЕ
    if (btn.isClick()) {
       kbGroupIdx++;
       if (kbGroupIdx >= currentCount) kbGroupIdx = 0;
       tone(BUZZER_PIN, 1000, 30);
    }

    // ВХОД
    if (btn.isHolded()) {
       String rawGrp = String(currentGroups[kbGroupIdx]);
       
       if (rawGrp == "RU" || rawGrp == "EN") {
           isRusLayout = !isRusLayout; 
           kbGroupIdx = 0; 
           tone(BUZZER_PIN, 2000, 100);
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
    // ... (Внутри группы без изменений)
    String group = currentGroups[kbGroupIdx];
    String currentC = getErrChar(group, kbCharIdx);
    
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
      }
      if (btn.isHolded()) currentMode = MENU_MAIN;
      return;
  }

  if (btn.isClick()) {
      birdVel = -3.0; 
      tone(BUZZER_PIN, 600, 20);
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
  }
}
