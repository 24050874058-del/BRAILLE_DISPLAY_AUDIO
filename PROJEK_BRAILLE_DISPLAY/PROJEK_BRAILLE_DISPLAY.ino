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

// ============================================================
// EEPROM
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

uint8_t ttsSpeedMode = 2; // 0=Normal, 1=Lambat, 2=Sangat Lambat

String getTtsSpeedParam() {
    if (ttsSpeedMode == 0) return "id";
    if (ttsSpeedMode == 1) return "id&ttsspeed=0.6";
    return "id&ttsspeed=0.3";
}

void eepromSaveSpeed() {
    EEPROM.write(EEPROM_SPEED_ADDR, ttsSpeedMode);
    EEPROM.commit();
}

void eepromLoadSpeed() {
    ttsSpeedMode = EEPROM.read(EEPROM_SPEED_ADDR);
    if (ttsSpeedMode > 2) ttsSpeedMode = 2; // Default sangat lambat
}

void eepromSaveWiFi(const String &ssid, const String &pass, const String &user, bool isEnt) {
    EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VAL);
    int ls = min((int)ssid.length(), EEPROM_MAX_LEN);
    for (int i=0;i<ls;i++) EEPROM.write(EEPROM_SSID_ADDR+i, ssid[i]);
    EEPROM.write(EEPROM_SSID_ADDR+ls, 0);
    int lp = min((int)pass.length(), EEPROM_MAX_LEN);
    for (int i=0;i<lp;i++) EEPROM.write(EEPROM_PASS_ADDR+i, pass[i]);
    EEPROM.write(EEPROM_PASS_ADDR+lp, 0);
    int lu = min((int)user.length(), EEPROM_MAX_LEN);
    for (int i=0;i<lu;i++) EEPROM.write(EEPROM_USER_ADDR+i, user[i]);
    EEPROM.write(EEPROM_USER_ADDR+lu, 0);
    EEPROM.write(EEPROM_ENT_ADDR, isEnt ? 1 : 0);
    EEPROM.commit();
    Serial.println("[EEPROM] Kredensial disimpan.");
}

bool eepromLoadWiFi(String &ssid, String &pass, String &user, bool &isEnt) {
    if (EEPROM.read(EEPROM_MAGIC_ADDR) != EEPROM_MAGIC_VAL) return false;
    ssid = ""; for (int i=0;i<EEPROM_MAX_LEN;i++) { char c=(char)EEPROM.read(EEPROM_SSID_ADDR+i); if(!c) break; ssid+=c; }
    pass = ""; for (int i=0;i<EEPROM_MAX_LEN;i++) { char c=(char)EEPROM.read(EEPROM_PASS_ADDR+i); if(!c) break; pass+=c; }
    user = ""; for (int i=0;i<EEPROM_MAX_LEN;i++) { char c=(char)EEPROM.read(EEPROM_USER_ADDR+i); if(!c) break; user+=c; }
    isEnt = (EEPROM.read(EEPROM_ENT_ADDR) == 1);
    return ssid.length() > 0;
}

void eepromClearWiFi() {
    EEPROM.write(EEPROM_MAGIC_ADDR, 0x00);
    EEPROM.commit();
    Serial.println("[EEPROM] Kredensial dihapus.");
}

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
// TOUCH CALIBRATION (Landscape 480x320)
// Kalibrasi dari data user:
//   Pojok Kiri Atas  Raw(678, 521)
//   Pojok Kanan Bawah Raw(3495, 479) -- rawX naik ke bawah, rawY naik ke kiri
// Kesimpulan mapping:
//   Layar X (0..479) <- rawY (3600..470)
//   Layar Y (0..319) <- rawX (700..3500)
// ============================================================
void touchToScreen(int16_t rawX, int16_t rawY, int16_t &sx, int16_t &sy) {
    sx = constrain(map(rawY, 3600, 470, 0, 480), 0, 480);
    sy = constrain(map(rawX,  700, 3500, 0, 320), 0, 320);
}

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

// ============================================================
// BRAILLE DECODE
// ============================================================
char decodeBraille(uint8_t p, bool isNum) {
    if (isNum) { switch(p) {
        case 0b000001: return '1'; case 0b000011: return '2';
        case 0b001001: return '3'; case 0b011001: return '4';
        case 0b010001: return '5'; case 0b001011: return '6';
        case 0b011011: return '7'; case 0b010011: return '8';
        case 0b001010: return '9'; case 0b011010: return '0';
        default: return '?'; } }
    switch(p) {
        case 0b000001: return 'a'; case 0b000011: return 'b';
        case 0b001001: return 'c'; case 0b011001: return 'd';
        case 0b010001: return 'e'; case 0b001011: return 'f';
        case 0b011011: return 'g'; case 0b010011: return 'h';
        case 0b001010: return 'i'; case 0b011010: return 'j';
        case 0b000101: return 'k'; case 0b000111: return 'l';
        case 0b001101: return 'm'; case 0b011101: return 'n';
        case 0b010101: return 'o'; case 0b001111: return 'p';
        case 0b011111: return 'q'; case 0b010111: return 'r';
        case 0b001110: return 's'; case 0b011110: return 't';
        case 0b100101: return 'u'; case 0b100111: return 'v';
        case 0b111010: return 'w'; case 0b101101: return 'x';
        case 0b111101: return 'y'; case 0b110101: return 'z';
        default: return '?';
    }
}

// ============================================================
// UI HELPERS
// ============================================================
bool inRect(int16_t tx,int16_t ty,int16_t rx,int16_t ry,int16_t rw,int16_t rh) {
    return tx>=rx && tx<rx+rw && ty>=ry && ty<ry+rh;
}

void drawBtn(int16_t x,int16_t y,int16_t w,int16_t h,
             uint16_t bg,uint16_t fg,const char* lbl,uint8_t sz=1,uint8_t r=6) {
    tft.fillRoundRect(x,y,w,h,r,bg);
    int16_t x1,y1; uint16_t tw,th;
    tft.setFont(); tft.setTextSize(sz); tft.setTextColor(fg,bg);
    tft.getTextBounds(lbl,0,0,&x1,&y1,&tw,&th);
    tft.setCursor(x+(w-(int16_t)tw)/2, y+(h-(int16_t)th)/2);
    tft.print(lbl);
}

void drawField(int16_t x,int16_t y,int16_t w,int16_t h,
               const String &text,bool active,const char* ph,bool masked=false) {
    uint16_t bc = active ? C_ACCENT : C_BORDER;
    tft.fillRoundRect(x,y,w,h,5,C_SURFACE);
    tft.drawRoundRect(x,y,w,h,5,bc);
    if (active) tft.drawRoundRect(x+1,y+1,w-2,h-2,4,bc);
    tft.setFont(); tft.setTextSize(1);
    if (text.length()==0) {
        tft.setTextColor(C_DGRAY,C_SURFACE);
        tft.setCursor(x+6, y+(h-8)/2); tft.print(ph);
    } else {
        tft.setTextColor(C_WHITE,C_SURFACE);
        String d="";
        if (masked) for (uint16_t i=0;i<text.length();i++) d+='*';
        else d=text;
        int mc=(w-12)/6;
        if ((int)d.length()>mc) d=d.substring(d.length()-mc);
        tft.setCursor(x+6, y+(h-8)/2); tft.print(d);
        if (active) {
            int cx=x+6+d.length()*6;
            if (cx<x+w-4) tft.fillRect(cx,y+4,2,h-8,C_ACCENT);
        }
    }
}

