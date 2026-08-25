// ============================================================
// SERIAL MENU
// ============================================================

void inputWiFiSerial() {
    Serial.println("\n========== INPUT WIFI (SERIAL) ==========");
    Serial.println("Sama seperti di layar sentuh (UI).");
    Serial.println("Ketik SCAN untuk mencari WiFi terlebih dahulu,");
    Serial.println("atau langsung ketik SSID lalu tekan Enter:");
    
    while (Serial.available()) { Serial.read(); delay(2); }
    while (!Serial.available()) delay(10);
    String input = Serial.readStringUntil('\n'); input.replace("\r",""); input.trim();
    
    // ---- Fitur SCAN (sama seperti tombol "CARI WIFI DI SEKITAR" di UI) ----
    if (input.equalsIgnoreCase("SCAN")) {
        Serial.println("\n[WiFi] Memindai jaringan sekitar...");
        // WiFi.disconnect(); delay(100); // FIXED BUG HERE
        int n = WiFi.scanNetworks();
        if (n <= 0) {
            Serial.println("[WiFi] Tidak ada WiFi ditemukan.");
            Serial.println("=========================================\n");
            return;
        }
        Serial.println("\n----- DAFTAR WIFI DITEMUKAN -----");
        for (int i = 0; i < n; i++) {
            int encType = WiFi.encryptionType(i);
            String encStr;
            if (encType == WIFI_AUTH_OPEN) encStr = "Terbuka";
            else if (encType == WIFI_AUTH_WPA2_ENTERPRISE) encStr = "WPA2 Enterprise";
            else encStr = "WPA/WPA2";
            Serial.print("  "); Serial.print(i + 1); Serial.print(") ");
            Serial.print(WiFi.SSID(i));
            Serial.print("  ["); Serial.print(encStr); Serial.print("]");
            Serial.print("  RSSI: "); Serial.print(WiFi.RSSI(i)); Serial.println(" dBm");
        }
        Serial.println("---------------------------------");
        Serial.println("Ketik nomor (1-" + String(n) + ") atau ketik SSID manual:");
        
        while (Serial.available()) { Serial.read(); delay(2); }
        while (!Serial.available()) delay(10);
        input = Serial.readStringUntil('\n'); input.replace("\r",""); input.trim();
        
        // Cek apakah input adalah nomor
        int sel = input.toInt();
        if (sel >= 1 && sel <= n) {
            WIFI_SSID = WiFi.SSID(sel - 1);
            int selEnc = WiFi.encryptionType(sel - 1);
            if (selEnc == WIFI_AUTH_WPA2_ENTERPRISE) {
                WIFI_IS_ENT = true;
            } else if (selEnc == WIFI_AUTH_OPEN) {
                WIFI_IS_ENT = false;
                WIFI_PASSWORD = "";
                WIFI_USER = "";
                Serial.print("[WiFi] Jaringan terbuka: "); Serial.println(WIFI_SSID);
                Serial.println("[WiFi] Menghubungkan...");
                if (connectWiFi(20000)) {
                    eepromSaveWiFi(WIFI_SSID, WIFI_PASSWORD, WIFI_USER, WIFI_IS_ENT);
                    if (currentScreen == SCR_MAIN) refreshMainWiFiStatus();
                    else drawWiFiScreen();
                } else {
                    Serial.println("[WiFi] Koneksi gagal.");
                }
                Serial.println("=========================================\n");
                return;
            } else {
                WIFI_IS_ENT = false;
            }
        } else {
            WIFI_SSID = input;
            WIFI_IS_ENT = false; 
        }
    } else {
        WIFI_SSID = input;
        WIFI_IS_ENT = false;
    }
    Serial.print("SSID: ["); Serial.print(WIFI_SSID); Serial.println("]");
    
    // ---- Tanya tipe jaringan ----
    if (!WIFI_IS_ENT) {
        while (Serial.available()) { Serial.read(); delay(2); }
        Serial.println("\nTipe jaringan:");
        Serial.println("  1) WPA/WPA2 Personal (Password saja)");
        Serial.println("  2) WPA2 Enterprise / Kampus (Username + Password)");
        Serial.println("  3) Terbuka (Tanpa password)");
        Serial.println("Pilih (1/2/3) [default=1]:");
        while (!Serial.available()) delay(10);
        String tipe = Serial.readStringUntil('\n'); tipe.replace("\r",""); tipe.trim();
        
        if (tipe == "2") {
            WIFI_IS_ENT = true;
        } else if (tipe == "3") {
            WIFI_IS_ENT = false;
            WIFI_PASSWORD = "";
            WIFI_USER = "";
            Serial.println("[WiFi] Jaringan terbuka, tanpa password.");
            Serial.println("[WiFi] Menghubungkan...");
            if (connectWiFi(20000)) {
                eepromSaveWiFi(WIFI_SSID, WIFI_PASSWORD, WIFI_USER, WIFI_IS_ENT);
                if (currentScreen == SCR_MAIN) refreshMainWiFiStatus();
                else drawWiFiScreen();
            } else {
                Serial.println("[WiFi] Koneksi gagal. Kredensial tidak disimpan.");
            }
            Serial.println("=========================================\n");
            return;
        }
    }
    
    // ---- Input Username ----
    if (WIFI_IS_ENT) {
        Serial.println("\n[Enterprise] Jaringan ini butuh Username (sama seperti di UI).");
        while (Serial.available()) { Serial.read(); delay(2); }
        Serial.println("Ketik Username lalu tekan Enter:");
        while (!Serial.available()) delay(10);
        WIFI_USER = Serial.readStringUntil('\n'); WIFI_USER.replace("\r",""); WIFI_USER.trim();
        Serial.print("Username: ["); Serial.print(WIFI_USER); Serial.println("]");
    } else {
        WIFI_USER = "";
    }
    
    // ---- Input Password ----
    while (Serial.available()) { Serial.read(); delay(2); }
    Serial.println("Ketik Password lalu tekan Enter (kosong=tanpa password):");
    while (!Serial.available()) delay(10);
    WIFI_PASSWORD = Serial.readStringUntil('\n'); WIFI_PASSWORD.replace("\r",""); WIFI_PASSWORD.trim();
    Serial.print("Password: ["); Serial.print(WIFI_PASSWORD.length()); Serial.println(" karakter]");
    
    // ---- Konfirmasi ----
    Serial.println("\n--- Ringkasan ---");
    Serial.print("  SSID     : "); Serial.println(WIFI_SSID);
    Serial.print("  Tipe     : "); Serial.println(WIFI_IS_ENT ? "WPA2 Enterprise" : "WPA Personal");
    if (WIFI_IS_ENT) { Serial.print("  Username : "); Serial.println(WIFI_USER); }
    Serial.print("  Password : "); Serial.print(WIFI_PASSWORD.length()); Serial.println(" karakter");
    Serial.println("-----------------");
    Serial.println("Ketik Y untuk SAMBUNGKAN, atau N untuk BATAL:");
    
    while (Serial.available()) { Serial.read(); delay(2); }
    while (!Serial.available()) delay(10);
    String konfirm = Serial.readStringUntil('\n'); konfirm.replace("\r",""); konfirm.trim();
    konfirm.toUpperCase();
    
    if (konfirm != "Y") {
        Serial.println("[WiFi] Dibatalkan oleh pengguna.");
        Serial.println("=========================================\n");
        return;
    }
    
    // ---- Sambungkan ----
    Serial.println("[WiFi] Menghubungkan...");
    if (connectWiFi(20000)) {
        eepromSaveWiFi(WIFI_SSID, WIFI_PASSWORD, WIFI_USER, WIFI_IS_ENT);
        Serial.println("[WiFi] Berhasil! Kredensial tersimpan ke EEPROM.");
        if (currentScreen == SCR_MAIN) refreshMainWiFiStatus();
        else drawWiFiScreen();
    } else {
        Serial.println("[WiFi] Koneksi gagal. Kredensial tidak disimpan.");
    }
    Serial.println("=========================================\n");
}

