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
void drawGearIcon(int16_t cx, int16_t cy, int16_t r_out, int16_t r_in, uint16_t color) {
    tft.drawCircle(cx, cy, r_out, color);
    tft.drawCircle(cx, cy, r_in, color);
    for (int i = 0; i < 8; i++) {
        float angle = i * PI / 4.0;
        int16_t x0 = cx + cos(angle) * r_out;
        int16_t y0 = cy + sin(angle) * r_out;
        int16_t x1 = cx + cos(angle) * (r_out + 4);
        int16_t y1 = cy + sin(angle) * (r_out + 4);
        tft.drawLine(x0, y0, x1, y1, color);
        tft.drawLine(x0+1, y0, x1+1, y1, color);
        tft.drawLine(x0, y0+1, x1, y1+1, color);
    }
}

void drawWiFiIcon(int16_t cx, int16_t cy, uint16_t color) {
    // cx, cy is the center of the dot (bottom of the WiFi symbol)
    tft.fillCircle(cx, cy, 2, color);
    tft.drawCircle(cx, cy, 6, color);
    tft.fillRect(cx - 7, cy + 1, 14, 7, C_PRIMARY);
    tft.drawCircle(cx, cy, 11, color);
    tft.fillRect(cx - 12, cy + 1, 24, 12, C_PRIMARY);
    tft.drawCircle(cx, cy, 16, color);
    tft.fillRect(cx - 17, cy + 1, 34, 17, C_PRIMARY);
}

void drawWiFiIndicator() {
    if (currentScreen != SCR_MAIN) return;
    bool wOK = (WiFi.status() == WL_CONNECTED);
    tft.fillRect(395, 10, 40, 30, C_PRIMARY);
    drawWiFiIcon(415, 34, wOK ? C_SUCCESS : C_ERROR);
}

String getPatternDotsString(uint8_t p) {
    if (p == 0) return "-";
    String s = "";
    for (int i = 0; i < 6; i++) {
        if ((p >> i) & 1) {
            if (s.length() > 0) s += ", ";
            s += String(i + 1);
        }
    }
    return s;
}

void drawCenteredText(const String &txt, int16_t bx, int16_t by, int16_t bw, int16_t bh, uint16_t color, bool useLargeFont) {
    tft.setTextColor(color);
    int16_t x1, y1;
    uint16_t tw, th;
    
    if (useLargeFont && txt.length() <= 4) {
        tft.setFont(&FreeSansBold24pt7b);
        tft.setTextSize(1);
    } else {
        tft.setFont(&FreeSansBold12pt7b);
        tft.setTextSize(1);
    }
    
    tft.getTextBounds(txt, 0, 0, &x1, &y1, &tw, &th);
    
    if (tw > bw - 16) {
        tft.setFont();
        tft.setTextSize(2);
        tft.getTextBounds(txt, 0, 0, &x1, &y1, &tw, &th);
    }
    
    tft.setCursor(bx + (bw - tw) / 2, by + (bh + th) / 2 - 2);
    tft.print(txt);
}

void drawStatusBadge() {
    if (currentScreen != SCR_MAIN) return;
    
    // Clear the entire badge bar area first (from x = 245 to 475, y = 60 to 85) with C_BG
    tft.fillRect(245, 60, 230, 26, C_BG);
    
    const char* modeNames[]={"HURUF","ANGKA","KATA","KALKULATOR"};
    String modeText = "MODE: " + String(modeNames[currentMode]);
    int16_t x1, y1; uint16_t tw, th;
    tft.setFont(); tft.setTextSize(1);
    tft.getTextBounds(modeText, 0, 0, &x1, &y1, &tw, &th);
    
    // ponytail: auto-width badge for longer mode names
    int badgeW = max((int)tw + 16, 105);
    int badgeX = 250 + (220 - badgeW) / 2;
    tft.fillRoundRect(badgeX, 60, badgeW, 24, 4, C_PRIMARY);
    tft.setTextColor(C_WHITE);
    tft.setCursor(badgeX + (badgeW - tw) / 2, 60 + (24 - th) / 2);
    tft.print(modeText);
}

void drawInputCardOnly() {
    if (currentScreen != SCR_MAIN) return;
    tft.fillRoundRect(250, 94, 220, 100, 8, C_SURFACE);
    tft.drawRoundRect(250, 94, 220, 100, 8, C_BORDER);
    
    tft.setFont(); tft.setTextSize(1);
    tft.setTextColor(C_LGRAY);
    tft.setCursor(258, 102); tft.print("INPUT PENGGUNA");
    
    if (currentMode == 3) {
        String val = calcExpression.length() ? calcExpression : "0";
        drawCenteredText(val, 250, 112, 220, 48, C_WARNING, true);
        tft.setFont(); tft.setTextSize(1);
        tft.setTextColor(C_DGRAY);
        tft.setCursor(258, 178); tft.print("Ekspresi kalkulator");
    } else if (currentMode == 2) {
        String val = currentWord.length() ? currentWord : "-";
        drawCenteredText(val, 250, 112, 220, 48, C_WHITE, true);
        tft.setFont(); tft.setTextSize(1);
        tft.setTextColor(C_DGRAY);
        tft.setCursor(258, 178); tft.print("Sedang mengetik...");
    } else {
        String val = getPatternDotsString(currentPattern);
        drawCenteredText(val, 250, 112, 220, 48, C_ACCENT, true);
        tft.setFont(); tft.setTextSize(1);
        tft.setTextColor(C_DGRAY);
        String binStr = "0b";
        for (int i=5; i>=0; i--) binStr += String((currentPattern>>i)&1);
        int16_t x1, y1; uint16_t tw, th;
        tft.getTextBounds(binStr, 0, 0, &x1, &y1, &tw, &th);
        tft.setCursor(250 + (220 - tw) / 2, 178);
        tft.print(binStr);
    }
}

