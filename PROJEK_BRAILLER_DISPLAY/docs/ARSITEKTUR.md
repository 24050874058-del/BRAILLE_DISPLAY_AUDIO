# Arsitektur Master-Slave Braille Display

Proyek ini telah dikonversi ke arsitektur Master-Slave agar sistem lebih termodulasi, mengurangi redundansi kontrol dari Slave, dan memastikan sistem bekerja stabil.

## Filosofi Desain
1. **Master (ESP32-S3) sebagai Otak Tunggal (Single Source of Truth):**
   - Menangani seluruh UI/UX, pembacaan touch screen, TFT, audio TTS, WiFi, hingga konektivitas logika input dari tombol Braille (via I2C GPIO expander).
   - Mengendalikan pola Braille kapan harus dibentuk, timing durasi on-off (sesuai kecepatan suara pengguna), serta validasi karakter.

2. **Slave (Arduino Mega) sebagai Aktuator Sederhana:**
   - Tugasnya sangat minimalis: mendengarkan perintah (lewat hardware UART/Serial1), lalu mengaktifkan atau mematikan relai/solenoid.
   - Tidak ada lagi logika memproses input tombol, tidak ada debounce, tidak ada tampilan OLED, tidak ada menu yang memakan resource dan tumpang tindih dengan fungsi Master.

## Diagram Arsitektur

```text
       [Pengguna]
           │
           ▼ (Sentuh Layar, Tekan Tombol, Dengarkan Audio)
┌────────────────────────────────────────────────────────┐
│ MASTER: ESP32-S3 (PROJEK_BRAILLE_DISPLAY)              │
│                                                        │
│  - Input Handling (MCP23017, XPT2046)                  │
│  - Display (ST7796S)                                   │
│  - Audio (I2S MAX98357A)                               │
│  - Logic Controller (Braille ke Teks, Kalkulator)      │
│  - Komunikasi Serial2 ke Slave (I_SlaveComm.ino)       │
│                                                        │
│  *Master mengirim pola "BS:100000" dan timingnya.      │
└──────────────────────────┬─────────────────────────────┘
                           │ UART (TX-RX, 9600 bps)
                           │
                           ▼
┌────────────────────────────────────────────────────────┐
│ SLAVE: Arduino Mega 2560 (SLAVE_BRAILLE_MEGA)          │
│                                                        │
│  - Hardware Serial1 untuk menerima instruksi           │
│  - Parser Perintah (Mendengarkan pola Braille)         │
│  - Watchdog Solenoid (5 detik timeout)                 │
│  - 6 Pin Output menggerakkan Solenoid 1 s/d 6          │
└──────────────────────────┬─────────────────────────────┘
                           │
           ┌──────┬────────┼────────┬──────┐
           ▼      ▼        ▼        ▼      ▼
        Sol 1   Sol 2    Sol 3    Sol 4  Sol 5 ...
```

## Mekanisme Sinkronisasi
1. Ketika Master mengeksekusi fungsi `setSolenoids(pattern)` atau `setSolenoidsOff()`, Master tidak hanya menyalakan/mematikan pin solenoid lokal di ESP32, tetapi juga akan mencetak pesan UART ke Slave secara bersamaan.
2. Respons Slave sangat cepat karena hanya membaca `Serial1.read()` dan parsing string pendek, sehingga delay hampir tidak terasa.
3. Slave memiliki **Watchdog Timer 5 detik**. Jika kabel UART tercabut atau Master reset secara tiba-tiba sementara solenoid dalam kondisi ON, Slave akan otomatis mendeteksinya sebagai kondisi tidak aman (tidak ada ping/perintah) dan mematikan seluruh solenoid secara otomatis untuk menghindari overheat koil.