// ============================================================
// KEYBOARD
// ============================================================
void drawKeyboard() {
    tft.fillRect(0,KB_Y-4,480,320-KB_Y+4,C_BG);
    tft.drawFastHLine(0,KB_Y-5,480,C_BORDER);

    // ROW 0
    int r0t=10*KW+9*KGAP, r0x=(480-r0t)/2;
    for (int i=0;i<10;i++) {
        char l[2]={KB_ROW0[kbPage][i],0};
        drawBtn(r0x+i*(KW+KGAP),KB_Y,KW,KH,C_KEY_BG,C_WHITE,l,1,4);
    }
    // ROW 1
    int r1l=strlen(KB_ROW1[kbPage]), r1t=r1l*KW+(r1l-1)*KGAP;
    int r1x=(480-r1t)/2, r1y=KB_Y+KH+KGAP;
    for (int i=0;i<r1l;i++) {
        char l[2]={KB_ROW1[kbPage][i],0};
        drawBtn(r1x+i*(KW+KGAP),r1y,KW,KH,C_KEY_BG,C_WHITE,l,1,4);
    }
    // ROW 2
    int r2l=strlen(KB_ROW2[kbPage]), shW=56, bsW=56;
    int r2t=shW+KGAP+r2l*(KW+KGAP)-KGAP+KGAP+bsW;
    int r2x=(480-r2t)/2, r2y=KB_Y+2*(KH+KGAP);
    drawBtn(r2x,r2y,shW,KH,(kbPage==1)?C_ACCENT:C_KEY_ACT,C_WHITE,(kbPage==1)?"^A":"^a",1,4);
    for (int i=0;i<r2l;i++) {
        char l[2]={KB_ROW2[kbPage][i],0};
        drawBtn(r2x+shW+KGAP+i*(KW+KGAP),r2y,KW,KH,C_KEY_BG,C_WHITE,l,1,4);
    }
    drawBtn(r2x+shW+KGAP+r2l*(KW+KGAP),r2y,bsW,KH,C_KEY_ACT,C_WHITE,"<=",1,4);

    // ROW 3
    int r3y=KB_Y+3*(KH+KGAP), fnW=80, okW=80, r3x=8;
    int spW=480-(r3x*2)-fnW-KGAP-KGAP-okW;
    drawBtn(r3x,              r3y,fnW,KH,C_KEY_ACT,C_WHITE,kbPage<2?"123":"ABC",1,4);
    drawBtn(r3x+fnW+KGAP,     r3y,spW,KH,C_KEY_SP, C_WHITE,"SPASI",1,4);
    drawBtn(r3x+fnW+KGAP+spW+KGAP,r3y,okW,KH,C_SUCCESS,C_WHITE,"OK",1,6);
}

char handleKbTouch(int16_t tx,int16_t ty) {
    if (ty<KB_Y-4) return '\0';
    // ROW 0
    int r0t=10*KW+9*KGAP, r0x=(480-r0t)/2;
    if (inRect(tx,ty,r0x,KB_Y,r0t,KH)) {
        int i=(tx-r0x)/(KW+KGAP); if(i>=0&&i<10) return KB_ROW0[kbPage][i];
    }
    // ROW 1
    int r1l=strlen(KB_ROW1[kbPage]), r1t=r1l*KW+(r1l-1)*KGAP;
    int r1x=(480-r1t)/2, r1y=KB_Y+KH+KGAP;
    if (inRect(tx,ty,r1x,r1y,r1t,KH)) {
        int i=(tx-r1x)/(KW+KGAP); if(i>=0&&i<r1l) return KB_ROW1[kbPage][i];
    }
    // ROW 2
    int r2l=strlen(KB_ROW2[kbPage]), shW=56, bsW=56;
    int r2t=shW+KGAP+r2l*(KW+KGAP)-KGAP+KGAP+bsW;
    int r2x=(480-r2t)/2, r2y=KB_Y+2*(KH+KGAP);
    if (inRect(tx,ty,r2x,r2y,shW,KH)) return '\t';
    int chX=r2x+shW+KGAP;
    if (inRect(tx,ty,chX,r2y,r2l*(KW+KGAP)-KGAP,KH)) {
        int i=(tx-chX)/(KW+KGAP); if(i>=0&&i<r2l) return KB_ROW2[kbPage][i];
    }
    if (inRect(tx,ty,r2x+shW+KGAP+r2l*(KW+KGAP),r2y,bsW,KH)) return '\b';
    // ROW 3
    int r3y=KB_Y+3*(KH+KGAP), fnW=80, okW=80, r3x=8;
    int spW=480-(r3x*2)-fnW-KGAP-KGAP-okW;
    if (inRect(tx,ty,r3x,              r3y,fnW,KH)) return '\x01';
    if (inRect(tx,ty,r3x+fnW+KGAP,    r3y,spW,KH)) return ' ';
    if (inRect(tx,ty,r3x+fnW+KGAP+spW+KGAP,r3y,okW,KH)) return '\r';
    return '\0';
}

// ============================================================
// MAIN SCREEN (Landscape 480x320)
// ============================================================
void drawBrailleDots() {
    // 6 titik: kiri=1,2,3 kanan=4,5,6, layout vertikal
    // Area braille: X 10-210, Y 110-310
    int dotR=28, dotGapX=90, dotGapY=64;
    // Tengahkan secara vertikal: total tinggi 2*dotR + 2*dotGapY = 56+128=184, start Y=(320-184)/2+44
    int totalH = 2*dotR + 2*dotGapY;   // 184
    int totalW = 2*dotR + dotGapX;     // 146
    int sX = (210 - totalW) / 2 + 10; // ~42
    int sY = (320 - 44 - 26 - 34 - totalH) / 2 + 44 + 26 + 34 + dotR; // tengah area
    for (int i=0;i<6;i++) {
        int col=i/3, row=i%3;
        int cx=sX+col*dotGapX, cy=sY+row*dotGapY;
        bool act=(currentPattern>>i)&1;
        
        // Perbaiki bug visual melenceng dengan membersihkan background dan menggunakan layer fillCircle
        tft.fillCircle(cx,cy,dotR+2,C_BG);
        tft.fillCircle(cx,cy,dotR+2,act?C_WHITE:C_GRAY);
        tft.fillCircle(cx,cy,dotR,act?C_ACCENT:C_DGRAY);
        
        tft.setFont(&FreeSans12pt7b); tft.setTextSize(1);
        tft.setTextColor(act?C_BG:C_GRAY);
        int16_t x1,y1; uint16_t tw,th;
        tft.getTextBounds(String(i+1),0,0,&x1,&y1,&tw,&th);
        tft.setCursor(cx-tw/2, cy+th/2-2);
        tft.print(i+1);
    }
}

