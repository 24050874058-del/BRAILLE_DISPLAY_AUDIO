// ============================================================
// UI & DISPLAY HELPERS
// ============================================================

bool inRect(int16_t tx,int16_t ty,int16_t rx,int16_t ry,int16_t rw,int16_t rh) {
    return tx>=rx && tx<rx+rw && ty>=ry && ty<ry+rh;
}

void drawBtn(int16_t x,int16_t y,int16_t w,int16_t h, uint16_t bg,uint16_t fg,const char* lbl,uint8_t sz=1,uint8_t r=6) {
    tft.fillRoundRect(x,y,w,h,r,bg);
    int16_t x1,y1; uint16_t tw,th;
    tft.setFont(); tft.setTextSize(sz); tft.setTextColor(fg,bg);
    tft.getTextBounds(lbl,0,0,&x1,&y1,&tw,&th);
    tft.setCursor(x+(w-(int16_t)tw)/2, y+(h-(int16_t)th)/2);
    tft.print(lbl);
}

void drawField(int16_t x,int16_t y,int16_t w,int16_t h, const String &text,bool active,const char* ph,bool masked=false) {
    uint16_t bc = active ? C_ACCENT : C_BORDER;
    tft.fillRoundRect(x,y,w,h,5,C_SURFACE);
    tft.drawRoundRect(x,y,w,h,5,bc);
    if (active) tft.drawRoundRect(x+1,y+1,w-2,h-2,4,bc);
    tft.setFont(); tft.setTextSize(1);
    
    if (text.length()==0) {
        tft.setTextColor(C_DGRAY,C_SURFACE);
        tft.setCursor(x+6, y+(h-8)/2); tft.print(ph);
    } else {
        tft.setTextColor(C_WHITE,C_SURFACE);
        String d="";
        if (masked) for (uint16_t i=0;i<text.length();i++) d+='*';
        else d=text;
        int mc=(w-12)/6;
        if ((int)d.length()>mc) d=d.substring(d.length()-mc);
        tft.setCursor(x+6, y+(h-8)/2); tft.print(d);
        if (active) {
            int cx=x+6+d.length()*6;
            if (cx<x+w-4) tft.fillRect(cx,y+4,2,h-8,C_ACCENT);
        }
    }
}

// ============================================================
// KEYBOARD DRAWING
// ============================================================
void drawKeyboard() {
    tft.fillRect(0,KB_Y-4,480,320-KB_Y+4,C_BG);
    tft.drawFastHLine(0,KB_Y-5,480,C_BORDER);
    // ROW 0
    int r0t=10*KW+9*KGAP, r0x=(480-r0t)/2;
    for (int i=0;i<10;i++) {
        char l[2]={KB_ROW0[kbPage][i],0};
        drawBtn(r0x+i*(KW+KGAP),KB_Y,KW,KH,C_KEY_BG,C_WHITE,l,1,4);
    }
    // ROW 1
    int r1l=strlen(KB_ROW1[kbPage]), r1t=r1l*KW+(r1l-1)*KGAP;
    int r1x=(480-r1t)/2, r1y=KB_Y+KH+KGAP;
    for (int i=0;i<r1l;i++) {
        char l[2]={KB_ROW1[kbPage][i],0};
        drawBtn(r1x+i*(KW+KGAP),r1y,KW,KH,C_KEY_BG,C_WHITE,l,1,4);
    }
    // ROW 2
    int r2l=strlen(KB_ROW2[kbPage]), shW=56, bsW=56;
    int r2t=shW+KGAP+r2l*(KW+KGAP)-KGAP+KGAP+bsW;
    int r2x=(480-r2t)/2, r2y=KB_Y+2*(KH+KGAP);
    drawBtn(r2x,r2y,shW,KH,(kbPage==1)?C_ACCENT:C_KEY_ACT,C_WHITE,(kbPage==1)?"^A":"^a",1,4);
    for (int i=0;i<r2l;i++) {
        char l[2]={KB_ROW2[kbPage][i],0};
        drawBtn(r2x+shW+KGAP+i*(KW+KGAP),r2y,KW,KH,C_KEY_BG,C_WHITE,l,1,4);
    }
    drawBtn(r2x+shW+KGAP+r2l*(KW+KGAP),r2y,bsW,KH,C_KEY_ACT,C_WHITE,"<=",1,4);
    // ROW 3
    int r3y=KB_Y+3*(KH+KGAP), fnW=80, okW=80, r3x=8;
    int spW=480-(r3x*2)-fnW-KGAP-KGAP-okW;
    drawBtn(r3x,              r3y,fnW,KH,C_KEY_ACT,C_WHITE,kbPage<2?"123":"ABC",1,4);
    drawBtn(r3x+fnW+KGAP,     r3y,spW,KH,C_KEY_SP, C_WHITE,"SPASI",1,4);
    drawBtn(r3x+fnW+KGAP+spW+KGAP,r3y,okW,KH,C_SUCCESS,C_WHITE,"OK",1,6);
}

