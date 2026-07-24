/**************************************************************************
 * ESP32-S3 Braille Trainer
 * BAGIAN 1
 *
 * Hardware
 * --------
 * ESP32-S3 N16R8
 * TFT ST7796S SPI
 * MCP23017
 * MAX98357A


 tesssssss
 **************************************************************************/

#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S.h>
#include <Fonts/FreeSansBold18pt7b.h>

#include <Adafruit_MCP23X17.h>

#include "Audio.h"



//======================================================
// WIFI
//======================================================

String WIFI_SSID = "";
String WIFI_PASSWORD = "";



//======================================================
// TFT
//======================================================

#define TFT_CS      15
#define TFT_DC       2
#define TFT_RST      4

#define TFT_SCLK    18
#define TFT_MOSI    11
#define TFT_MISO    13

#define TFT_BL_PIN  10

Adafruit_ST7796S tft(TFT_CS,TFT_DC,TFT_RST);



//======================================================
// I2C
//======================================================

#define SDA_PIN      8
#define SCL_PIN      9



//======================================================
// AUDIO
//======================================================

#define I2S_BCLK     5
#define I2S_LRC      6
#define I2S_DOUT     7

Audio audio;



//======================================================
// MCP23017
//======================================================

Adafruit_MCP23X17 mcp1;



//======================================================
// GPIO EXTRA
//======================================================

const uint8_t extraGPIO[6]=
{
    1,
    3,
    16,
    17,
    21,
    22
};



//======================================================
// BRAILLE STATE & LOGIC
//======================================================

uint8_t currentPattern = 0;
uint8_t currentMode = 0; // 0=Huruf, 1=Angka, 2=Kata
String currentWord = "";

uint16_t lastButtonState = 0xFFFF; // MCP23017 is pulled high
uint16_t lastFlickerState = 0xFFFF;
unsigned long lastDebounceTime = 0;

char decodeBraille(uint8_t pattern, bool isNumber) {
    if (isNumber) {
        switch(pattern) {
            case 0b000001: return '1';
            case 0b000011: return '2';
            case 0b001001: return '3';
            case 0b011001: return '4';
            case 0b010001: return '5';
            case 0b001011: return '6';
            case 0b011011: return '7';
            case 0b010011: return '8';
            case 0b001010: return '9';
            case 0b011010: return '0';
            default: return '?';
        }
    } else {
        switch(pattern) {
            case 0b000001: return 'a';
            case 0b000011: return 'b';
            case 0b001001: return 'c';
            case 0b011001: return 'd';
            case 0b010001: return 'e';
            case 0b001011: return 'f';
            case 0b011011: return 'g';
            case 0b010011: return 'h';
            case 0b001010: return 'i';
            case 0b011010: return 'j';
            case 0b000101: return 'k';
            case 0b000111: return 'l';
            case 0b001101: return 'm';
            case 0b011101: return 'n';
            case 0b010101: return 'o';
            case 0b001111: return 'p';
            case 0b011111: return 'q';
            case 0b010111: return 'r';
            case 0b001110: return 's';
            case 0b011110: return 't';
            case 0b100101: return 'u';
            case 0b100111: return 'v';
            case 0b111010: return 'w';
            case 0b101101: return 'x';
            case 0b111101: return 'y';
            case 0b110101: return 'z';
            default: return '?';
        }
    }
}



//======================================================
// LCD
//======================================================

void lcdCenter(String txt)
{

    tft.fillScreen(ST77XX_BLACK);

    tft.setTextColor(ST77XX_WHITE);

    tft.setTextWrap(false);

    uint8_t size=2;

    if(txt.length()==1)
        size=8;
    else if(txt.length()<5)
        size=6;
    else if(txt.length()<10)
        size=4;
    else
        size=2;

    tft.setFont();

    tft.setTextSize(size);

    int16_t x1,y1;

    uint16_t w,h;

    tft.getTextBounds(txt,0,0,&x1,&y1,&w,&h);

    int16_t x=(tft.width()-w)/2;

    int16_t y=(tft.height()-h)/2;

    tft.setCursor(x,y);

    tft.print(txt);

}