void drawMainScreen() {
    tft.fillScreen(C_BG);

    // ======= TOP BAR =======
    tft.fillRect(0,0,480,50,C_PRIMARY);
    tft.setFont(&FreeSansBold12pt7b); tft.setTextSize(1);
    tft.setTextColor(C_WHITE);
    tft.setCursor(14,34); tft.print("Braille Trainer");

    // WiFi icon (kanan atas, 50px tall)
    bool wOK=(WiFi.status()==WL_CONNECTED);
    uint16_t wCol=wOK?C_SUCCESS:C_ERROR;
    tft.fillRect(430,0,50,50,C_PRIMARY);
    tft.drawRoundRect(431,2,48,46,5,wOK?C_SUCCESS:C_ERROR);
    // Ikon WiFi: 3 busur
    int cx=455, cy=34;
    tft.drawCircle(cx,cy,16,wCol);
    tft.drawCircle(cx,cy,10,wCol);
    tft.fillCircle(cx,cy, 4,wCol);
    tft.setTextSize(1); tft.setTextColor(wCol,C_PRIMARY);
    tft.setCursor(440,4); tft.print("WiFi");

    // ======= STATUS STRIP =======
    uint16_t stBg=wOK?0x0320:0x3000;
    tft.fillRect(0,50,480,24,stBg);
    tft.setFont(); tft.setTextSize(1);
    tft.setTextColor(C_WHITE,stBg);
    tft.setCursor(8,58);
    if (wOK) {
        tft.print(" Terhubung: "); tft.print(WIFI_SSID);
        tft.print("  |  IP: "); tft.print(WiFi.localIP().toString());
    } else {
        tft.print(" WiFi terputus  -  Sentuh ikon WiFi di kanan atas untuk mengatur");
    }

    // ======= MODE BAR =======
    tft.fillRect(0,74,480,36,C_SURFACE);
    tft.drawFastHLine(0,74, 480,C_BORDER);
    tft.drawFastHLine(0,109,480,C_BORDER);
    tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_LGRAY,C_SURFACE);
    tft.setCursor(10,84); tft.print("MODE AKTIF:");
    tft.setFont(&FreeSans12pt7b);
    tft.setTextColor(C_ACCENT); 
    const char* modeNames[]={"HURUF","ANGKA","KATA"};
    tft.setCursor(94,100); tft.print(modeNames[currentMode]);

    // ======= DIVIDER VERTIKAL =======
    tft.drawFastVLine(215,110,200,C_BORDER);

    // ======= BRAILLE DOTS (kiri) =======
    tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_ACCENT,C_BG);
    // Label header titik
    tft.setCursor(14,114); tft.print("[ TITIK BRAILLE ]");
    drawBrailleDots();

    // ======= PANEL KANAN =======
    if (currentMode==2) {
        // Mode Kata: tampilkan kata
        tft.fillRoundRect(220,110,254,170,8,C_SURFACE);
        tft.drawRoundRect(220,110,254,170,8,C_BORDER);
        tft.setFont(); tft.setTextSize(1);
        tft.setTextColor(C_LGRAY,C_SURFACE);
        tft.setCursor(232,122); tft.print("KATA SAAT INI:");
        tft.drawFastHLine(224,134,242,C_BORDER);
        // Kata besar di tengah
        tft.setFont(&FreeSansBold24pt7b); tft.setTextSize(1); 
        tft.setTextColor(C_WHITE);
        String txt = currentWord.length() ? currentWord : "-";
        int16_t x1,y1; uint16_t tw,th;
        tft.getTextBounds(txt,0,0,&x1,&y1,&tw,&th);
        tft.setCursor(220 + (254-tw)/2, 200); 
        tft.print(txt);
        // Pola binary kecil
        tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_DGRAY,C_SURFACE);
        tft.setCursor(232,254); tft.print("Pola: 0b");
        for (int i=5;i>=0;i--) tft.print((currentPattern>>i)&1);
    } else {
        // Mode Huruf / Angka: tampilkan karakter terakhir
        tft.fillRoundRect(220,110,254,170,8,C_SURFACE);
        tft.drawRoundRect(220,110,254,170,8,C_BORDER);
        tft.setFont(); tft.setTextSize(1);
        tft.setTextColor(C_LGRAY,C_SURFACE);
        tft.setCursor(232,122); tft.print(currentMode==0 ? "HURUF TERAKHIR:" : "ANGKA TERAKHIR:");
        tft.drawFastHLine(224,134,242,C_BORDER);
        
        // Karakter besar di tengah
        tft.setFont(&FreeSansBold24pt7b); tft.setTextSize(1);
        tft.setTextColor(C_ACCENT);
        String txt = String(lastChar);
        int16_t x1,y1; uint16_t tw,th;
        tft.getTextBounds(txt,0,0,&x1,&y1,&tw,&th);
        tft.setCursor(220 + (254-tw)/2, 200); 
        tft.print(txt);
        
        // Pola binary kecil
        tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_DGRAY,C_SURFACE);
        tft.setCursor(232,254); tft.print("Pola: 0b");
        for (int i=5;i>=0;i--) tft.print((currentPattern>>i)&1);
    }

    // ======= TOMBOL KECEPATAN SUARA =======
    uint16_t spdBg = (ttsSpeedMode==0)?C_SUCCESS:((ttsSpeedMode==1)?C_WARNING:C_ERROR);
    tft.fillRoundRect(220,283,254,24,4,spdBg);
    tft.setFont(); tft.setTextSize(1);
    tft.setTextColor((ttsSpeedMode==1)?C_BG:C_WHITE);
    const char* spdName[] = {"SUARA: NORMAL", "SUARA: LAMBAT", "SUARA: S. LAMBAT"};
    int16_t bx,by; uint16_t bw,bh;
    tft.getTextBounds(spdName[ttsSpeedMode],0,0,&bx,&by,&bw,&bh);
    tft.setCursor(220 + (254-bw)/2, 283 + (24-bh)/2);
    tft.print(spdName[ttsSpeedMode]);

    // ======= FOOTER =======
    tft.fillRect(0,310,480,10,C_SURFACE);
    tft.drawFastHLine(0,310,480,C_BORDER);
    tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_DGRAY,C_SURFACE);
    tft.setCursor(8,312); tft.print("ESP32-S3 Braille Trainer  |  Touch ikon WiFi = Pengaturan Jaringan");
}

void refreshMainWiFiStatus() {
    bool wOK=(WiFi.status()==WL_CONNECTED);
    uint16_t wCol=wOK?C_SUCCESS:C_ERROR;
    tft.fillRect(430,0,50,50,C_PRIMARY);
    tft.drawRoundRect(431,2,48,46,5,wOK?C_SUCCESS:C_ERROR);
    tft.drawCircle(455,34,16,wCol);
    tft.drawCircle(455,34,10,wCol);
    tft.fillCircle(455,34, 4,wCol);
    tft.setFont(); tft.setTextSize(1); tft.setTextColor(wCol,C_PRIMARY);
    tft.setCursor(440,4); tft.print("WiFi");
    uint16_t stBg=wOK?0x0320:0x3000;
    tft.fillRect(0,50,480,24,stBg);
    tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_WHITE,stBg);
    tft.setCursor(8,58);
    if (wOK) {
        tft.print(" Terhubung: "); tft.print(WIFI_SSID);
        tft.print("  |  IP: "); tft.print(WiFi.localIP().toString());
    } else {
        tft.print(" WiFi terputus  -  Sentuh ikon WiFi di kanan atas untuk mengatur");
    }
}

