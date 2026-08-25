// ============================================================
// I_SlaveComm.ino
// KOMUNIKASI MASTER (ESP32-S3) → SLAVE (Arduino Mega 2560)
//
// Protokol: UART (Hardware Serial)
// Baud: 9600
// Format command: "BS:PPPPPP\n"  (P = 0 atau 1, dot1~dot6)
//                 "PING\n"
//                 "OFF\n"
//                 "TM:AAAA:GGGG\n"
//
// Response Slave: "ACK\n" | "PONG\n" | "ERR:INV\n" | "RDY\n"
//
// PIN ESP32-S3:
//   [PERLU VERIFIKASI] Gunakan GPIO yang bebas di board Anda.
//   Default di sini: TX=GPIO19, RX=GPIO20
//   Ganti SLAVE_TX_PIN / SLAVE_RX_PIN jika konflik.
//
// CATATAN PENTING:
//   Fungsi setSolenoids() yang asli di H_Solenoid.ino TIDAK DIUBAH.
//   Modul ini menambahkan pengiriman ke Slave di SAMPING kontrol lokal.
//   Jika Slave tidak terhubung, Master tetap berjalan normal.
// ============================================================

// ============================================================
// KONFIGURASI PIN UART SLAVE
// [PERLU VERIFIKASI] Sesuaikan dengan board ESP32-S3 Anda
// Pastikan pin ini bebas dari fungsi lain:
//   GPIO 1,3,16,17,21,22 = Solenoid lokal → JANGAN PAKAI
//   GPIO 2,4,18 = TFT → JANGAN PAKAI
//   GPIO 5,6,7 = I2S → JANGAN PAKAI
//   GPIO 8,9 = I2C → JANGAN PAKAI
//   GPIO 10,11,13,14,15 = SPI/TFT → JANGAN PAKAI
//   GPIO 12 = TOUCH_IRQ → JANGAN PAKAI
// ============================================================
#define SLAVE_TX_PIN  41   // ESP32-S3 TX → Slave RX (D19 Mega)
#define SLAVE_RX_PIN  42   // ESP32-S3 RX ← Slave TX (D18 Mega)
#define SLAVE_BAUD    9600

// ============================================================
// TIMEOUT & WATCHDOG
// ============================================================
#define SLAVE_ACK_TIMEOUT_MS  300   // Tunggu ACK maksimal 300ms (non-blocking)
#define SLAVE_PING_INTERVAL   5000  // Ping ke Slave setiap 5 detik

// ============================================================
// STATE SLAVE
// ============================================================
bool   slaveConnected   = false;
unsigned long lastSlavePing = 0;
unsigned long lastSlaveAckTime = 0;

// Buffer parsing response dari Slave
String slaveRxBuffer = "";

// ============================================================
// INIT KOMUNIKASI SLAVE
// Dipanggil dari setup() utama
// ============================================================
void initSlaveComm() {
    Serial.println("[SlaveComm] Inisialisasi UART ke Slave...");
    Serial.print("[SlaveComm] TX=GPIO"); Serial.print(SLAVE_TX_PIN);
    Serial.print(" RX=GPIO"); Serial.print(SLAVE_RX_PIN);
    Serial.print(" Baud="); Serial.println(SLAVE_BAUD);

    // Serial2 = hardware UART kedua ESP32-S3
    // begin(baud, config, rxPin, txPin)
    Serial2.begin(SLAVE_BAUD, SERIAL_8N1, SLAVE_RX_PIN, SLAVE_TX_PIN);
    delay(200);

    // Kirim PING untuk cek apakah Slave sudah siap
    bool pong = slavePing();
    if (pong) {
        Serial.println("[SlaveComm] Slave TERHUBUNG dan siap.");
        slaveConnected = true;
    } else {
        Serial.println("[SlaveComm] Slave tidak merespon. Master tetap berjalan tanpa Slave.");
        slaveConnected = false;
    }
    lastSlavePing = millis();
}

// ============================================================
// PING → PONG
// Return true jika Slave merespon dalam SLAVE_ACK_TIMEOUT_MS
// ============================================================
bool slavePing() {
    // Bersihkan buffer
    while (Serial2.available()) Serial2.read();

    Serial2.print("PING\n");

    unsigned long t = millis();
    String resp = "";
    while (millis() - t < SLAVE_ACK_TIMEOUT_MS) {
        while (Serial2.available()) {
            char c = (char)Serial2.read();
            if (c == '\n') {
                resp.trim();
                if (resp == "PONG") {
                    Serial.println("[SlaveComm] PONG diterima.");
                    return true;
                }
                resp = "";
            } else {
                resp += c;
            }
        }
    }
    Serial.println("[SlaveComm] PING timeout - Slave tidak merespon.");
    return false;
}

// ============================================================
// KIRIM PATTERN KE SLAVE
// pattern: uint8_t 6-bit (bit0=dot1 ... bit5=dot6)
// Non-blocking: tidak menunggu ACK jika Slave tidak terhubung
// ============================================================
void slaveSendPattern(uint8_t pattern) {
    if (!Serial2) return;  // Serial2 belum diinisialisasi

    // Bangun string command "BS:PPPPPP\n"
    char cmd[12];
    cmd[0] = 'B'; cmd[1] = 'S'; cmd[2] = ':';
    for (int i = 0; i < 6; i++) {
        cmd[3 + i] = ((pattern >> i) & 1) ? '1' : '0';
    }
    cmd[9] = '\n';
    cmd[10] = '\0';

    Serial2.print(cmd);

    Serial.print("[SlaveComm] → ");
    Serial.print(cmd);
}

