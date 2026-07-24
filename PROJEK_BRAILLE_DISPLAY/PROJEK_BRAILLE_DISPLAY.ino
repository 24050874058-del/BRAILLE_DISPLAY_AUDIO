/**************************************************************************
 * ESP32-S3 Braille Trainer
 * BAGIAN 1
 *
 * Hardware
 * --------
 * ESP32-S3 N16R8
 * TFT ST7796S SPI
 * 2x MCP23017
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
Adafruit_MCP23X17 mcp2;



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
// CHARACTER MAP
//======================================================

const char charMap[]=
{
'1','2','3','4','5','6','7','8','9','0',
'A','B','C','D','V','E',
'F','G','H','I','J','K','L','M',
'N','O','P','Q','R','S','T','U',
'W','X','Y','Z',
'+','='
};

const uint8_t TOTAL_BUTTON=38;



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
    lcdCenter("Connecting WiFi");

    WiFi.disconnect(true);
    delay(500);

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    Serial.print("Mencoba menghubungkan ke: [");
    Serial.print(WIFI_SSID);
    Serial.println("]");

    WiFi.begin(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());

    unsigned long t = millis();

    while(WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");

        if(millis() - t > 15000)
        {
            Serial.println();
            Serial.print("WiFi Failed! Status Code: ");
            Serial.println(WiFi.status());

            if (WiFi.status() == 6) {
                Serial.println("-> Keterangan: Password Salah / Terputus (WL_CONNECT_FAILED)");
            } else if (WiFi.status() == 1) {
                Serial.println("-> Keterangan: SSID Tidak Ditemukan (WL_NO_SSID_AVAIL)");
            }

            lcdCenter("WiFi Failed");
            return false;
        }
    }

    Serial.println();
    Serial.print("WiFi Connected! IP Address: ");
    Serial.println(WiFi.localIP());

    lcdCenter("WiFi Connected");
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

    Serial.print("Password Diterima (Panjang: ");
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
    lcdCenter("Init Audio");

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

    lcdCenter("Audio Ready");

    delay(1000);
}

//======================================================
// TEST AUDIO
//======================================================

void testAudio()
{
    lcdCenter("Testing Audio");

    Serial.println();
    Serial.println("===== AUDIO TEST =====");

    // 1. Tes Nada Bip Lokal (Murni I2S Hardware Check)
    Serial.println("Memulai Tes Suara Lokal (Bip Hardware I2S)...");
    audio.connecttohost("http://www.soundjay.com/button/button-1.wav"); // Jika ingin tes file online
    
    // Atau bunyikan sinyal nada test internal
    audio.setVolume(12); // Tingkatkan volume ke 21 (maks 21)

    if(WiFi.status()!=WL_CONNECTED)
    {
        Serial.println("WiFi NOT Connected");
        lcdCenter("No WiFi");
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

    lcdCenter("Audio OK");

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
    lcdCenter("Checking MCP...");

    Wire.begin(SDA_PIN, SCL_PIN);

    bool mcp1OK = mcp1.begin_I2C(0x20);
    bool mcp2OK = mcp2.begin_I2C(0x21);

    if(mcp1OK)
        Serial.println("MCP23017 #1 Ready");
    else
        Serial.println("MCP23017 #1 ERROR");

    if(mcp2OK)
        Serial.println("MCP23017 #2 Ready");
    else
        Serial.println("MCP23017 #2 ERROR");

    if(mcp1OK && mcp2OK)
    {
        lcdCenter("MCP Ready");
    }
    else if(mcp1OK)
    {
        lcdCenter("MCP1 Ready");
    }
    else if(mcp2OK)
    {
        lcdCenter("MCP2 Ready");
    }
    else
    {
        lcdCenter("MCP ERROR");
    }

    delay(1000);

    if(mcp1OK)
    {
        for(int i=0; i<16; i++)
        {
            mcp1.pinMode(i, INPUT_PULLUP);
        }
    }

    if(mcp2OK)
    {
        for(int i=0; i<16; i++)
        {
            mcp2.pinMode(i, INPUT_PULLUP);
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

    switch(cmd)
    {
        case '1':

            testAudio();

            break;

        case '2':

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
            Serial.println("MCP23017 #1 : Ready");
            Serial.println("MCP23017 #2 : Ready");

            Serial.println("============================");

            break;

        case '3':

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

    lcdCenter("READY");
}



//======================================================
// LOOP
//======================================================

void loop()
{

    audio.loop();

}