// ============================================================
// MAIN SCREEN
// ============================================================
void drawBrailleDots() {
    int dotR=28, dotGapX=90, dotGapY=64;
    int totalH = 2*dotR + 2*dotGapY; 
    int totalW = 2*dotR + dotGapX;
    int sX = (210 - totalW) / 2 + 10;
    int sY = (320 - 44 - 26 - 34 - totalH) / 2 + 44 + 26 + 34 + dotR;
    
    for (int i=0;i<6;i++) {
        int col=i/3, row=i%3;
        int cx=sX+col*dotGapX, cy=sY+row*dotGapY;
        bool act=(currentPattern>>i)&1;
        
        tft.fillCircle(cx,cy,dotR+2,C_BG);
        tft.fillCircle(cx,cy,dotR+2,act?C_WHITE:C_GRAY);
        tft.fillCircle(cx,cy,dotR,act?C_ACCENT:C_DGRAY);
        
        tft.setFont(&FreeSans12pt7b); tft.setTextSize(1);
        tft.setTextColor(act?C_BG:C_GRAY);
        int16_t x1,y1; uint16_t tw,th;
        tft.getTextBounds(String(i+1),0,0,&x1,&y1,&tw,&th);
        tft.setCursor(cx-tw/2, cy+th/2-2);
        tft.print(i+1);
    }
}