// ============================================================
// WIFI SCREEN (Landscape 480x320)
// ============================================================
void drawWiFiScreen() {
    bool kbV=(editingField!=FIELD_NONE);
    bool wOK=(WiFi.status()==WL_CONNECTED);
    tft.fillScreen(C_BG);

    // Top Bar
    tft.fillRect(0,0,480,44,C_PRIMARY);
    drawBtn(4,6,80,32,C_KEY_ACT,C_WHITE,"< Balik",1,5);
    tft.setFont(); tft.setTextSize(2); tft.setTextColor(C_WHITE,C_PRIMARY);
    tft.setCursor(100,12); tft.print("Pengaturan WiFi");

    if (wifiSubState == W_MENU) {
        uint16_t stBg=wOK?0x0320:0x3000;
        tft.fillRoundRect(8,52,464,42,6,stBg);
        tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_WHITE,stBg);
        tft.setCursor(16,60);
        if (wOK) {
            tft.print("Terhubung ke: "); tft.print(WIFI_SSID);
            tft.setCursor(16,74); tft.print("IP: "); tft.print(WiFi.localIP().toString());
        } else {
            tft.print("Status: Terputus dari jaringan.");
        }
        
        drawBtn(8, 120, 464, 50, C_ACCENT, C_WHITE, "CARI WIFI DI SEKITAR", 1, 8);
        drawBtn(8, 190, 464, 50, C_ERROR, C_WHITE, "HAPUS WIFI TERSIMPAN", 1, 8);
    }
    else if (wifiSubState == W_SCAN) {
        tft.setFont(); tft.setTextSize(2); tft.setTextColor(C_WHITE, C_BG);
        tft.setCursor(120, 150); tft.print("Mencari WiFi...");
    }
    else if (wifiSubState == W_LIST) {
        int startIdx = wifiListPage * 4;
        int count = min(4, wifiFoundCount - startIdx);
        if (wifiFoundCount == 0) {
            tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_WHITE, C_BG);
            tft.setCursor(16, 80); tft.print("Tidak ada WiFi ditemukan.");
        }
        for (int i=0; i<count; i++) {
            int y = 52 + i * 52;
            String ssid = WiFi.SSID(startIdx + i);
            int encType = WiFi.encryptionType(startIdx + i);
            bool isEnt = (encType == WIFI_AUTH_WPA2_ENTERPRISE);
            String encStr = (encType == WIFI_AUTH_OPEN) ? "Terbuka" : (isEnt ? "WPA2 Enterprise (Kampus)" : "Terkunci WPA");
            
            tft.fillRoundRect(8, y, 464, 46, 6, C_SURFACE);
            tft.drawRoundRect(8, y, 464, 46, 6, C_BORDER);
            tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_WHITE, C_SURFACE);
            tft.setCursor(16, y+8); tft.print(ssid);
            tft.setTextColor(C_LGRAY, C_SURFACE);
            tft.setCursor(16, y+24); tft.print(encStr);
        }
        // Prev/Next buttons
        if (wifiListPage > 0) drawBtn(8, 270, 100, 40, C_KEY_ACT, C_WHITE, "< Prev", 1, 6);
        if (startIdx + 4 < wifiFoundCount) drawBtn(372, 270, 100, 40, C_KEY_ACT, C_WHITE, "Next >", 1, 6);
    }
    else if (wifiSubState == W_INPUT) {
        if (kbV) {
            // compact mode
            tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_LGRAY,C_BG);
            tft.setCursor(8,52); tft.print("SSID: "); tft.print(inputSSID);
            if (inputIsEnt) {
                tft.setCursor(8,68); tft.print("User:");
                drawField(48,60,424,28,inputUser,editingField==FIELD_USER,"Username...");
                tft.setCursor(8,98); tft.print("Pass:");
                drawField(48,90,380,28,inputPass,editingField==FIELD_PASS,"Password...",!showPassword);
                drawBtn(432,90,40,28,C_KEY_ACT,C_WHITE,showPassword?"abc":"***",1,4);
            } else {
                tft.setCursor(8,80); tft.print("Password:");
                drawField(8,92,422,34,inputPass,editingField==FIELD_PASS,"Password...",!showPassword);
                drawBtn(434,92,38,34,C_KEY_ACT,C_WHITE,showPassword?"abc":"***",1,4);
            }
            drawKeyboard();
        } else {
            // normal mode
            tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_WHITE, C_BG);
            tft.setCursor(10, 60); tft.print("Menghubungkan ke: "); tft.print(inputSSID);
            
            if (inputIsEnt) {
                tft.setTextColor(C_ACCENT, C_BG);
                tft.setCursor(10, 80); tft.print("Jaringan ini butuh Username (Enterprise)");
                tft.setTextColor(C_LGRAY, C_BG);
                tft.setCursor(10, 104); tft.print("Username:");
                drawField(8, 114, 464, 40, inputUser, false, "Sentuh untuk isi username...");
                tft.setCursor(10, 162); tft.print("Password:");
                drawField(8, 172, 412, 40, inputPass, false, "Sentuh untuk isi password...", !showPassword);
                drawBtn(428, 172, 44, 40, C_KEY_ACT, C_WHITE, showPassword?"abc":"***", 1, 4);
            } else {
                tft.setTextColor(C_LGRAY, C_BG);
                tft.setCursor(10, 100); tft.print("Password:");
                drawField(8, 114, 412, 40, inputPass, false, "Sentuh untuk isi password...", !showPassword);
                drawBtn(428, 114, 44, 40, C_KEY_ACT, C_WHITE, showPassword?"abc":"***", 1, 4);
            }
            drawBtn(8, 230, 224, 44, C_SUCCESS, C_WHITE, "SAMBUNGKAN", 1, 8);
            drawBtn(248, 230, 224, 44, C_ERROR, C_WHITE, "BATAL", 1, 8);
        }
    }
}

void showConnectingOverlay(const String &ssid) {
    tft.fillRoundRect(80,100,320,120,10,C_SURFACE);
    tft.drawRoundRect(80,100,320,120,10,C_ACCENT);
    tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_LGRAY,C_SURFACE);
    tft.setCursor(100,116); tft.print("Menghubungkan ke:");
    tft.setTextSize(2); tft.setTextColor(C_WHITE,C_SURFACE);
    tft.setCursor(100,136); tft.print(ssid.length()>20?ssid.substring(0,20):ssid);
    tft.setTextSize(1); tft.setTextColor(C_ACCENT,C_SURFACE);
    tft.setCursor(100,162); tft.print("Harap tunggu (maks 20 detik)...");
    for (int i=0;i<5;i++) tft.fillCircle(170+i*22,192,7,C_ACCENT);
}

