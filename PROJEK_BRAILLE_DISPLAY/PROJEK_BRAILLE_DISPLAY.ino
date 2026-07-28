/**************************************************************************
 * ESP32-S3 Braille Trainer  -  LANDSCAPE 480x320
 * Hardware: ST7796S TFT, HR2046 Touch, MCP23017, MAX98357A I2S
 **************************************************************************/
#include <WiFi.h>
#include <esp_wifi.h>
#if __has_include("esp_eap_client.h")
#include "esp_eap_client.h"
#elif __has_include("esp_wpa2.h")
#include "esp_wpa2.h"
#endif
#include <Wire.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Adafruit_ST7796S.h>
#include <XPT2046_Touchscreen.h>
#include <Adafruit_MCP23X17.h>
#include "Audio.h"

// ---- Forward declarations ----
bool connectWiFi(unsigned long timeoutMs = 20000);
void drawMainScreen();
void drawWiFiScreen();
void drawKeyboard();
void handleButtonPress(int pin);
void refreshMainWiFiStatus();
void eepromLoadSpeed();
bool autoConnectWiFi();
void initAudio();
void checkButtons();
void handleTouch();
void serialMenu();
void eepromClearWiFi();
void eepromSaveWiFi(const String &ssid, const String &pass, const String &user, bool isEnt);
bool eepromLoadWiFi(String &ssid, String &pass, String &user, bool &isEnt);
void eepromSaveSpeed();
void speak(const String &text);
void drawStatusBadge();
void drawInputCardOnly();
void drawOutputCardOnly();
void drawCenteredText(const String &txt, int16_t bx, int16_t by, int16_t bw, int16_t bh, uint16_t color, bool useLargeFont = true);
String getPatternDotsString(uint8_t p);
String evalCalc(const String &expr);
void drawGearIcon(int16_t cx, int16_t cy, int16_t r_out, int16_t r_in, uint16_t color);
void drawWiFiIcon(int16_t cx, int16_t cy, uint16_t color);
void drawWiFiIndicator();

// ============================================================
// EEPROM DEFS
// ============================================================
#define EEPROM_SIZE      200
#define EEPROM_MAGIC_ADDR  0
#define EEPROM_SSID_ADDR   1
#define EEPROM_PASS_ADDR  66
#define EEPROM_USER_ADDR  132
#define EEPROM_ENT_ADDR   197
#define EEPROM_MAX_LEN    64
#define EEPROM_MAGIC_VAL 0xAB
#define EEPROM_SPEED_ADDR 130

// ============================================================
// PIN DEFINITIONS
// ============================================================
#define TFT_CS    15
#define TFT_DC     2
#define TFT_RST    4
#define TFT_SCLK  18
#define TFT_MOSI  11
#define TFT_MISO  13
#define TFT_BL    10
#define TOUCH_CS  14
#define TOUCH_IRQ 12
#define SDA_PIN    8
#define SCL_PIN    9
#define I2S_BCLK   5
#define I2S_LRC    6
#define I2S_DOUT   7

// ============================================================
// OBJECTS
// ============================================================
Adafruit_ST7796S tft(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);
Adafruit_MCP23X17 mcp1;
Audio audio;

// ============================================================
// COLORS (RGB565)
// ============================================================
#define C_BG      0x0841
#define C_SURFACE 0x1082
#define C_BORDER  0x2965
#define C_PRIMARY 0x0278
#define C_ACCENT  0x07FF
#define C_SUCCESS 0x07E0
#define C_ERROR   0xF800
#define C_WARNING 0xFFE0
#define C_WHITE   0xFFFF
#define C_LGRAY   0xC618
#define C_GRAY    0x7BEF
#define C_DGRAY   0x39E7
#define C_KEY_BG  0x2104
#define C_KEY_SP  0x4208
#define C_KEY_ACT 0x0557

// ============================================================
// UI STATE
// ============================================================
enum ScreenState { SCR_MAIN, SCR_WIFI };
enum WiFiSubState { W_MENU, W_SCAN, W_LIST, W_INPUT };
enum EditField   { FIELD_NONE, FIELD_SSID, FIELD_USER, FIELD_PASS };
ScreenState currentScreen = SCR_MAIN;
WiFiSubState wifiSubState = W_MENU;
EditField   editingField  = FIELD_NONE;
String inputSSID    = "";
String inputUser    = "";
String inputPass    = "";
bool   inputIsEnt   = false;
bool   showPassword = false;
int    kbPage       = 0; // 0=lower 1=upper 2=num/sym
int    wifiFoundCount = 0;
int    wifiListPage   = 0;

// Keyboard layouts
const char* KB_ROW0[3] = { "qwertyuiop", "QWERTYUIOP", "1234567890" };
const char* KB_ROW1[3] = { "asdfghjkl",  "ASDFGHJKL",  "-/:;()@&\"" };
const char* KB_ROW2[3] = { "zxcvbnm",    "ZXCVBNM",    ".,?!'_~^`" };

// Keyboard geometry (Landscape 480px wide)
#define KB_Y  130
#define KH     42
#define KW     40
#define KGAP    6