void drawMainScreen() {
    tft.fillScreen(C_BG);
    // ======= TOP BAR =======
    tft.fillRect(0,0,480,50,C_PRIMARY);
    tft.setFont(&FreeSansBold12pt7b); tft.setTextSize(1);
    tft.setTextColor(C_WHITE);
    tft.setCursor(14,34); tft.print("Braille Trainer");
    
    // WiFi icon
    bool wOK=(WiFi.status()==WL_CONNECTED);
    uint16_t wCol=wOK?C_SUCCESS:C_ERROR;
    tft.fillRect(430,0,50,50,C_PRIMARY);
    tft.drawRoundRect(431,2,48,46,5,wOK?C_SUCCESS:C_ERROR);
    int cx=455, cy=34;
    tft.drawCircle(cx,cy,16,wCol);
    tft.drawCircle(cx,cy,10,wCol);
    tft.fillCircle(cx,cy, 4,wCol);
    tft.setTextSize(1); tft.setTextColor(wCol,C_PRIMARY);
    tft.setCursor(440,4); tft.print("WiFi");
    
    // ======= STATUS STRIP =======
    uint16_t stBg=wOK?0x0320:0x3000;
    tft.fillRect(0,50,480,24,stBg);
    tft.setFont(); tft.setTextSize(1);
    tft.setTextColor(C_WHITE,stBg);
    tft.setCursor(8,58);
    if (wOK) {
        tft.print(" Terhubung: "); tft.print(WIFI_SSID);
        tft.print("  |  IP: "); tft.print(WiFi.localIP().toString());
    } else {
        tft.print(" WiFi terputus  -  Sentuh ikon WiFi di kanan atas untuk mengatur");
    }
    
    // ======= MODE BAR =======
    tft.fillRect(0,74,480,36,C_SURFACE);
    tft.drawFastHLine(0,74, 480,C_BORDER);
    tft.drawFastHLine(0,109,480,C_BORDER);
    tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_LGRAY,C_SURFACE);
    tft.setCursor(10,84); tft.print("MODE AKTIF:");
    tft.setFont(&FreeSans12pt7b);
    tft.setTextColor(C_ACCENT); 
    const char* modeNames[]={"HURUF","ANGKA","KATA","KALKULATOR"};
    tft.setCursor(94,100); tft.print(modeNames[currentMode]);
    
    // ======= DIVIDER VERTIKAL =======
    tft.drawFastVLine(215,110,200,C_BORDER);
    
    // ======= BRAILLE DOTS (kiri) =======
    tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_ACCENT,C_BG);
    tft.setCursor(14,114); tft.print("[ TITIK BRAILLE ]");
    drawBrailleDots();
    
    // ======= PANEL KANAN =======
    tft.fillRoundRect(220,110,254,170,8,C_SURFACE);
    tft.drawRoundRect(220,110,254,170,8,C_BORDER);
    tft.setFont(); tft.setTextSize(1);
    tft.setTextColor(C_LGRAY,C_SURFACE);
    
    if (currentMode==2 || currentMode==3) {
        tft.setCursor(232,122); tft.print(currentMode==2 ? "KATA SAAT INI:" : "EKSPRESI MTK:");
        tft.drawFastHLine(224,134,242,C_BORDER);
        
        tft.setFont(&FreeSansBold24pt7b); tft.setTextSize(1); 
        tft.setTextColor(C_WHITE);
        String txt = currentWord.length() ? currentWord : "-";
        int16_t x1,y1; uint16_t tw,th;
        tft.getTextBounds(txt,0,0,&x1,&y1,&tw,&th);
        
        int textY = currentMode==3 ? 180 : 200;
        tft.setCursor(220 + (254-tw)/2, textY); 
        tft.print(txt);
        
        if (currentMode==3) {
            tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_ACCENT, C_SURFACE);
            tft.setCursor(232, 215); tft.print("Tekan Tombol Fisik:");
            tft.setCursor(232, 232); tft.print("[+] Pin 10  [-] Pin 11");
            tft.setCursor(232, 248); tft.print("[*] Pin 12  [/] Pin 13");
            tft.setCursor(232, 264); tft.print("[=] Pin 6   [R] Pin 7(Spasi)");
        } else {
            tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_DGRAY,C_SURFACE);
            tft.setCursor(232,254); tft.print("Pola: 0b");
            for (int i=5;i>=0;i--) tft.print((currentPattern>>i)&1);
        }
    } else {
        tft.setCursor(232,122); tft.print(currentMode==0 ? "HURUF TERAKHIR:" : "ANGKA TERAKHIR:");
        tft.drawFastHLine(224,134,242,C_BORDER);
        
        tft.setFont(&FreeSansBold24pt7b); tft.setTextSize(1);
        tft.setTextColor(C_ACCENT);
        String txt = String(lastChar);
        int16_t x1,y1; uint16_t tw,th;
        tft.getTextBounds(txt,0,0,&x1,&y1,&tw,&th);
        tft.setCursor(220 + (254-tw)/2, 200); 
        tft.print(txt);
        
        // Pola binary kecil
        tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_DGRAY,C_SURFACE);
        tft.setCursor(232,254); tft.print("Pola: 0b");
        for (int i=5;i>=0;i--) tft.print((currentPattern>>i)&1);
    }
    
    // ======= TOMBOL KECEPATAN SUARA =======
    uint16_t spdBg = (ttsSpeedMode==0)?C_SUCCESS:((ttsSpeedMode==1)?C_WARNING:C_ERROR);
    tft.fillRoundRect(220,283,254,24,4,spdBg);
    tft.setFont(); tft.setTextSize(1);
    tft.setTextColor((ttsSpeedMode==1)?C_BG:C_WHITE);
    const char* spdName[] = {"SUARA: NORMAL", "SUARA: LAMBAT", "SUARA: S. LAMBAT"};
    int16_t bx,by; uint16_t bw,bh;
    tft.getTextBounds(spdName[ttsSpeedMode],0,0,&bx,&by,&bw,&bh);
    tft.setCursor(220 + (254-bw)/2, 283 + (24-bh)/2);
    tft.print(spdName[ttsSpeedMode]);
    
    // ======= FOOTER =======
    tft.fillRect(0,310,480,10,C_SURFACE);
    tft.drawFastHLine(0,310,480,C_BORDER);
    tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_DGRAY,C_SURFACE);
    tft.setCursor(8,312); tft.print("ESP32-S3 Braille Trainer  |  Touch ikon WiFi = Pengaturan Jaringan");
}