void serialMenu() {
    if (!Serial.available()) return;
    char cmd=Serial.read();
    if (cmd=='\n'||cmd=='\r') return;
    
    switch(cmd) {
        // ---- Braille dots ----
        case '1': case '2': case '3':
        case '4': case '5': case '6':
            handleButtonPress(cmd-'1');
            Serial.print("[Serial] Titik "); Serial.print(cmd); Serial.println(" toggle");
            break;
        // ---- Aksi ----
        case 'e': case 'E':
            Serial.println("[Serial] ENTER");
            handleButtonPress(6); break;
        case 's': case 'S':
            Serial.println("[Serial] SPASI");
            handleButtonPress(7); break;
        case 'x': case 'X':
            Serial.println("[Serial] HAPUS");
            handleButtonPress(8); break;
        case 'm': case 'M':
            Serial.println("[Serial] GANTI MODE");
            handleButtonPress(9); break;
        // ---- Calc operators (debug) ----
        case '+': Serial.println("[Serial] OP: +"); handleButtonPress(10); break;
        case '-': Serial.println("[Serial] OP: -"); handleButtonPress(11); break;
        case '*': Serial.println("[Serial] OP: *"); handleButtonPress(12); break;
        case '/': Serial.println("[Serial] OP: /"); handleButtonPress(13); break;
        // ---- Audio ----
        case '!':
            Serial.println("[Serial] Test Audio...");
            if (WiFi.status()==WL_CONNECTED) {
                audio.setVolume(21);
                speak("Braille siap digunakan");
            } else Serial.println("[Serial] WiFi tidak terhubung, audio dilewati.");
            break;
        // ---- Status ----
        case '@': {
            Serial.println("\n======== STATUS PERANGKAT ========");
            if (WiFi.status()==WL_CONNECTED) {
                Serial.println("WiFi     : Terhubung");
                Serial.print("SSID     : "); Serial.println(WIFI_SSID);
                Serial.print("IP       : "); Serial.println(WiFi.localIP());
                Serial.print("RSSI     : "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
            } else {
                Serial.println("WiFi     : Tidak terhubung");
            }
            String sv,sp,su; bool se;
            Serial.print("EEPROM   : ");
            Serial.println(eepromLoadWiFi(sv,sp,su,se)?sv:"(kosong)");
            Serial.print("Mode     : ");
            const char* mn[]={"Huruf","Angka","Kata","Kalkulator"};
            Serial.println(mn[currentMode]);
            Serial.print("Pola     : 0b");
            for (int i=5;i>=0;i--) Serial.print((currentPattern>>i)&1);
            Serial.println();
            Serial.print("Layar    : ");
            Serial.println(currentScreen==SCR_MAIN?"Main":"WiFi");
            printSlaveStatus();  // Status koneksi Slave Arduino Mega
            Serial.println("==================================");
            break;
        }
        // ---- WiFi input ----
        case 'w': case 'W': inputWiFiSerial(); break;
        // ---- CLR ----
        case 'c': case 'C': {
            delay(50);
            String rest="";
            while (Serial.available()) {
                char ch=Serial.read();
                if (ch=='\n'||ch=='\r') break;
                rest+=ch; delay(2);
            }
            rest.toUpperCase();
            if (rest=="LR") {
                eepromClearWiFi();
                WIFI_SSID=WIFI_PASSWORD=WIFI_USER=""; WIFI_IS_ENT=false;
                WiFi.disconnect(true);
                if (currentScreen==SCR_MAIN) refreshMainWiFiStatus();
                else drawWiFiScreen();
            } else {
                Serial.println("[Serial] Perintah tidak dikenal. Maksud CLR?");
            }
            break;
        }
    }
}