//======================================================
// WIFI
//======================================================

bool connectWiFi()
{
    WiFi.disconnect(true);
    delay(500);

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    Serial.print("Mencoba menghubungkan ke: [");
    Serial.print(WIFI_SSID);
    Serial.println("]");

    if (WIFI_PASSWORD.length() == 0 || WIFI_PASSWORD == "-") {
        WiFi.begin(WIFI_SSID.c_str());
    } else {
        WiFi.begin(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());
    }

    unsigned long t = millis();

    while(WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");

        if(millis() - t > 20000)
        {
            Serial.println();
            Serial.print("WiFi Failed! Status Code: ");
            Serial.println(WiFi.status());

            if (WiFi.status() == 6) {
                Serial.println("-> Keterangan: Password Salah / Terputus (WL_CONNECT_FAILED)");
            } else if (WiFi.status() == 1) {
                Serial.println("-> Keterangan: SSID Tidak Ditemukan (WL_NO_SSID_AVAIL)");
            }

            return false;
        }
    }

    Serial.println();
    Serial.print("WiFi Connected! IP Address: ");
    Serial.println(WiFi.localIP());

    return true;
}

void inputWiFi()
{
    Serial.println();
    Serial.println("================================");
    Serial.println("      INPUT WIFI ESP32");
    Serial.println("================================");

    // Flus/kosongkan sisa masukan serial sebelumnya
    while (Serial.available()) { Serial.read(); delay(2); }

    Serial.println("Masukkan SSID WiFi:");
    while (!Serial.available()) { delay(10); }

    WIFI_SSID = Serial.readStringUntil('\n');
    WIFI_SSID.replace("\r", "");
    WIFI_SSID.trim();

    Serial.print("SSID Diterima: [");
    Serial.print(WIFI_SSID);
    Serial.println("]");

    // Flus/kosongkan sisa masukan serial
    while (Serial.available()) { Serial.read(); delay(2); }

    Serial.println("Masukkan Password:");
    while (!Serial.available()) { delay(10); }

    WIFI_PASSWORD = Serial.readStringUntil('\n');
    WIFI_PASSWORD.replace("\r", "");
    WIFI_PASSWORD.trim();

    Serial.print("Password Diterima: [");
    Serial.print(WIFI_PASSWORD);
    Serial.print("] (Panjang: ");
    Serial.print(WIFI_PASSWORD.length());
    Serial.println(" karakter).");
}



//======================================================
// AUDIO
//======================================================

//======================================================
// AUDIO PCM5102A
//======================================================

void initAudio()
{
    Serial.println();
    Serial.println("================================");
    Serial.println(" PCM5102A + PAM8403 ");
    Serial.println("================================");

    audio.setPinout(
        I2S_BCLK,
        I2S_LRC,
        I2S_DOUT
    );

    audio.setVolume(100);

    Serial.println("I2S Initialized");
    Serial.print("BCLK : GPIO ");
    Serial.println(I2S_BCLK);

    Serial.print("LRCK : GPIO ");
    Serial.println(I2S_LRC);

    Serial.print("DATA : GPIO ");
    Serial.println(I2S_DOUT);

    delay(1000);
}

//======================================================
// TEST AUDIO
//======================================================