void showConnectResult(bool ok, const String &ssid) {
    uint16_t col=ok?C_SUCCESS:C_ERROR;
    tft.fillRoundRect(80,100,320,120,10,C_SURFACE);
    tft.drawRoundRect(80,100,320,120,10,col);
    tft.setFont(); tft.setTextSize(2); tft.setTextColor(col,C_SURFACE);
    tft.setCursor(100,116); tft.print(ok?"Berhasil!":"Gagal!");
    tft.setTextSize(1); tft.setTextColor(C_LGRAY,C_SURFACE);
    tft.setCursor(100,146);
    if (ok) {
        tft.print("Terhubung: "); tft.print(ssid);
        tft.setCursor(100,160); tft.print("IP: "); tft.print(WiFi.localIP().toString());
        tft.setCursor(100,174); tft.print("Tersimpan ke EEPROM.");
    } else {
        tft.print("Tidak bisa terhubung.");
        tft.setCursor(100,160); tft.print("Periksa SSID dan Password.");
    }
    delay(3000);
}

// ============================================================
// TOUCH HANDLERS
// ============================================================
void handleTouchMain(int16_t tx,int16_t ty) {
    if (inRect(tx,ty,436,0,44,44)) {
        Serial.println("[Touch] -> Buka Pengaturan WiFi");
        wifiSubState = W_MENU;
        editingField=FIELD_NONE; kbPage=0;
        currentScreen=SCR_WIFI; drawWiFiScreen();
        return;
    }
    // Tombol Kecepatan Suara
    if (inRect(tx,ty,220,283,254,24)) {
        ttsSpeedMode = (ttsSpeedMode + 1) % 3;
        Serial.print("[Touch] -> Ubah Kecepatan Suara ke: "); Serial.println(ttsSpeedMode);
        eepromSaveSpeed();
        if (ttsSpeedMode == 0) speak("Normal");
        else if (ttsSpeedMode == 1) speak("Lambat");
        else speak("Sangat Lambat");
        drawMainScreen();
        return;
    }
}

void handleTouchWiFi(int16_t tx,int16_t ty) {
    bool kbV=(editingField!=FIELD_NONE);

    // Tombol Kembali
    if (inRect(tx,ty,0,0,90,44)) {
        if (wifiSubState == W_MENU) {
            editingField=FIELD_NONE; currentScreen=SCR_MAIN; drawMainScreen(); return;
        } else if (wifiSubState == W_LIST) {
            wifiSubState = W_MENU; drawWiFiScreen(); return;
        } else if (wifiSubState == W_INPUT) {
            if (kbV) { editingField=FIELD_NONE; drawWiFiScreen(); return; }
            wifiSubState = W_LIST; drawWiFiScreen(); return;
        }
    }

    if (wifiSubState == W_MENU) {
        if (inRect(tx,ty,8,120,464,50)) {
            wifiSubState = W_SCAN; drawWiFiScreen();
            WiFi.disconnect(); delay(100);
            wifiFoundCount = WiFi.scanNetworks();
            wifiListPage = 0;
            wifiSubState = W_LIST; drawWiFiScreen();
            return;
        }
        if (inRect(tx,ty,8,190,464,50)) {
            eepromClearWiFi();
            WIFI_SSID=WIFI_PASSWORD=WIFI_USER=""; WIFI_IS_ENT=false;
            WiFi.disconnect(true); drawWiFiScreen(); return;
        }
        return;
    }
    
    if (wifiSubState == W_LIST) {
        int startIdx = wifiListPage * 4;
        int count = min(4, wifiFoundCount - startIdx);
        for (int i=0; i<count; i++) {
            if (inRect(tx,ty,8,52+i*52,464,46)) {
                inputSSID = WiFi.SSID(startIdx + i);
                inputIsEnt = (WiFi.encryptionType(startIdx + i) == WIFI_AUTH_WPA2_ENTERPRISE);
                inputPass = ""; inputUser = "";
                wifiSubState = W_INPUT;
                drawWiFiScreen();
                return;
            }
        }
        if (wifiListPage > 0 && inRect(tx,ty,8,270,100,40)) { wifiListPage--; drawWiFiScreen(); return; }
        if (startIdx + 4 < wifiFoundCount && inRect(tx,ty,372,270,100,40)) { wifiListPage++; drawWiFiScreen(); return; }
        return;
    }

    if (wifiSubState == W_INPUT) {
        // Keyboard area
        if (kbV && ty>=KB_Y-4) {
            char k=handleKbTouch(tx,ty);
            if (k=='\0') return;
            String *tgt=(editingField==FIELD_USER)?&inputUser:&inputPass;
            if      (k=='\b') { if(tgt->length()>0) tgt->remove(tgt->length()-1); }
            else if (k=='\r') { editingField=FIELD_NONE; drawWiFiScreen(); return; }
            else if (k=='\t') { kbPage=(kbPage==0)?1:0; drawKeyboard(); return; }
            else if (k=='\x01') { kbPage=(kbPage<2)?2:0; drawKeyboard(); return; }
            else if (tgt->length()<(uint16_t)EEPROM_MAX_LEN) *tgt+=k;
            
            if (inputIsEnt) {
                if (editingField==FIELD_USER) drawField(48,60,424,28,inputUser,true,"Username...");
                else drawField(48,90,380,28,inputPass,true,"Password...",!showPassword);
            } else {
                if (editingField==FIELD_PASS) drawField(8,92,422,34,inputPass,true,"Password...",!showPassword);
            }
            return;
        }

        if (kbV) {
            if (inputIsEnt) {
                if (inRect(tx,ty,48,60,424,28)) { editingField=FIELD_USER; kbPage=0; drawWiFiScreen(); return; }
                if (inRect(tx,ty,48,90,380,28)) { editingField=FIELD_PASS; kbPage=0; drawWiFiScreen(); return; }
                if (inRect(tx,ty,432,90,40,28)) {
                    showPassword=!showPassword;
                    drawField(48,90,380,28,inputPass,editingField==FIELD_PASS,"Password...",!showPassword);
                    drawBtn(432,90,40,28,C_KEY_ACT,C_WHITE,showPassword?"abc":"***",1,4);
                }
            } else {
                if (inRect(tx,ty,8,92,422,34)) { editingField=FIELD_PASS; kbPage=0; drawWiFiScreen(); return; }
                if (inRect(tx,ty,434,92,38,34)) {
                    showPassword=!showPassword;
                    drawField(8,92,422,34,inputPass,editingField==FIELD_PASS,"Password...",!showPassword);
                    drawBtn(434,92,38,34,C_KEY_ACT,C_WHITE,showPassword?"abc":"***",1,4);
                }
            }
        } else {
            if (inputIsEnt) {
                if (inRect(tx,ty,8,114,464,40)) { editingField=FIELD_USER; kbPage=0; drawWiFiScreen(); return; }
                if (inRect(tx,ty,8,172,412,40)) { editingField=FIELD_PASS; kbPage=0; drawWiFiScreen(); return; }
                if (inRect(tx,ty,428,172,44,40)) {
                    showPassword=!showPassword;
                    drawField(8,172,412,40,inputPass,false,"Sentuh untuk isi password...",!showPassword);
                    drawBtn(428,172,44,40,C_KEY_ACT,C_WHITE,showPassword?"abc":"***",1,4);
                }
            } else {
                if (inRect(tx,ty,8,114,412,40)) { editingField=FIELD_PASS; kbPage=0; drawWiFiScreen(); return; }
                if (inRect(tx,ty,428,114,44,40)) {
                    showPassword=!showPassword;
                    drawField(8,114,412,40,inputPass,false,"Sentuh untuk isi password...",!showPassword);
                    drawBtn(428,114,44,40,C_KEY_ACT,C_WHITE,showPassword?"abc":"***",1,4);
                }
            }

            // SAMBUNGKAN
            if (inRect(tx,ty,8,230,224,44)) {
                WIFI_SSID=inputSSID; WIFI_PASSWORD=inputPass; 
                WIFI_USER=inputUser; WIFI_IS_ENT=inputIsEnt;
                showConnectingOverlay(WIFI_SSID);
                bool ok=connectWiFi(20000);
                if (ok) eepromSaveWiFi(WIFI_SSID,WIFI_PASSWORD,WIFI_USER,WIFI_IS_ENT);
                showConnectResult(ok,WIFI_SSID);
                wifiSubState=W_MENU; drawWiFiScreen(); return;
            }
            // BATAL
            if (inRect(tx,ty,248,230,224,44)) {
                wifiSubState=W_LIST; drawWiFiScreen(); return;
            }
        }
    }
}