// ============================================================
// KIRIM ALL-OFF KE SLAVE
// ============================================================
void slaveSendOff() {
    if (!Serial2) return;
    Serial2.print("OFF\n");
    Serial.println("[SlaveComm] → OFF");
}

// ============================================================
// KIRIM TIMING KE SLAVE
// activeDurationMs: lama solenoid ON per huruf
// gapDurationMs: jeda antar huruf
// ============================================================
void slaveSendTiming(uint32_t activeDurationMs, uint32_t gapDurationMs) {
    if (!Serial2) return;

    // Batasi range: 50ms ~ 9999ms
    activeDurationMs = constrain(activeDurationMs, 50, 9999);
    gapDurationMs    = constrain(gapDurationMs,    50, 9999);

    char cmd[20];
    snprintf(cmd, sizeof(cmd), "TM:%04lu:%04lu\n", activeDurationMs, gapDurationMs);
    Serial2.print(cmd);

    Serial.print("[SlaveComm] → ");
    Serial.print(cmd);
}

// ============================================================
// PROSES INCOMING DATA DARI SLAVE (non-blocking)
// Dipanggil dari loop() utama
// ============================================================
void updateSlaveComm() {
    // Baca response dari Slave
    while (Serial2.available()) {
        char c = (char)Serial2.read();
        if (c == '\n') {
            slaveRxBuffer.trim();
            if (slaveRxBuffer.length() > 0) {
                processSlaveResponse(slaveRxBuffer);
            }
            slaveRxBuffer = "";
        } else if (c != '\r') {
            slaveRxBuffer += c;
            // Batasi buffer agar tidak overflow
            if (slaveRxBuffer.length() > 32) {
                slaveRxBuffer = "";
            }
        }
    }

    // Periodic ping untuk memonitor koneksi Slave
    unsigned long now = millis();
    if (now - lastSlavePing >= SLAVE_PING_INTERVAL) {
        lastSlavePing = now;
        bool ok = slavePing();
        if (ok != slaveConnected) {
            slaveConnected = ok;
            if (slaveConnected) {
                Serial.println("[SlaveComm] Slave kembali terhubung.");
            } else {
                Serial.println("[SlaveComm] Slave terputus! Solenoid lokal masih aktif.");
            }
        }
    }
}

// ============================================================
// PROSES RESPONSE DARI SLAVE
// ============================================================
void processSlaveResponse(const String &resp) {
    if (resp == "ACK") {
        // Normal acknowledgment
        // Serial.println("[SlaveComm] ← ACK");  // Uncomment untuk debug verbose
        lastSlaveAckTime = millis();
        if (!slaveConnected) {
            slaveConnected = true;
            Serial.println("[SlaveComm] Slave terhubung (via ACK).");
        }
    } else if (resp == "PONG") {
        // Response ping
        lastSlaveAckTime = millis();
    } else if (resp == "RDY") {
        Serial.println("[SlaveComm] ← Slave RDY (baru boot).");
        slaveConnected = true;
        lastSlaveAckTime = millis();
        // Sinkronisasi timing berdasarkan ttsSpeedMode saat ini
        syncSlaveTimingToSpeed();
    } else if (resp.startsWith("ERR:")) {
        Serial.print("[SlaveComm] ← ERROR dari Slave: ");
        Serial.println(resp);
    } else {
        Serial.print("[SlaveComm] ← Response tidak dikenal: ");
        Serial.println(resp);
    }
}

// ============================================================
// SINKRONISASI TIMING SLAVE SESUAI ttsSpeedMode
// Dipanggil saat ttsSpeedMode berubah atau Slave baru konek
// ============================================================
void syncSlaveTimingToSpeed() {
    uint32_t activeDur, gapDur;
    if (ttsSpeedMode == 0) {        // Normal
        activeDur = 400;
        gapDur    = 150;
    } else if (ttsSpeedMode == 1) { // Lambat
        activeDur = 750;
        gapDur    = 250;
    } else {                        // Sangat Lambat (default)
        activeDur = 1400;
        gapDur    = 450;
    }
    slaveSendTiming(activeDur, gapDur);
    Serial.print("[SlaveComm] Timing dikirim ke Slave: ON=");
    Serial.print(activeDur); Serial.print("ms GAP=");
    Serial.print(gapDur); Serial.println("ms");
}

// ============================================================
// STATUS SLAVE (untuk serial menu '@')
// ============================================================
void printSlaveStatus() {
    Serial.print("Slave    : ");
    Serial.println(slaveConnected ? "Terhubung" : "Tidak terhubung");
    if (slaveConnected) {
        unsigned long ago = millis() - lastSlaveAckTime;
        Serial.print("ACK terakhir : ");
        Serial.print(ago); Serial.println("ms lalu");
    }
}
