# Pinout Master-Slave

## Master - ESP32-S3 (PROJEK_BRAILLE_DISPLAY)
Sebagian besar interaksi IO melalui SPI (Touch, TFT) dan I2C (Tombol MCP23017), serta I2S (Audio). Pin solenoid lokal dipertahankan, dan UART (Serial2) ditambahkan untuk modul komunikasi Slave.

| Pin GPIO ESP32-S3 | Keterangan / Fungsi Utama                   | Status         |
|-------------------|---------------------------------------------|----------------|
| 1                 | Solenoid Lokal (Dot 1)                      | Aktif / Backup |
| 2                 | TFT_DC                                      | Aktif          |
| 3                 | Solenoid Lokal (Dot 2)                      | Aktif / Backup |
| 4                 | TFT_RST                                     | Aktif          |
| 5                 | I2S_BCLK (Audio)                            | Aktif          |
| 6                 | I2S_LRC (Audio)                             | Aktif          |
| 7                 | I2S_DOUT (Audio)                            | Aktif          |
| 8                 | I2C SDA (ke MCP23017 untuk Tombol)          | Aktif          |
| 9                 | I2C SCL (ke MCP23017 untuk Tombol)          | Aktif          |
| 10                | TFT_BL (Backlight Layar)                    | Aktif          |
| 11                | TFT_MOSI                                    | Aktif          |
| 12                | TOUCH_IRQ (XPT2046 Interupsi)               | Aktif          |
| 13                | TFT_MISO                                    | Aktif          |
| 14                | TOUCH_CS                                    | Aktif          |
| 15                | TFT_CS                                      | Aktif          |
| 16                | Solenoid Lokal (Dot 3)                      | Aktif / Backup |
| 17                | Solenoid Lokal (Dot 4)                      | Aktif / Backup |
| 18                | TFT_SCLK                                    | Aktif          |
| **19**            | **Serial2 TX (Kirim ke Slave RX/D19)**      | **BARU (Master)** |
| **20**            | **Serial2 RX (Terima dari Slave TX/D18)**   | **BARU (Master)** |
| 21                | Solenoid Lokal (Dot 5)                      | Aktif / Backup |
| 22                | Solenoid Lokal (Dot 6)                      | Aktif / Backup |

> **PERHATIAN (GPIO 19, 20):** Beberapa dev-board ESP32-S3 menggunakan pin 19 dan 20 untuk USB internal (D-/D+). Pastikan dev board yang digunakan menyediakan akses pin tersebut bebas dari USB. Jika pin 19 dan 20 terpakai, ubah `#define SLAVE_TX_PIN` dan `#define SLAVE_RX_PIN` di `I_SlaveComm.ino` ke GPIO yang free seperti GPIO 38/39 atau 43/44.

## Slave - Arduino Mega 2560 (SLAVE_BRAILLE_MEGA)
Arduino Mega kini dipangkas seluruhnya fungsinya dari logika UI dan input tombol. Seluruh pin tombol, enter, dan OLED telah **dilepas**. Mega hanya mengontrol solenoid dan mendengarkan instruksi Master.

| Pin Mega 2560 | Keterangan / Fungsi Utama                 | Status        |
|---------------|-------------------------------------------|---------------|
| D2            | Output ke Driver Solenoid Dot 1           | Aktif         |
| D3            | Output ke Driver Solenoid Dot 2           | Aktif         |
| D4            | Output ke Driver Solenoid Dot 3           | Aktif         |
| D5            | Output ke Driver Solenoid Dot 4           | Aktif         |
| D6            | Output ke Driver Solenoid Dot 5           | Aktif         |
| D7            | Output ke Driver Solenoid Dot 6           | Aktif         |
| D8 ~ D13      | Tombol 1-6 (Tidak lagi dipakai)           | ❌ **Dihapus** |
| A0            | Tombol Enter (Tidak lagi dipakai)         | ❌ **Dihapus** |
| D20 (SDA)     | I2C OLED (Tidak lagi dipakai)             | ❌ **Dihapus** |
| D21 (SCL)     | I2C OLED (Tidak lagi dipakai)             | ❌ **Dihapus** |
| **D18 (TX1)** | **UART1 TX (Kirim PONG/ACK ke Master RX)**| ✅ **Ditambahkan** |
| **D19 (RX1)** | **UART1 RX (Terima instruksi Master TX)** | ✅ **Ditambahkan** |