void drawOutputCardOnly() {
    if (currentScreen != SCR_MAIN) return;
    tft.fillRoundRect(250, 206, 220, 100, 8, C_SURFACE);
    tft.drawRoundRect(250, 206, 220, 100, 8, C_BORDER);
    
    tft.setFont(); tft.setTextSize(1);
    tft.setTextColor(C_LGRAY);
    tft.setCursor(258, 214); tft.print("RESPONS SISTEM");
    
    if (currentMode == 3) {
        String val = calcResult.length() ? calcResult : "-";
        drawCenteredText(val, 250, 224, 220, 48, C_SUCCESS, true);
        tft.setFont(); tft.setTextSize(1);
        tft.setTextColor(C_DGRAY);
        tft.setCursor(258, 290); tft.print("Hasil kalkulasi");
    } else if (currentMode == 2) {
        String val = lastSpokenWord.length() ? lastSpokenWord : "-";
        drawCenteredText(val, 250, 224, 220, 48, C_SUCCESS, true);
        tft.setFont(); tft.setTextSize(1);
        tft.setTextColor(C_DGRAY);
        tft.setCursor(258, 290); tft.print("Kata terakhir diucapkan");
    } else {
        String val = String(lastChar);
        drawCenteredText(val, 250, 224, 220, 48, C_ACCENT, true);
        tft.setFont(); tft.setTextSize(1);
        tft.setTextColor(C_DGRAY);
        tft.setCursor(258, 290);
        tft.print(currentMode == 0 ? "Huruf terakhir terjemahan" : "Angka terakhir terjemahan");
    }
}

void drawBrailleDots() {
    int dotR=30, dotGapX=95, dotGapY=70;
    int sX = 120; 
    int sY = 185; 
    for (int i=0;i<6;i++) {
        int col=i/3; 
        int row=i%3; 
        int cx = sX - (dotGapX / 2) + col * dotGapX;
        int cy = sY - dotGapY + row * dotGapY;
        bool act=(currentPattern>>i)&1;
        
        tft.fillCircle(cx,cy,dotR+2,C_BG);
        tft.fillCircle(cx,cy,dotR+2,act?C_WHITE:C_GRAY);
        tft.fillCircle(cx,cy,dotR,act?C_ACCENT:C_DGRAY);
        
        tft.setFont(&FreeSansBold12pt7b); tft.setTextSize(1);
        tft.setTextColor(act?C_WHITE:C_GRAY);
        int16_t x1,y1; uint16_t tw,th;
        tft.getTextBounds(String(i+1),0,0,&x1,&y1,&tw,&th);
        tft.setCursor(cx-tw/2, cy+th/2);
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
    
    // Settings icon (Gear)
    drawGearIcon(455, 25, 10, 4, C_WHITE);
    
    // WiFi status indicator next to settings icon
    drawWiFiIndicator();
    
    // ======= DIVIDER VERTIKAL =======
    tft.drawFastVLine(240,50,270,C_BORDER);
    
    // ======= BRAILLE DOTS (kiri) =======
    drawBrailleDots();
    
    // ======= PANEL KANAN =======
    // 1. Badges
    drawStatusBadge();
    
    // 2. Input Card
    drawInputCardOnly();
    
    // 3. Output Card
    drawOutputCardOnly();
}

void refreshMainWiFiStatus() {
    if (currentScreen == SCR_MAIN) {
        drawMainScreen();
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
    tft.setCursor(100,12); tft.print("Pengaturan");
    
    if (wifiSubState == W_MENU) {
        uint16_t stBg=wOK?0x0320:0x3000;
        tft.fillRoundRect(8,52,464,42,6,stBg);
        tft.setFont(); tft.setTextSize(1); tft.setTextColor(C_WHITE,stBg);
        tft.setCursor(16,60);
        if (wOK) {
            tft.print("WiFi: Terhubung ke "); tft.print(WIFI_SSID);
            tft.setCursor(16,74); tft.print("IP: "); tft.print(WiFi.localIP().toString());
        } else {
            tft.print("WiFi Status: Terputus dari jaringan.");
        }
        
        // Speed toggle button inside Settings Menu
        uint16_t spdBg = (ttsSpeedMode==0)?C_SUCCESS:((ttsSpeedMode==1)?C_WARNING:C_ERROR);
        uint16_t spdFg = (ttsSpeedMode==1)?C_BG:C_WHITE;
        const char* spdName[] = {"SUARA: NORMAL", "SUARA: LAMBAT", "SUARA: SANGAT LAMBAT"};
        drawBtn(8, 105, 464, 45, spdBg, spdFg, spdName[ttsSpeedMode], 1, 8);
        
        drawBtn(8, 160, 464, 45, C_ACCENT, C_WHITE, "CARI WIFI DI SEKITAR", 1, 8);
        drawBtn(8, 215, 464, 45, C_ERROR, C_WHITE, "HAPUS WIFI TERSIMPAN", 1, 8);
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
            // compact mode
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
            // normal mode
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
