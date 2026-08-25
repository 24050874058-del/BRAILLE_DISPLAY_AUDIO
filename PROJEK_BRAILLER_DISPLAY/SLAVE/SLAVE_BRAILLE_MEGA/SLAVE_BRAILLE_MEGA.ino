// ============================================================
// SLAVE_BRAILLE_MEGA.ino
// Arduino Mega 2560 sebagai Slave Controller Solenoid Braille
// ============================================================
// Peran: Menerima perintah dari Master (ESP32-S3) via Serial1
// PIN SERIAL1: RX=D19, TX=D18
// ============================================================

// PIN SOLENOID
const byte solenoidPins[6] = {
    2,  // Dot 1
    3,  // Dot 2
    4,  // Dot 3
    5,  // Dot 4
    6,  // Dot 5
    7   // Dot 6
};

// SERIAL & TIMEOUT
#define MASTER_SERIAL Serial1  // RX=D19, TX=D18
#define BAUD_RATE 9600
#define WATCHDOG_TIMEOUT 5000  // 5 detik tanpa komunikasi -> matikan solenoid

String rxBuffer = "";
unsigned long lastCommandTime = 0;

// Forward declarations
void processCommand(const String& cmd);
void allSolenoidsOff();
void applyPattern(const byte pattern[6]);

void setup() {
    // Debug port (USB) - monitor di komputer
    Serial.begin(115200);
    delay(200);
    Serial.println("=========================================");
    Serial.println(" BRAILLE SLAVE CONTROLLER (Arduino Mega) ");
    Serial.println("=========================================");
    Serial.println("Baud Serial1: 9600");
    Serial.println("Menunggu perintah dari Master...");

    // Inisialisasi pin solenoid - semua OFF saat boot
    for (byte i = 0; i < 6; i++) {
        pinMode(solenoidPins[i], OUTPUT);
        digitalWrite(solenoidPins[i], LOW);
    }
    Serial.println("Solenoid: OK (semua OFF)");

    // Inisialisasi Serial1 ke Master
    MASTER_SERIAL.begin(BAUD_RATE);
    delay(100);

    // Kirim sinyal Ready ke Master
    MASTER_SERIAL.print("RDY\n");
    Serial.println("Terkirim: RDY ke Master");

    lastCommandTime = millis();
}

void loop() {
    // Proses input dari Master
    while (MASTER_SERIAL.available() > 0) {
        char c = (char)MASTER_SERIAL.read();

        // Echo byte masuk ke Serial Monitor untuk debug
        Serial.print("[RAW] 0x");
        Serial.print((byte)c, HEX);
        Serial.print(" '");
        if (c >= 32 && c < 127) Serial.print(c);
        else Serial.print('?');
        Serial.println("'");

        if (c == '\n') {
            rxBuffer.trim();
            if (rxBuffer.length() > 0) {
                Serial.print("[CMD] '");
                Serial.print(rxBuffer);
                Serial.print("' len=");
                Serial.println(rxBuffer.length());
                processCommand(rxBuffer);
            }
            rxBuffer = "";
        } else if (c != '\r') {
            rxBuffer += c;
            if (rxBuffer.length() > 32) {
                Serial.println("[WARN] Buffer overflow, reset.");
                rxBuffer = "";
            }
        }
    }

    // Watchdog Timer: jika 5 detik tidak ada perintah, matikan solenoid
    if (millis() - lastCommandTime >= WATCHDOG_TIMEOUT) {
        bool anyOn = false;
        for (byte i = 0; i < 6; i++) {
            if (digitalRead(solenoidPins[i]) == HIGH) {
                anyOn = true;
                break;
            }
        }

        if (anyOn) {
            allSolenoidsOff();
            Serial.println("[Watchdog] Timeout! Mematikan semua solenoid.");
        }

        lastCommandTime = millis(); // Reset agar tidak terus print
    }
}

void processCommand(const String& cmd) {
    lastCommandTime = millis(); // Reset watchdog

    if (cmd == "PING") {
        MASTER_SERIAL.print("PONG\n");
        Serial.println("  -> PONG");
        return;
    }

    if (cmd == "OFF") {
        allSolenoidsOff();
        MASTER_SERIAL.print("ACK\n");
        Serial.println("  -> ACK (Solenoid Mati)");
        return;
    }

    // Format BS:PPPPPP - minimal 9 karakter (>= bukan == agar trim tidak menyebabkan gagal)
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
                Serial.print("  -> Invalid char at pos ");
                Serial.print(3 + i);
                Serial.print(": 0x");
                Serial.println((byte)p, HEX);
                break;
            }
        }

        if (valid) {
            applyPattern(pattern);
            MASTER_SERIAL.print("ACK\n");
            Serial.print("  -> ACK (Pola: ");
            for (int i = 0; i < 6; i++) Serial.print(pattern[i]);
            Serial.println(")");
        } else {
            MASTER_SERIAL.print("ERR:INV\n");
            Serial.println("  -> ERR:INV");
        }
        return;
    }

    if (cmd.startsWith("TM:")) {
        MASTER_SERIAL.print("ACK\n");
        Serial.print("  -> ACK (Timing: ");
        Serial.print(cmd);
        Serial.println(")");
        return;
    }

    // Perintah tidak dikenal
    MASTER_SERIAL.print("ERR:INV\n");
    Serial.print("  -> ERR:INV (Unknown: '");
    Serial.print(cmd);
    Serial.println("')");
}

void allSolenoidsOff() {
    for (byte i = 0; i < 6; i++) {
        digitalWrite(solenoidPins[i], LOW);
    }
    Serial.println("[Solenoid] Semua OFF");
}

void applyPattern(const byte pattern[6]) {
    for (byte i = 0; i < 6; i++) {
        digitalWrite(solenoidPins[i], pattern[i] ? HIGH : LOW);
    }
}
