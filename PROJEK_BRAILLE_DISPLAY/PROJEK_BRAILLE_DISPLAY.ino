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

const char* WIFI_SSID="NamaWifi";
const char* WIFI_PASSWORD="PasswordWifi";



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

void connectWiFi()
{

    lcdCenter("Connecting WiFi");

    WiFi.mode(WIFI_STA);

    WiFi.begin(WIFI_SSID,WIFI_PASSWORD);

    unsigned long t=millis();

    while(WiFi.status()!=WL_CONNECTED)
    {

        delay(300);

        Serial.print(".");

        if(millis()-t>20000)
        {

            Serial.println();

            Serial.println("WiFi Failed");

            lcdCenter("WiFi Failed");

            return;

        }

    }

    Serial.println();

    Serial.println(WiFi.localIP());

    lcdCenter("WiFi Connected");

}



//======================================================
// AUDIO
//======================================================

void initAudio()
{

    audio.setPinout(
        I2S_BCLK,
        I2S_LRC,
        I2S_DOUT
    );

    audio.setVolume(18);

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

void initMCP()
{

    Wire.begin(
        SDA_PIN,
        SCL_PIN
    );

    if(!mcp1.begin_I2C(0x20))
    {

        Serial.println("MCP1 ERROR");

    }

    if(!mcp2.begin_I2C(0x21))
    {

        Serial.println("MCP2 ERROR");

    }

    for(int i=0;i<16;i++)
    {

        mcp1.pinMode(i,INPUT_PULLUP);

        mcp2.pinMode(i,INPUT_PULLUP);

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
// SETUP
//======================================================

void setup()
{

    Serial.begin(115200);

    initLCD();

    connectWiFi();

    initAudio();

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