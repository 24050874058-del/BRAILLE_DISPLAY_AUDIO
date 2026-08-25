# Protokol Komunikasi Master-Slave

## Tinjauan
Komunikasi antara Master (ESP32-S3) dan Slave (Arduino Mega 2560) dilakukan melalui interface **UART (Hardware Serial)**.
- **Baud Rate**: 9600 bps
- **Format Data**: 8 Data bits, No Parity, 1 Stop Bit (8N1)
- **Line Ending**: `\n` (Newline / Line Feed)

## Koneksi Fisik
- **Master TX** (ESP32-S3) ➔ **Slave RX** (Arduino Mega D19 / RX1)
- **Master RX** (ESP32-S3) ➔ **Slave TX** (Arduino Mega D18 / TX1)
- **GND** ➔ **GND** (Wajib terhubung)

> **Catatan**: Pin TX/RX pada ESP32-S3 dapat dikonfigurasi melalui `#define SLAVE_TX_PIN` dan `#define SLAVE_RX_PIN` di `I_SlaveComm.ino`. Standar default yang diberikan adalah GPIO 19 dan 20. Pastikan ini tidak berbenturan dengan hardware Anda.

## Command Set (Master ke Slave)

### 1. PING
Digunakan untuk mengecek apakah Slave terhubung dan responsif.
- **Kirim**: `PING\n`
- **Respons Normal**: `PONG\n`

### 2. SET PATTERN
Mengirim pola Braille 6-titik untuk ditampilkan di solenoid Slave.
- **Kirim**: `BS:PPPPPP\n` (di mana P adalah '1' atau '0')
  - `P` ke-1 = Solenoid 1
  - ...
  - `P` ke-6 = Solenoid 6
- **Contoh**: `BS:100000\n` (Mengaktifkan solenoid 1 saja, titik 'A')
- **Respons Normal**: `ACK\n`
- **Respons Error**: `ERR:INV\n` (Jika panjang tidak pas atau karakter selain 0/1)

### 3. ALL OFF
Mematikan seluruh solenoid secara paksa.
- **Kirim**: `OFF\n`
- **Respons Normal**: `ACK\n`

### 4. SET TIMING
Memberi tahu slave mengenai durasi aktuasi solenoid dan durasi gap. (Saat ini Master mengatur timing sendiri dengan perintah BS dan OFF, namun ini disediakan untuk backward/forward compatibility jika Slave kelak mengatur animasinya).
- **Kirim**: `TM:AAAA:GGGG\n` (AAAA = durasi aktif dalam milidetik, GGGG = durasi gap)
- **Contoh**: `TM:1400:0450\n`
- **Respons Normal**: `ACK\n`

## Response Set (Slave ke Master)

- `RDY\n`
  Dikirim sekali saat Slave (Mega) selesai boot/setup. Membantu Master mengetahui jika Slave ter-reset.
- `ACK\n`
  Pesan keberhasilan atas suatu eksekusi command (`BS:`, `OFF`, `TM:`).
- `PONG\n`
  Respons khusus untuk `PING`.
- `ERR:INV\n`
  Menunjukkan command tidak dikenali atau formatnya salah.

## Manajemen Kesalahan & Keselamatan
- **Timeouts**: Jika Slave tidak menerima command apa pun (atau `PING`) dari Master selama lebih dari 5000 ms, watchdog slave akan mematikan seluruh solenoid. Ini melindungi koil solenoid agar tidak terbakar jika komunikasi terputus atau Master hang/reset saat solenoid sedang hidup.
- **Asynchronous**: Master bersifat non-blocking saat mengirim command dan menerima response, sehingga jika Slave terputus atau rusak, antarmuka Master (TFT, suara) tetap berjalan lancar.