void refreshMainWiFiStatus() {
    bool wOK=(WiFi.status()==WL_CONNECTED);
    uint16_t wCol=wOK?C_SUCCESS:C_ERROR;
    tft.fillRect(430,0,50,50,C_PRIMARY);
    tft.drawRoundRect(431,2,48,46,5,wOK?C_SUCCESS:C_ERROR);
    tft.drawCircle(455,34,16,wCol);
    tft.drawCircle(455,34,10,wCol);
    tft.fillCircle(455,34, 4,wCol);
    tft.setFont(); tft.setTextSize(1); tft.setTextColor(wCol,C_PRIMARY);
    tft.setCursor(440,4); tft.print("WiFi");
    uint16_t stBg=wOK?0x0320:0x3000;
    tft.fillRect(0,50,480,24,stBg);
    tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_WHITE,stBg);
    tft.setCursor(8,58);
    if (wOK) {
        tft.print(" Terhubung: "); tft.print(WIFI_SSID);
        tft.print("  |  IP: "); tft.print(WiFi.localIP().toString());
    } else {
        tft.print(" WiFi terputus  -  Sentuh ikon WiFi di kanan atas untuk mengatur");
    }
}

// ============================================================
// WIFI SCREEN
// ============================================================
void drawWiFiScreen() {
    bool kbV=(editingField!=FIELD_NONE);
    bool wOK=(WiFi.status()==WL_CONNECTED);
    tft.fillScreen(C_BG);
    
    // Top Bar
    tft.fillRect(0,0,480,44,C_PRIMARY);
    drawBtn(4,6,80,32,C_KEY_ACT,C_WHITE,"< Balik",1,5);
    tft.setFont(); tft.setTextSize(2); tft.setTextColor(C_WHITE,C_PRIMARY);
    tft.setCursor(100,12); tft.print("Pengaturan WiFi");
    
    if (wifiSubState == W_MENU) {
        uint16_t stBg=wOK?0x0320:0x3000;
        tft.fillRoundRect(8,52,464,42,6,stBg);
        tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_WHITE,stBg);
        tft.setCursor(16,60);
        if (wOK) {
            tft.print("Terhubung ke: "); tft.print(WIFI_SSID);
            tft.setCursor(16,74); tft.print("IP: "); tft.print(WiFi.localIP().toString());
        } else {
            tft.print("Status: Terputus dari jaringan.");
        }
        
        drawBtn(8, 120, 464, 50, C_ACCENT, C_WHITE, "CARI WIFI DI SEKITAR", 1, 8);
        drawBtn(8, 190, 464, 50, C_ERROR, C_WHITE, "HAPUS WIFI TERSIMPAN", 1, 8);
    }
    else if (wifiSubState == W_SCAN) {
        tft.setFont(); tft.setTextSize(2); tft.setTextColor(C_WHITE, C_BG);
        tft.setCursor(120, 150); tft.print("Mencari WiFi...");
    }
    else if (wifiSubState == W_LIST) {
        int startIdx = wifiListPage * 4;
        int count = min(4, wifiFoundCount - startIdx);
        if (wifiFoundCount == 0) {
            tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_WHITE, C_BG);
            tft.setCursor(16, 80); tft.print("Tidak ada WiFi ditemukan.");
        }
        for (int i=0; i<count; i++) {
            int y = 52 + i * 52;
            String ssid = WiFi.SSID(startIdx + i);
            int encType = WiFi.encryptionType(startIdx + i);
            bool isEnt = (encType == WIFI_AUTH_WPA2_ENTERPRISE);
            String encStr = (encType == WIFI_AUTH_OPEN) ? "Terbuka" : (isEnt ? "WPA2 Enterprise (Kampus)" : "Terkunci WPA");
            
            tft.fillRoundRect(8, y, 464, 46, 6, C_SURFACE);
            tft.drawRoundRect(8, y, 464, 46, 6, C_BORDER);
            tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_WHITE, C_SURFACE);
            tft.setCursor(16, y+8); tft.print(ssid);
            tft.setTextColor(C_LGRAY, C_SURFACE);
            tft.setCursor(16, y+24); tft.print(encStr);
        }
        if (wifiListPage > 0) drawBtn(8, 270, 100, 40, C_KEY_ACT, C_WHITE, "< Prev", 1, 6);
        if (startIdx + 4 < wifiFoundCount) drawBtn(372, 270, 100, 40, C_KEY_ACT, C_WHITE, "Next >", 1, 6);
    }
    else if (wifiSubState == W_INPUT) {
        if (kbV) {
            tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_LGRAY,C_BG);
            tft.setCursor(8,52); tft.print("SSID: "); tft.print(inputSSID);
            if (inputIsEnt) {
                tft.setCursor(8,68); tft.print("User:");
                drawField(48,60,424,28,inputUser,editingField==FIELD_USER,"Username...");
                tft.setCursor(8,98); tft.print("Pass:");
                drawField(48,90,380,28,inputPass,editingField==FIELD_PASS,"Password...",!showPassword);
                drawBtn(432,90,40,28,C_KEY_ACT,C_WHITE,showPassword?"abc":"***",1,4);
            } else {
                tft.setCursor(8,80); tft.print("Password:");
                drawField(8,92,422,34,inputPass,editingField==FIELD_PASS,"Password...",!showPassword);
                drawBtn(434,92,38,34,C_KEY_ACT,C_WHITE,showPassword?"abc":"***",1,4);
            }
            drawKeyboard();
        } else {
            tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_WHITE, C_BG);
            tft.setCursor(10, 60); tft.print("Menghubungkan ke: "); tft.print(inputSSID);
            
            if (inputIsEnt) {
                tft.setTextColor(C_ACCENT, C_BG);
                tft.setCursor(10, 80); tft.print("Jaringan ini butuh Username (Enterprise)");
                tft.setTextColor(C_LGRAY, C_BG);
                tft.setCursor(10, 104); tft.print("Username:");
                drawField(8, 114, 464, 40, inputUser, false, "Sentuh untuk isi username...");
                tft.setCursor(10, 162); tft.print("Password:");
                drawField(8, 172, 412, 40, inputPass, false, "Sentuh untuk isi password...", !showPassword);
                drawBtn(428, 172, 44, 40, C_KEY_ACT, C_WHITE, showPassword?"abc":"***", 1, 4);
            } else {
                tft.setTextColor(C_LGRAY, C_BG);
                tft.setCursor(10, 100); tft.print("Password:");
                drawField(8, 114, 412, 40, inputPass, false, "Sentuh untuk isi password...", !showPassword);
                drawBtn(428, 114, 44, 40, C_KEY_ACT, C_WHITE, showPassword?"abc":"***", 1, 4);
            }
            drawBtn(8, 230, 224, 44, C_SUCCESS, C_WHITE, "SAMBUNGKAN", 1, 8);
            drawBtn(248, 230, 224, 44, C_ERROR, C_WHITE, "BATAL", 1, 8);
        }
    }
}

