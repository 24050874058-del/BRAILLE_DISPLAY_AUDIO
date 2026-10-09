// ============================================================
// SLAVE_BRAILLE_NANO.ino (Relay Active LOW Version)
// Arduino Nano sebagai Slave Controller Solenoid Braille
// ============================================================

#include <SoftwareSerial.h>

// PIN SOLENOID (Dot 1 s/d Dot 6)
const byte solenoidPins[6] = {2, 3, 4, 5, 6, 7};

// SERIAL & TIMEOUT
SoftwareSerial MASTER_SERIAL(10, 11); 

#define BAUD_RATE 9600
#define WATCHDOG_TIMEOUT 5000  // 5 detik tanpa komunikasi -> matikan solenoid

String rxBuffer = "";
unsigned long lastCommandTime = 0;

// Forward declarations
void processCommand(const String& cmd);
void allSolenoidsOff();
void applyPattern(const byte pattern[6]);

void setup() {
    // Debug port (USB Serial Monitor Laptop)
    Serial.begin(115200);
    delay(200);
    Serial.println(F("========================================="));
    Serial.println(F(" BRAILLE SLAVE CONTROLLER (Arduino Nano) "));
    Serial.println(F("========================================="));
    Serial.println(F("Baud SoftwareSerial (D10/D11): 9600"));
    Serial.println(F("Menunggu perintah dari Master..."));

    // Inisialisasi pin relay - HIGH berarti OFF untuk relay Active LOW
    for (byte i = 0; i < 6; i++) {
        pinMode(solenoidPins[i], OUTPUT);
        digitalWrite(solenoidPins[i], HIGH);
    }
    Serial.println(F("Solenoid (Relay): OK (semua OFF)"));

    // Inisialisasi komunikasi ke ESP32-S3
    MASTER_SERIAL.begin(BAUD_RATE);
    delay(100);

    // Kirim sinyal Ready ke Master
    MASTER_SERIAL.print("RDY\n");
    Serial.println(F("Terkirim: RDY ke Master"));

    lastCommandTime = millis();
}

void loop() {
    // Proses input dari Master
    while (MASTER_SERIAL.available() > 0) {
        char c = (char)MASTER_SERIAL.read();

        // Echo byte masuk ke Serial Monitor untuk debug
        Serial.print(F("[RAW] 0x"));
        Serial.print((byte)c, HEX);
        Serial.print(F(" '"));
        if (c >= 32 && c < 127) Serial.print(c);
        else Serial.print('?');
        Serial.println(F("'"));

        if (c == '\n') {
            rxBuffer.trim();
            if (rxBuffer.length() > 0) {
                Serial.print(F("[CMD] '"));
                Serial.print(rxBuffer);
                Serial.print(F("' len="));
                Serial.println(rxBuffer.length());
                processCommand(rxBuffer);
            }
            rxBuffer = "";
        } else if (c != '\r') {
            rxBuffer += c;
            if (rxBuffer.length() > 32) {
                Serial.println(F("[WARN] Buffer overflow, reset."));
                rxBuffer = "";
            }
        }
    }

    // Watchdog Timer: jika 5 detik tidak ada perintah, matikan relay
    if (millis() - lastCommandTime >= WATCHDOG_TIMEOUT) {
        bool anyOn = false;
        for (byte i = 0; i < 6; i++) {
            // Pada Relay Active LOW, pin berlogika LOW menandakan relay sedang aktif (ON)
            if (digitalRead(solenoidPins[i]) == LOW) {
                anyOn = true;
                break;
            }
        }

        if (anyOn) {
            allSolenoidsOff();
            Serial.println(F("[Watchdog] Timeout! Mematikan semua solenoid."));
        }

        lastCommandTime = millis(); // Reset agar tidak terus print
    }
}

void processCommand(const String& cmd) {
    lastCommandTime = millis(); // Reset watchdog

    if (cmd == "PING") {
        MASTER_SERIAL.print("PONG\n");
        Serial.println(F("  -> PONG"));
        return;
    }

    if (cmd == "OFF") {
        allSolenoidsOff();
        MASTER_SERIAL.print("ACK\n");
        Serial.println(F("  -> ACK (Solenoid Mati)"));
        return;
    }

    // Format BS:PPPPPP
    if (cmd.startsWith("BS:") && cmd.length() >= 9) {
        bool valid = true;
        byte pattern[6] = {0};

        for (int i = 0; i < 6; i++) {
            char p = cmd.charAt(3 + i);
            if (p == '1') {
                pattern[i] = 1;
            } else if (p == '0') {
                pattern[i] = 0;
            } else {
                valid = false;
                Serial.print(F("  -> Invalid char at pos "));
                Serial.print(3 + i);
                Serial.print(F(": 0x"));
                Serial.println((byte)p, HEX);
                break;
            }
        }

        if (valid) {
            applyPattern(pattern);
            MASTER_SERIAL.print("ACK\n");
            Serial.print(F("  -> ACK (Pola: "));
            for (int i = 0; i < 6; i++) Serial.print(pattern[i]);
            Serial.println(F(")"));
        } else {
            MASTER_SERIAL.print("ERR:INV\n");
            Serial.println(F("  -> ERR:INV"));
        }
        return;
    }

    if (cmd.startsWith("TM:")) {
        MASTER_SERIAL.print("ACK\n");
        Serial.print(F("  -> ACK (Timing: "));
        Serial.print(cmd);
        Serial.println(F(")"));
        return;
    }

    // Perintah tidak dikenal
    MASTER_SERIAL.print("ERR:INV\n");
    Serial.print(F("  -> ERR:INV (Unknown: '"));
    Serial.print(cmd);
    Serial.println(F("')"));
}

void allSolenoidsOff() {
    for (byte i = 0; i < 6; i++) {
        digitalWrite(solenoidPins[i], HIGH); // HIGH = Relay OFF
    }
    Serial.println(F("[Solenoid] Semua OFF"));
}

void applyPattern(const byte pattern[6]) {
    for (byte i = 0; i < 6; i++) {
        // pattern 1 -> LOW (Relay ON)
        // pattern 0 -> HIGH (Relay OFF)
        digitalWrite(solenoidPins[i], pattern[i] ? LOW : HIGH);
    }
}