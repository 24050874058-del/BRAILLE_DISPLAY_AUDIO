# PROJEK_BRAILLER_DISPLAY

Proyek ini adalah sistem "Braille Trainer" yang telah dimigrasikan dari standalone Arduino Mega ke arsitektur **Master-Slave**, menggunakan ESP32-S3 sebagai **Master** (kontrol antarmuka dan suara TTS) serta Arduino Mega sebagai **Slave** (kontroler aktuator solenoid).

## Struktur Repositori

```text
PROJEK_BRAILLER_DISPLAY/
│
├── MASTER/
│   └── PROJEK_BRAILLE_DISPLAY/     # Program ESP32-S3 (Diubah dari project asli)
│       ├── PROJEK_BRAILLE_DISPLAY.ino
│       ├── A_EEPROM.ino
│       ├── B_Audio.ino
│       ├── C_Braille_Logic.ino
│       ├── D_Display.ino
│       ├── E_Touch.ino
│       ├── F_WiFi.ino
│       ├── G_Serial.ino
│       ├── H_Solenoid.ino          # Modifikasi sinkronisasi lokal & slave
│       └── I_SlaveComm.ino         # BARU: Modul komunikasi UART Master ke Slave
│
├── SLAVE/
│   └── SLAVE_BRAILLE_MEGA/         # Program Arduino Mega 2560
│       └── SLAVE_BRAILLE_MEGA.ino  # BARU: Firmware penerima UART & Driver Solenoid
│
├── docs/
│   ├── PROTOKOL_KOMUNIKASI.md      # Detail serial UART "BS:PPPPPP" dsb.
│   ├── PINOUT.md                   # Perubahan & daftar wiring yang digunakan
│   └── ARSITEKTUR.md               # Penjelasan rancangan Master-Slave
│
└── README.md                       # Anda berada di sini
```

## Panduan Penggunaan
1. Upload kode di folder `MASTER/PROJEK_BRAILLE_DISPLAY` ke board **ESP32-S3** (Gunakan partisi Huge APP atau di atas 3MB karena terdapat framework WiFi + TTS I2S).
2. Upload kode di folder `SLAVE/SLAVE_BRAILLE_MEGA` ke board **Arduino Mega 2560**.
3. Rangkai kabel komunikasi:
   - Sambungkan Master TX (ESP32) ke Mega RX1 (D19).
   - Sambungkan Master RX (ESP32) ke Mega TX1 (D18).
   - Sambungkan Ground (GND) ESP32 ke GND Arduino Mega.
4. Perhatikan bahwa di mode serial menu ESP32 (ketik `@`), kini akan tampil status "*Slave: Terhubung*" (jika komunikasi Mega-ESP berhasil berjalan).
5. Segala interaksi UI, Input Keypad (via MCP23017), layar sentuh (TFT) berjalan **hanya pada ESP32 Master**. Arduino Mega hanya akan mengeklik solenoidnya berdasarkan data dari Master.

Untuk dokumentasi lebih lengkap silakan masuk ke direktori `docs/`.