void testAudio()
{
    Serial.println();
    Serial.println("===== AUDIO TEST =====");

    // 1. Tes Nada Bip Lokal (Murni I2S Hardware Check)
    Serial.println("Memulai Tes Suara Lokal (Bip Hardware I2S)...");
    audio.connecttohost("http://www.soundjay.com/button/button-1.wav"); // Jika ingin tes file online
    
    // Atau bunyikan sinyal nada test internal
    audio.setVolume(21); // Tingkatkan volume ke 21 (maks 21)

    if(WiFi.status()!=WL_CONNECTED)
    {
        Serial.println("WiFi NOT Connected");
        delay(1000);
        return;
    }

    Serial.println("Sending Google TTS...");

    audio.connecttospeech("Braiile ready", "id");

    unsigned long timeout = millis();

    while(audio.isRunning())
    {
        audio.loop();

        if(millis()-timeout>15000)
        {
            Serial.println("Audio Timeout");

            audio.stopSong();

            break;
        }
    }

    Serial.println("Audio Test Finished");

    delay(1000);
}

//======================================================
// LCD
//======================================================

void initLCD()
{

    pinMode(TFT_BL_PIN,OUTPUT);

    digitalWrite(TFT_BL_PIN,HIGH);

    SPI.begin(
        TFT_SCLK,
        TFT_MISO,
        TFT_MOSI,
        TFT_CS
    );

    tft.init(
        320,
        480,
        0,
        0,
        ST7796S_RGB
    );

    tft.setRotation(0);

    lcdCenter("Braille");

}



//======================================================
// MCP
//======================================================

//======================================================
// MCP23017
//======================================================

void initMCP()
{
    Serial.println("Checking MCP...");

    Wire.begin(SDA_PIN, SCL_PIN);

    bool mcp1OK = mcp1.begin_I2C(0x20);

    if(mcp1OK)
        Serial.println("MCP23017 Ready");
    else
        Serial.println("MCP23017 ERROR");

    delay(1000);

    if(mcp1OK)
    {
        for(int i=0; i<16; i++)
        {
            mcp1.pinMode(i, INPUT_PULLUP);
        }
    }
}



//======================================================
// EXTRA GPIO
//======================================================

void initGPIO()
{

    for(int i=0;i<6;i++)
    {

        pinMode(extraGPIO[i],INPUT_PULLUP);

    }

}

//======================================================
// SERIAL MENU
//======================================================

void serialMenu()
{
    if(!Serial.available())
        return;

    char cmd = Serial.read();

    // Abaikan enter/newline
    if (cmd == '\n' || cmd == '\r') return;

    switch(cmd)
    {
        // --- SIMULASI TOMBOL BRAILLE ---
        case '1': handleButtonPress(0); break; // Titik 1
        case '2': handleButtonPress(1); break; // Titik 2
        case '3': handleButtonPress(2); break; // Titik 3
        case '4': handleButtonPress(3); break; // Titik 4
        case '5': handleButtonPress(4); break; // Titik 5
        case '6': handleButtonPress(5); break; // Titik 6
        case 'e': 
        case 'E': handleButtonPress(6); break; // ENTER
        case 's': 
        case 'S': handleButtonPress(7); break; // SPASI
        case 'x': 
        case 'X': handleButtonPress(8); break; // HAPUS
        case 'm': 
        case 'M': handleButtonPress(9); break; // GANTI MODE

        // --- MENU DEBUGGING LAMA ---
        case '!':
            testAudio();
            break;

        case '@':
            Serial.println();
            Serial.println("========== STATUS ==========");

            if(WiFi.status()==WL_CONNECTED)
            {
                Serial.println("WiFi        : Connected");
                Serial.print("IP Address  : ");
                Serial.println(WiFi.localIP());
            }
            else
            {
                Serial.println("WiFi        : Disconnected");
            }

            Serial.println("LCD         : OK");
            Serial.println("PCM5102A    : I2S Initialized");
            Serial.println("PAM8403     : Connected");
            Serial.println("MCP23017    : Ready");

            Serial.println("============================");
            break;

        case '#':
            Serial.println();
            Serial.println("Playing Test Voice...");
            audio.connecttospeech("Selamat datang pada alat braille elektronik", "id");
            break;
    }
}


//======================================================
// SETUP
//======================================================