// ============================================================
// WIFI & BRAILLE STATE
// ============================================================
String   WIFI_SSID     = "";
String   WIFI_PASSWORD = "";
String   WIFI_USER     = "";
bool     WIFI_IS_ENT   = false;
uint8_t  currentPattern   = 0;
uint8_t  currentMode      = 0; // 0=Huruf 1=Angka 2=Kata
String   currentWord      = "";
char     lastChar         = '-';
uint16_t lastButtonState  = 0xFFFF;
uint16_t lastFlickerState = 0xFFFF;
unsigned long lastDebounceTime = 0;
unsigned long lastTouchMs = 0;
#define TOUCH_DEBOUNCE_MS 220
const uint8_t extraGPIO[6] = { 1, 3, 16, 17, 21, 22 };
uint8_t ttsSpeedMode = 2; // 0=Normal, 1=Lambat, 2=Sangat Lambat
String   appStatus     = "Listening";
String   lastSpokenWord = "-";
String   calcExpression = "";
String   calcResult     = "";

// ============================================================
// SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n\n================================");
    Serial.println("   ESP32-S3 Braille Trainer");
    Serial.println("================================");
    
    EEPROM.begin(EEPROM_SIZE);
    Serial.println("[EEPROM] OK");
    eepromLoadSpeed();
    
    // LCD Init
    pinMode(TFT_BL,OUTPUT); digitalWrite(TFT_BL,HIGH);
    SPI.begin(TFT_SCLK,TFT_MISO,TFT_MOSI,-1);
    tft.init(320,480,0,0,ST7796S_RGB);
    tft.setRotation(1); // Landscape 480x320
    
    // Splash screen - CENTERED
    tft.fillScreen(C_BG);
    tft.drawRoundRect(60, 80, 360, 160, 16, C_BORDER);
    tft.fillRoundRect(62, 82, 356, 156, 14, C_SURFACE);
    
    tft.setFont(&FreeSansBold24pt7b); tft.setTextSize(1);
    tft.setTextColor(C_ACCENT);
    int16_t x1,y1; uint16_t tw,th;
    tft.getTextBounds("Braille",0,0,&x1,&y1,&tw,&th);
    tft.setCursor((480-(int16_t)tw)/2, 145);
    tft.print("Braille");
    
    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(C_DGRAY);
    tft.getTextBounds("Trainer ESP32-S3",0,0,&x1,&y1,&tw,&th);
    tft.setCursor((480-(int16_t)tw)/2, 190);
    tft.print("Trainer ESP32-S3");
    
    tft.setTextColor(C_SUCCESS);
    tft.getTextBounds("Memuat...",0,0,&x1,&y1,&tw,&th);
    tft.setCursor((480-(int16_t)tw)/2, 220);
    tft.print("Memuat...");
    Serial.println("[LCD] ST7796S OK (Landscape 480x320)");
    
    // Touch Init
    ts.begin();
    SPI.begin(TFT_SCLK,TFT_MISO,TFT_MOSI,-1); // Kunci ulang pin SPI
    ts.setRotation(0);
    Serial.println("[Touch] HR2046/XPT2046 OK");
    
    // WiFi auto-connect
    Serial.println("[WiFi] Mencoba auto-konek dari EEPROM...");
    bool wOK = autoConnectWiFi();
    if (!wOK) Serial.println("[WiFi] Melanjutkan tanpa WiFi. Tekan W di Serial atau sentuh ikon WiFi.");
    
    // Audio
    initAudio();
    
    // MCP23017
    Wire.begin(SDA_PIN,SCL_PIN);
    if (mcp1.begin_I2C(0x20)) {
        for (int i=0;i<16;i++) mcp1.pinMode(i,INPUT_PULLUP);
        Serial.println("[MCP23017] OK");
    } else {
        Serial.println("[MCP23017] ERROR - Cek I2C!");
    }
    
    // Extra GPIO
    for (int i=0;i<6;i++) pinMode(extraGPIO[i],INPUT_PULLUP);
    
    drawMainScreen();
    
    Serial.println("\n======= PERINTAH SERIAL =======");
    Serial.println(" 1-6   = Toggle Titik Braille 1-6");
    Serial.println(" E     = Konfirmasi / Enter");
    Serial.println(" S     = Spasi (mode Kata)");
    Serial.println(" X     = Hapus huruf terakhir");
    Serial.println(" M     = Ganti Mode (Huruf/Angka/Kata/Kalkulator)");
    Serial.println(" +−*/  = Operator Kalkulator (mode Kalkulator)");
    Serial.println(" W     = Input WiFi (SCAN/Enterprise/Personal)");
    Serial.println(" CLR   = Hapus WiFi dari EEPROM");
    Serial.println(" @     = Tampilkan status lengkap");
    Serial.println(" !     = Tes suara TTS");
    Serial.println("================================\n");
}

void loop() {
    audio.loop();
    checkButtons();
    handleTouch();
    serialMenu();
    
    // Auto-update status to Speaking or Listening based on audio engine state
    if (audio.isRunning()) {
        if (appStatus != "Speaking") {
            appStatus = "Speaking";
            drawStatusBadge();
        }
    } else {
        if (appStatus == "Speaking") {
            appStatus = "Listening";
            drawStatusBadge();
        }
    }
    
    // Auto-refresh WiFi indicator every 5 seconds
    static unsigned long lastWifiCheck = 0;
    if (millis() - lastWifiCheck > 5000) {
        lastWifiCheck = millis();
        drawWiFiIndicator();
    }
}