void handleTouch() {
    if (!ts.tirqTouched()) return;
    if (!ts.touched())     return;
    unsigned long now=millis();
    if (now-lastTouchMs<TOUCH_DEBOUNCE_MS) return;
    lastTouchMs=now;
    TS_Point p=ts.getPoint();
    int16_t tx,ty;
    touchToScreen(p.x,p.y,tx,ty);
    Serial.print("[Touch] Raw("); Serial.print(p.x);
    Serial.print(","); Serial.print(p.y);
    Serial.print(") -> Screen("); Serial.print(tx);
    Serial.print(","); Serial.print(ty); Serial.println(")");
    switch(currentScreen) {
        case SCR_MAIN: handleTouchMain(tx,ty); break;
        case SCR_WIFI: handleTouchWiFi(tx,ty); break;
    }
}

// ============================================================
// WIFI
// ============================================================
bool connectWiFi(unsigned long timeoutMs) {
    Serial.print("[WiFi] Menghubungkan ke ["); Serial.print(WIFI_SSID); Serial.print("] ");
    WiFi.disconnect(true); delay(500);
    WiFi.mode(WIFI_STA); WiFi.setAutoReconnect(true);
    
    if (WIFI_IS_ENT) {
        Serial.println("(WPA2-Enterprise)");
#if __has_include("esp_eap_client.h")
        esp_eap_client_set_identity((uint8_t *)WIFI_USER.c_str(), WIFI_USER.length());
        esp_eap_client_set_username((uint8_t *)WIFI_USER.c_str(), WIFI_USER.length());
        esp_eap_client_set_password((uint8_t *)WIFI_PASSWORD.c_str(), WIFI_PASSWORD.length());
        esp_wifi_sta_enterprise_enable();
#elif __has_include("esp_wpa2.h")
        esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)WIFI_USER.c_str(), WIFI_USER.length());
        esp_wifi_sta_wpa2_ent_set_username((uint8_t *)WIFI_USER.c_str(), WIFI_USER.length());
        esp_wifi_sta_wpa2_ent_set_password((uint8_t *)WIFI_PASSWORD.c_str(), WIFI_PASSWORD.length());
        esp_wifi_sta_wpa2_ent_enable();
#endif
        WiFi.begin(WIFI_SSID.c_str());
    } else {
        if (WIFI_PASSWORD.length()==0||WIFI_PASSWORD=="-") WiFi.begin(WIFI_SSID.c_str());
        else WiFi.begin(WIFI_SSID.c_str(),WIFI_PASSWORD.c_str());
    }
    
    unsigned long t=millis();
    while (WiFi.status()!=WL_CONNECTED) {
        delay(500); Serial.print(".");
        if (millis()-t>timeoutMs) {
            WiFi.disconnect(true);
            Serial.println(" Timeout! Gagal.");
            int st=WiFi.status();
            if (st==4) Serial.println("[WiFi] Password salah!");
            else if (st==WL_NO_SSID_AVAIL) Serial.println("[WiFi] SSID tidak ditemukan!");
            return false;
        }
    }
    Serial.println(" Terhubung!");
    Serial.print("[WiFi] IP: "); Serial.println(WiFi.localIP());
    return true;
}

bool autoConnectWiFi() {
    String ssid,pass,user; bool isEnt;
    if (!eepromLoadWiFi(ssid,pass,user,isEnt)) {
        Serial.println("[WiFi] Tidak ada kredensial di EEPROM.");
        return false;
    }
    Serial.print("[WiFi] Auto-konek ke ["); Serial.print(ssid); Serial.println("]");
    WIFI_SSID=ssid; WIFI_PASSWORD=pass; WIFI_USER=user; WIFI_IS_ENT=isEnt;
    if (connectWiFi(15000)) return true;
    Serial.println("[WiFi] Jaringan tidak tersedia. Lanjut tanpa WiFi.");
    return false;
}