void setup()
{
    Serial.begin(115200);

    while(!Serial);

    delay(100);

    initLCD();

    // Loop input & koneksi WiFi sampai berhasil
    while (true)
    {
        inputWiFi();
        if (connectWiFi())
        {
            break; // Jika terkoneksi, keluar dari loop
        }
        Serial.println("\n[!] WiFi Gagal Terkoneksi. Silakan masukkan SSID & Password kembali.");
        delay(1000);
    }

    initAudio();

    testAudio();

    initMCP();

    initGPIO();

    lcdCenter("Siap Digunakan");
}



//======================================================
// BUTTON LOGIC
//======================================================

void handleButtonPress(int pin) {
    if (pin >= 0 && pin <= 5) {
        // Titik 1-6 ditekan
        currentPattern ^= (1 << pin); // Toggle titik
        Serial.print("Titik ");
        Serial.print(pin + 1);
        if (currentPattern & (1 << pin)) Serial.println(" AKTIF");
        else Serial.println(" MATI");
    }
    else if (pin == 6) {
        // ENTER
        if (currentPattern == 0) return;

        char c = decodeBraille(currentPattern, (currentMode == 1));
        
        if (c == '?') {
            Serial.println("Pola salah!");
            audio.connecttospeech("Pola salah", "id");
        } else {
            Serial.print("Karakter Terbaca: ");
            Serial.println(c);
            
            if (currentMode == 0 || currentMode == 1) {
                String toSpeak = String(c);
                audio.connecttospeech(toSpeak.c_str(), "id");
            } 
            else if (currentMode == 2) {
                currentWord += c;
                Serial.print("Kata saat ini: ");
                Serial.println(currentWord);
                String toSpeak = String(c);
                audio.connecttospeech(toSpeak.c_str(), "id");
            }
        }
        currentPattern = 0; // Reset pola setelah enter
    }
    else if (pin == 7) {
        // SPASI (khusus mode kata)
        if (currentMode == 2) {
            Serial.print("Spasi! Membaca kata: ");
            Serial.println(currentWord);
            if (currentWord.length() > 0) {
                audio.connecttospeech(currentWord.c_str(), "id");
                currentWord = ""; 
            }
        } else {
            Serial.println("Spasi hanya untuk mode kata.");
        }
    }
    else if (pin == 8) {
        // HAPUS (Backspace)
        if (currentMode == 2 && currentWord.length() > 0) {
            currentWord.remove(currentWord.length() - 1);
            Serial.print("Hapus 1 huruf. Kata saat ini: ");
            Serial.println(currentWord);
            audio.connecttospeech("Hapus", "id");
        }
        currentPattern = 0; 
    }
    else if (pin == 9) {
        // GANTI MODE
        currentMode++;
        if (currentMode > 2) currentMode = 0;
        
        currentPattern = 0;
        currentWord = "";
        
        if (currentMode == 0) {
            Serial.println("MODE HURUF");
            audio.connecttospeech("Mode Huruf", "id");
        } else if (currentMode == 1) {
            Serial.println("MODE ANGKA");
            audio.connecttospeech("Mode Angka", "id");
        } else if (currentMode == 2) {
            Serial.println("MODE KATA");
            audio.connecttospeech("Mode Kata", "id");
        }
    }
}

void checkButtons() {
    uint16_t readState = mcp1.readGPIOAB();
    
    if (readState != lastFlickerState) {
        lastDebounceTime = millis();
        lastFlickerState = readState;
    }

    if ((millis() - lastDebounceTime) > 50) {
        if (readState != lastButtonState) {
            for (int i = 0; i < 10; i++) {
                bool isPressed = !(readState & (1 << i));
                bool wasPressed = !(lastButtonState & (1 << i));
                if (isPressed && !wasPressed) {
                    handleButtonPress(i);
                }
            }
            lastButtonState = readState;
        }
    }
}

//======================================================
// LOOP
//======================================================

void loop()
{
    audio.loop();
    checkButtons();
    serialMenu();
}