void showConnectingOverlay(const String &ssid) {
    tft.fillRoundRect(80,100,320,120,10,C_SURFACE);
    tft.drawRoundRect(80,100,320,120,10,C_ACCENT);
    tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_LGRAY,C_SURFACE);
    tft.setCursor(100,116); tft.print("Menghubungkan ke:");
    tft.setTextSize(2); tft.setTextColor(C_WHITE,C_SURFACE);
    tft.setCursor(100,136); tft.print(ssid.length()>20?ssid.substring(0,20):ssid);
    tft.setTextSize(1); tft.setTextColor(C_ACCENT,C_SURFACE);
    tft.setCursor(100,162); tft.print("Harap tunggu (maks 20 detik)...");
    for (int i=0;i<5;i++) tft.fillCircle(170+i*22,192,7,C_ACCENT);
}

void showConnectResult(bool ok, const String &ssid) {
    uint16_t col=ok?C_SUCCESS:C_ERROR;
    tft.fillRoundRect(80,100,320,120,10,C_SURFACE);
    tft.drawRoundRect(80,100,320,120,10,col);
    tft.setFont(); tft.setTextSize(2); tft.setTextColor(col,C_SURFACE);
    tft.setCursor(100,116); tft.print(ok?"Berhasil!":"Gagal!");
    tft.setTextSize(1); tft.setTextColor(C_LGRAY,C_SURFACE);
    tft.setCursor(100,146);
    if (ok) {
        tft.print("Terhubung: "); tft.print(ssid);
        tft.setCursor(100,160); tft.print("IP: "); tft.print(WiFi.localIP().toString());
        tft.setCursor(100,174); tft.print("Tersimpan ke EEPROM.");
    } else {
        tft.print("Tidak bisa terhubung.");
        tft.setCursor(100,160); tft.print("Periksa SSID dan Password.");
    }
    delay(3000);
}