// ============================================================
// SERIAL MENU
// ============================================================
void inputWiFiSerial() {
    Serial.println("\n========== INPUT WIFI (SERIAL) ==========");
    Serial.println("Sama seperti di layar sentuh (UI).");
    Serial.println("Ketik SCAN untuk mencari WiFi terlebih dahulu,");
    Serial.println("atau langsung ketik SSID lalu tekan Enter:");
    while (Serial.available()) { Serial.read(); delay(2); }

    while (!Serial.available()) delay(10);
    String input = Serial.readStringUntil('\n'); input.replace("\r",""); input.trim();

    // ---- Fitur SCAN (sama seperti tombol "CARI WIFI DI SEKITAR" di UI) ----
    if (input.equalsIgnoreCase("SCAN")) {
        Serial.println("\n[WiFi] Memindai jaringan sekitar...");
        WiFi.disconnect(); delay(100);
        int n = WiFi.scanNetworks();
        if (n <= 0) {
            Serial.println("[WiFi] Tidak ada WiFi ditemukan.");
            Serial.println("=========================================\n");
            return;
        }
        Serial.println("\n----- DAFTAR WIFI DITEMUKAN -----");
        for (int i = 0; i < n; i++) {
            int encType = WiFi.encryptionType(i);
            String encStr;
            if (encType == WIFI_AUTH_OPEN) encStr = "Terbuka";
            else if (encType == WIFI_AUTH_WPA2_ENTERPRISE) encStr = "WPA2 Enterprise";
            else encStr = "WPA/WPA2";
            Serial.print("  "); Serial.print(i + 1); Serial.print(") ");
            Serial.print(WiFi.SSID(i));
            Serial.print("  ["); Serial.print(encStr); Serial.print("]");
            Serial.print("  RSSI: "); Serial.print(WiFi.RSSI(i)); Serial.println(" dBm");
        }
        Serial.println("---------------------------------");
        Serial.println("Ketik nomor (1-" + String(n) + ") atau ketik SSID manual:");
        while (Serial.available()) { Serial.read(); delay(2); }
        while (!Serial.available()) delay(10);
        input = Serial.readStringUntil('\n'); input.replace("\r",""); input.trim();

        // Cek apakah input adalah nomor
        int sel = input.toInt();
        if (sel >= 1 && sel <= n) {
            WIFI_SSID = WiFi.SSID(sel - 1);
            int selEnc = WiFi.encryptionType(sel - 1);
            if (selEnc == WIFI_AUTH_WPA2_ENTERPRISE) {
                WIFI_IS_ENT = true;
            } else if (selEnc == WIFI_AUTH_OPEN) {
                // Jaringan terbuka, langsung konek tanpa password
                WIFI_IS_ENT = false;
                WIFI_PASSWORD = "";
                WIFI_USER = "";
                Serial.print("[WiFi] Jaringan terbuka: "); Serial.println(WIFI_SSID);
                Serial.println("[WiFi] Menghubungkan...");
                if (connectWiFi(20000)) {
                    eepromSaveWiFi(WIFI_SSID, WIFI_PASSWORD, WIFI_USER, WIFI_IS_ENT);
                    if (currentScreen == SCR_MAIN) refreshMainWiFiStatus();
                    else drawWiFiScreen();
                } else {
                    Serial.println("[WiFi] Koneksi gagal.");
                }
                Serial.println("=========================================\n");
                return;
            } else {
                WIFI_IS_ENT = false;
            }
        } else {
            WIFI_SSID = input;
            WIFI_IS_ENT = false; // Default, akan ditanya di bawah
        }
    } else {
        WIFI_SSID = input;
        WIFI_IS_ENT = false;
    }

    Serial.print("SSID: ["); Serial.print(WIFI_SSID); Serial.println("]");

    // ---- Tanya tipe jaringan (sama seperti deteksi otomatis di UI) ----
    if (!WIFI_IS_ENT) {
        while (Serial.available()) { Serial.read(); delay(2); }
        Serial.println("\nTipe jaringan:");
        Serial.println("  1) WPA/WPA2 Personal (Password saja)");
        Serial.println("  2) WPA2 Enterprise / Kampus (Username + Password)");
        Serial.println("  3) Terbuka (Tanpa password)");
        Serial.println("Pilih (1/2/3) [default=1]:");
        while (!Serial.available()) delay(10);
        String tipe = Serial.readStringUntil('\n'); tipe.replace("\r",""); tipe.trim();
        if (tipe == "2") {
            WIFI_IS_ENT = true;
        } else if (tipe == "3") {
            WIFI_IS_ENT = false;
            WIFI_PASSWORD = "";
            WIFI_USER = "";
            Serial.println("[WiFi] Jaringan terbuka, tanpa password.");
            Serial.println("[WiFi] Menghubungkan...");
            if (connectWiFi(20000)) {
                eepromSaveWiFi(WIFI_SSID, WIFI_PASSWORD, WIFI_USER, WIFI_IS_ENT);
                if (currentScreen == SCR_MAIN) refreshMainWiFiStatus();
                else drawWiFiScreen();
            } else {
                Serial.println("[WiFi] Koneksi gagal. Kredensial tidak disimpan.");
            }
            Serial.println("=========================================\n");
            return;
        }
    }

    // ---- Input Username (untuk Enterprise, sama seperti field Username di UI) ----
    if (WIFI_IS_ENT) {
        Serial.println("\n[Enterprise] Jaringan ini butuh Username (sama seperti di UI).");
        while (Serial.available()) { Serial.read(); delay(2); }
        Serial.println("Ketik Username lalu tekan Enter:");
        while (!Serial.available()) delay(10);
        WIFI_USER = Serial.readStringUntil('\n'); WIFI_USER.replace("\r",""); WIFI_USER.trim();
        Serial.print("Username: ["); Serial.print(WIFI_USER); Serial.println("]");
    } else {
        WIFI_USER = "";
    }

    // ---- Input Password (sama seperti field Password di UI) ----
    while (Serial.available()) { Serial.read(); delay(2); }
    Serial.println("Ketik Password lalu tekan Enter (kosong=tanpa password):");
    while (!Serial.available()) delay(10);
    WIFI_PASSWORD = Serial.readStringUntil('\n'); WIFI_PASSWORD.replace("\r",""); WIFI_PASSWORD.trim();
    Serial.print("Password: ["); Serial.print(WIFI_PASSWORD.length()); Serial.println(" karakter]");

    // ---- Konfirmasi (seperti tombol SAMBUNGKAN di UI) ----
    Serial.println("\n--- Ringkasan ---");
    Serial.print("  SSID     : "); Serial.println(WIFI_SSID);
    Serial.print("  Tipe     : "); Serial.println(WIFI_IS_ENT ? "WPA2 Enterprise" : "WPA Personal");
    if (WIFI_IS_ENT) { Serial.print("  Username : "); Serial.println(WIFI_USER); }
    Serial.print("  Password : "); Serial.print(WIFI_PASSWORD.length()); Serial.println(" karakter");
    Serial.println("-----------------");
    Serial.println("Ketik Y untuk SAMBUNGKAN, atau N untuk BATAL:");
    while (Serial.available()) { Serial.read(); delay(2); }
    while (!Serial.available()) delay(10);
    String konfirm = Serial.readStringUntil('\n'); konfirm.replace("\r",""); konfirm.trim();
    konfirm.toUpperCase();
    if (konfirm != "Y") {
        Serial.println("[WiFi] Dibatalkan oleh pengguna.");
        Serial.println("=========================================\n");
        return;
    }

    // ---- Sambungkan (sama seperti tombol SAMBUNGKAN di UI) ----
    Serial.println("[WiFi] Menghubungkan...");
    if (connectWiFi(20000)) {
        eepromSaveWiFi(WIFI_SSID, WIFI_PASSWORD, WIFI_USER, WIFI_IS_ENT);
        Serial.println("[WiFi] Berhasil! Kredensial tersimpan ke EEPROM.");
        if (currentScreen == SCR_MAIN) refreshMainWiFiStatus();
        else drawWiFiScreen();
    } else {
        Serial.println("[WiFi] Koneksi gagal. Kredensial tidak disimpan.");
    }
    Serial.println("=========================================\n");
}

void serialMenu() {
    if (!Serial.available()) return;
    char cmd=Serial.read();
    if (cmd=='\n'||cmd=='\r') return;

    switch(cmd) {
        // ---- Braille dots ----
        case '1': case '2': case '3':
        case '4': case '5': case '6':
            handleButtonPress(cmd-'1');
            Serial.print("[Serial] Titik "); Serial.print(cmd); Serial.println(" toggle");
            break;
        // ---- Aksi ----
        case 'e': case 'E':
            Serial.println("[Serial] ENTER");
            handleButtonPress(6); break;
        case 's': case 'S':
            Serial.println("[Serial] SPASI");
            handleButtonPress(7); break;
        case 'x': case 'X':
            Serial.println("[Serial] HAPUS");
            handleButtonPress(8); break;
        case 'm': case 'M':
            Serial.println("[Serial] GANTI MODE");
            handleButtonPress(9); break;
        // ---- Audio ----
        case '!':
            Serial.println("[Serial] Test Audio...");
            if (WiFi.status()==WL_CONNECTED) {
                audio.setVolume(21);
                speak("Braille siap digunakan");
            } else Serial.println("[Serial] WiFi tidak terhubung, audio dilewati.");
            break;
        // ---- Status ----
        case '@': {
            Serial.println("\n======== STATUS PERANGKAT ========");
            if (WiFi.status()==WL_CONNECTED) {
                Serial.println("WiFi     : Terhubung");
                Serial.print("SSID     : "); Serial.println(WIFI_SSID);
                Serial.print("IP       : "); Serial.println(WiFi.localIP());
                Serial.print("RSSI     : "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
            } else {
                Serial.println("WiFi     : Tidak terhubung");
            }
            String sv,sp,su; bool se;
            Serial.print("EEPROM   : ");
            Serial.println(eepromLoadWiFi(sv,sp,su,se)?sv:"(kosong)");
            Serial.print("Mode     : ");
            const char* mn[]={"Huruf","Angka","Kata"};
            Serial.println(mn[currentMode]);
            Serial.print("Pola     : 0b");
            for (int i=5;i>=0;i--) Serial.print((currentPattern>>i)&1);
            Serial.println();
            Serial.print("Layar    : ");
            Serial.println(currentScreen==SCR_MAIN?"Main":"WiFi");
            Serial.println("==================================");
            break;
        }
        // ---- WiFi input ----
        case 'w': case 'W': inputWiFiSerial(); break;
        // ---- CLR ----
        case 'c': case 'C': {
            delay(50);
            String rest="";
            while (Serial.available()) {
                char ch=Serial.read();
                if (ch=='\n'||ch=='\r') break;
                rest+=ch; delay(2);
            }
            rest.toUpperCase();
            if (rest=="LR") {
                eepromClearWiFi();
                WIFI_SSID=WIFI_PASSWORD=WIFI_USER=""; WIFI_IS_ENT=false;
                WiFi.disconnect(true);
                if (currentScreen==SCR_MAIN) refreshMainWiFiStatus();
                else drawWiFiScreen();
            } else {
                Serial.println("[Serial] Perintah tidak dikenal. Maksud CLR?");
            }
            break;
        }
    }
}

// ============================================================
// AUDIO
// ============================================================
void initAudio() {
    Serial.println("[Audio] Init I2S...");
    audio.setPinout(I2S_BCLK,I2S_LRC,I2S_DOUT);
    audio.setVolume(100);
    Serial.print("[Audio] BCLK="); Serial.print(I2S_BCLK);
    Serial.print(" LRC="); Serial.print(I2S_LRC);
    Serial.print(" DATA="); Serial.println(I2S_DOUT);
    delay(800);
}

void speak(const String &text) {
    if (WiFi.status() == WL_CONNECTED) {
        // Kecepatan suara bisa diatur lewat parameter URL
        audio.connecttospeech(text.c_str(), getTtsSpeedParam().c_str());
    }
}

// ============================================================
// BUTTON HANDLER
// ============================================================
void handleButtonPress(int pin) {
    if (pin>=0 && pin<=5) {
        currentPattern^=(1<<pin);
        Serial.print("[Braille] Titik "); Serial.print(pin+1);
        Serial.println((currentPattern&(1<<pin))?" ON":" OFF");
        Serial.print("[Braille] Pola: 0b");
        for (int i=5;i>=0;i--) Serial.print((currentPattern>>i)&1);
        Serial.println();
        if (currentScreen==SCR_MAIN) {
            int dotR=22, dotGapX=78, dotGapY=58, sX=55, sY=148;
            for (int i=0;i<6;i++) {
                int col=i/3, row=i%3;
                int cx=sX+col*dotGapX, cy=sY+row*dotGapY;
                bool act=(currentPattern>>i)&1;
                tft.fillCircle(cx,cy,dotR,act?C_ACCENT:C_DGRAY);
                tft.drawCircle(cx,cy,dotR,act?C_WHITE:C_GRAY);
                tft.setFont(); tft.setTextSize(2);
                tft.setTextColor(act?C_BG:C_GRAY,act?C_ACCENT:C_DGRAY);
                tft.setCursor(cx-5,cy-8); tft.print(i+1);
            }
        }
    }
    else if (pin==6) { // ENTER
        if (currentPattern==0) { Serial.println("[Braille] Pola kosong, abaikan."); return; }
        char c=decodeBraille(currentPattern,(currentMode==1));
        if (c=='?') {
            Serial.println("[Braille] Pola tidak dikenal!");
            speak("Pola salah");
        } else {
            Serial.print("[Braille] Karakter: "); Serial.println(c);
            if (currentMode==0||currentMode==1) {
                lastChar = c;
                speak(String(c));
                if (currentScreen==SCR_MAIN) drawMainScreen();
            } else if (currentMode==2) {
                currentWord+=c;
                Serial.print("[Braille] Kata: "); Serial.println(currentWord);
                speak(String(c));
                if (currentScreen==SCR_MAIN) drawMainScreen(); // redraw full to clear transparent font overlap
            }
        }
        currentPattern=0;
    }
    else if (pin==7) { // SPASI
        if (currentMode==2 && currentWord.length()>0) {
            Serial.print("[Braille] Spasi. Ucapkan kata: "); Serial.println(currentWord);
            speak(currentWord);
            currentWord="";
            if (currentScreen==SCR_MAIN) drawMainScreen();
        } else {
            Serial.println("[Braille] Spasi hanya di mode Kata.");
        }
    }
    else if (pin==8) { // HAPUS
        if (currentMode==2 && currentWord.length()>0) {
            currentWord.remove(currentWord.length()-1);
            Serial.print("[Braille] Hapus. Kata: "); Serial.println(currentWord);
            speak("Hapus");
            if (currentScreen==SCR_MAIN) drawMainScreen();
        }
        currentPattern=0;
    }
    else if (pin==9) { // GANTI MODE
        currentMode=(currentMode+1)%3;
        currentPattern=0; currentWord=""; lastChar='-';
        const char* mn[]={"Mode Huruf","Mode Angka","Mode Kata"};
        Serial.print("[Braille] "); Serial.println(mn[currentMode]);
        speak(mn[currentMode]);
        if (currentScreen==SCR_MAIN) drawMainScreen();
    }
}

void checkButtons() {
    uint16_t readState=mcp1.readGPIOAB();
    if (readState!=lastFlickerState) { lastDebounceTime=millis(); lastFlickerState=readState; }
    if ((millis()-lastDebounceTime)>50) {
        if (readState!=lastButtonState) {
            for (int i=0;i<10;i++) {
                bool isP=!(readState&(1<<i));
                bool wasP=!(lastButtonState&(1<<i));
                if (isP&&!wasP) handleButtonPress(i);
            }
            lastButtonState=readState;
        }
    }
}

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
    bool wOK=autoConnectWiFi();
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
    Serial.println(" M     = Ganti Mode (Huruf/Angka/Kata)");
    Serial.println(" W     = Input WiFi (SCAN/Enterprise/Personal)");
    Serial.println(" CLR   = Hapus WiFi dari EEPROM");
    Serial.println(" @     = Tampilkan status lengkap");
    Serial.println(" !     = Tes suara TTS");
    Serial.println("================================\n");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
    audio.loop();
    checkButtons();
    handleTouch();
    serialMenu();
}
