// ============================================================
// TOUCH HANDLING
// ============================================================

void touchToScreen(int16_t rawX, int16_t rawY, int16_t &sx, int16_t &sy) {
    sx = constrain(map(rawY, 3750, 320, 0, 479), 0, 479);
    sy = constrain(map(rawX,  350, 3700, 0, 319), 0, 319);
}

char handleKbTouch(int16_t tx,int16_t ty) {
    if (ty<KB_Y-4) return '\0';
    // ROW 0
    int r0t=10*KW+9*KGAP, r0x=(480-r0t)/2;
    if (inRect(tx,ty,r0x,KB_Y,r0t,KH)) {
        int i=(tx-r0x)/(KW+KGAP); if(i>=0&&i<10) return KB_ROW0[kbPage][i];
    }
    // ROW 1
    int r1l=strlen(KB_ROW1[kbPage]), r1t=r1l*KW+(r1l-1)*KGAP;
    int r1x=(480-r1t)/2, r1y=KB_Y+KH+KGAP;
    if (inRect(tx,ty,r1x,r1y,r1t,KH)) {
        int i=(tx-r1x)/(KW+KGAP); if(i>=0&&i<r1l) return KB_ROW1[kbPage][i];
    }
    // ROW 2
    int r2l=strlen(KB_ROW2[kbPage]), shW=56, bsW=56;
    int r2t=shW+KGAP+r2l*(KW+KGAP)-KGAP+KGAP+bsW;
    int r2x=(480-r2t)/2, r2y=KB_Y+2*(KH+KGAP);
    if (inRect(tx,ty,r2x,r2y,shW,KH)) return '\t';
    int chX=r2x+shW+KGAP;
    if (inRect(tx,ty,chX,r2y,r2l*(KW+KGAP)-KGAP,KH)) {
        int i=(tx-chX)/(KW+KGAP); if(i>=0&&i<r2l) return KB_ROW2[kbPage][i];
    }
    if (inRect(tx,ty,r2x+shW+KGAP+r2l*(KW+KGAP),r2y,bsW,KH)) return '\b';
    // ROW 3
    int r3y=KB_Y+3*(KH+KGAP), fnW=80, okW=80, r3x=8;
    int spW=480-(r3x*2)-fnW-KGAP-KGAP-okW;
    if (inRect(tx,ty,r3x,              r3y,fnW,KH)) return '\x01';
    if (inRect(tx,ty,r3x+fnW+KGAP,    r3y,spW,KH)) return ' ';
    if (inRect(tx,ty,r3x+fnW+KGAP+spW+KGAP,r3y,okW,KH)) return '\r';
    return '\0';
}

void handleTouchMain(int16_t tx,int16_t ty) {
    if (inRect(tx,ty,436,0,44,44)) {
        Serial.println("[Touch] -> Buka Pengaturan WiFi");
        wifiSubState = W_MENU;
        editingField=FIELD_NONE; kbPage=0;
        currentScreen=SCR_WIFI; drawWiFiScreen();
        return;
    }
    // Tombol Kecepatan Suara
    if (inRect(tx,ty,220,283,254,24)) {
        ttsSpeedMode = (ttsSpeedMode + 1) % 3;
        Serial.print("[Touch] -> Ubah Kecepatan Suara ke: "); Serial.println(ttsSpeedMode);
        eepromSaveSpeed();
        if (ttsSpeedMode == 0) speak("Normal");
        else if (ttsSpeedMode == 1) speak("Lambat");
        else speak("Sangat Lambat");
        drawMainScreen();
        return;
    }
    
    // Tombol Kalkulator di layar sentuh
    if (currentMode == 3) {
        if (ty >= 230 && ty <= 270) {
            if (tx >= 224 && tx <= 268) { appendMathOp('+'); return; }
            if (tx >= 272 && tx <= 316) { appendMathOp('-'); return; }
            if (tx >= 320 && tx <= 364) { appendMathOp('*'); return; }
            if (tx >= 368 && tx <= 412) { appendMathOp('/'); return; }
            if (tx >= 416 && tx <= 468) { calculateResult(); return; }
        }
    }
}

void handleTouchWiFi(int16_t tx,int16_t ty) {
    bool kbV=(editingField!=FIELD_NONE);
    // Tombol Kembali
    if (inRect(tx,ty,0,0,90,44)) {
        if (wifiSubState == W_MENU) {
            editingField=FIELD_NONE; currentScreen=SCR_MAIN; drawMainScreen(); return;
        } else if (wifiSubState == W_LIST) {
            wifiSubState = W_MENU; drawWiFiScreen(); return;
        } else if (wifiSubState == W_INPUT) {
            if (kbV) { editingField=FIELD_NONE; drawWiFiScreen(); return; }
            wifiSubState = W_LIST; drawWiFiScreen(); return;
        }
    }
    if (wifiSubState == W_MENU) {
        if (inRect(tx,ty,8,120,464,50)) {
            wifiSubState = W_SCAN; drawWiFiScreen();
            wifiFoundCount = WiFi.scanNetworks();
            wifiListPage = 0;
            wifiSubState = W_LIST; drawWiFiScreen();
            return;
        }
        if (inRect(tx,ty,8,190,464,50)) {
            eepromClearWiFi();
            WIFI_SSID=WIFI_PASSWORD=WIFI_USER=""; WIFI_IS_ENT=false;
            WiFi.disconnect(true); drawWiFiScreen(); return;
        }
        return;
    }
    
    if (wifiSubState == W_LIST) {
        int startIdx = wifiListPage * 4;
        int count = min(4, wifiFoundCount - startIdx);
        for (int i=0; i<count; i++) {
            if (inRect(tx,ty,8,52+i*52,464,46)) {
                inputSSID = WiFi.SSID(startIdx + i);
                inputIsEnt = (WiFi.encryptionType(startIdx + i) == WIFI_AUTH_WPA2_ENTERPRISE);
                inputPass = ""; inputUser = "";
                wifiSubState = W_INPUT;
                drawWiFiScreen();
                return;
            }
        }
        if (wifiListPage > 0 && inRect(tx,ty,8,270,100,40)) { wifiListPage--; drawWiFiScreen(); return; }
        if (startIdx + 4 < wifiFoundCount && inRect(tx,ty,372,270,100,40)) { wifiListPage++; drawWiFiScreen(); return; }
        return;
    }
    
    if (wifiSubState == W_INPUT) {
        // Keyboard area
        if (kbV && ty>=KB_Y-4) {
            char k=handleKbTouch(tx,ty);
            if (k=='\0') return;
            String *tgt=(editingField==FIELD_USER)?&inputUser:&inputPass;
            if      (k=='\b') { if(tgt->length()>0) tgt->remove(tgt->length()-1); }
            else if (k=='\r') { editingField=FIELD_NONE; drawWiFiScreen(); return; }
            else if (k=='\t') { kbPage=(kbPage==0)?1:0; drawKeyboard(); return; }
            else if (k=='\x01') { kbPage=(kbPage<2)?2:0; drawKeyboard(); return; }
            else if (tgt->length()<(uint16_t)EEPROM_MAX_LEN) *tgt+=k;
            
            if (inputIsEnt) {
                if (editingField==FIELD_USER) drawField(48,60,424,28,inputUser,true,"Username...");
                else drawField(48,90,380,28,inputPass,true,"Password...",!showPassword);
            } else {
                if (editingField==FIELD_PASS) drawField(8,92,422,34,inputPass,true,"Password...",!showPassword);
            }
            return;
        }
        if (kbV) {
            if (inputIsEnt) {
                if (inRect(tx,ty,48,60,424,28)) { editingField=FIELD_USER; kbPage=0; drawWiFiScreen(); return; }
                if (inRect(tx,ty,48,90,380,28)) { editingField=FIELD_PASS; kbPage=0; drawWiFiScreen(); return; }
                if (inRect(tx,ty,432,90,40,28)) {
                    showPassword=!showPassword;
                    drawField(48,90,380,28,inputPass,editingField==FIELD_PASS,"Password...",!showPassword);
                    drawBtn(432,90,40,28,C_KEY_ACT,C_WHITE,showPassword?"abc":"***",1,4);
                }
            } else {
                if (inRect(tx,ty,8,92,422,34)) { editingField=FIELD_PASS; kbPage=0; drawWiFiScreen(); return; }
                if (inRect(tx,ty,434,92,38,34)) {
                    showPassword=!showPassword;
                    drawField(8,92,422,34,inputPass,editingField==FIELD_PASS,"Password...",!showPassword);
                    drawBtn(434,92,38,34,C_KEY_ACT,C_WHITE,showPassword?"abc":"***",1,4);
                }
            }
        } else {
            if (inputIsEnt) {
                if (inRect(tx,ty,8,114,464,40)) { editingField=FIELD_USER; kbPage=0; drawWiFiScreen(); return; }
                if (inRect(tx,ty,8,172,412,40)) { editingField=FIELD_PASS; kbPage=0; drawWiFiScreen(); return; }
                if (inRect(tx,ty,428,172,44,40)) {
                    showPassword=!showPassword;
                    drawField(8,172,412,40,inputPass,false,"Sentuh untuk isi password...",!showPassword);
                    drawBtn(428,172,44,40,C_KEY_ACT,C_WHITE,showPassword?"abc":"***",1,4);
                }
            } else {
                if (inRect(tx,ty,8,114,412,40)) { editingField=FIELD_PASS; kbPage=0; drawWiFiScreen(); return; }
                if (inRect(tx,ty,428,114,44,40)) {
                    showPassword=!showPassword;
                    drawField(8,114,412,40,inputPass,false,"Sentuh untuk isi password...",!showPassword);
                    drawBtn(428,114,44,40,C_KEY_ACT,C_WHITE,showPassword?"abc":"***",1,4);
                }
            }
            // SAMBUNGKAN
            if (inRect(tx,ty,8,230,224,44)) {
                WIFI_SSID=inputSSID; WIFI_PASSWORD=inputPass; 
                WIFI_USER=inputUser; WIFI_IS_ENT=inputIsEnt;
                showConnectingOverlay(WIFI_SSID);
                bool ok=connectWiFi(20000);
                if (ok) eepromSaveWiFi(WIFI_SSID,WIFI_PASSWORD,WIFI_USER,WIFI_IS_ENT);
                showConnectResult(ok,WIFI_SSID);
                wifiSubState=W_MENU; drawWiFiScreen(); return;
            }
            // BATAL
            if (inRect(tx,ty,248,230,224,44)) {
                wifiSubState=W_LIST; drawWiFiScreen(); return;
            }
        }
    }
}

void handleTouch() {
    if (!ts.tirqTouched()) return;
    if (!ts.touched()) return;
    
    unsigned long now=millis();
    if (now-lastTouchMs<TOUCH_DEBOUNCE_MS) return;
    lastTouchMs=now;
    
    TS_Point p=ts.getPoint();
    int16_t tx,ty;
    touchToScreen(p.x,p.y,tx,ty);
    
    Serial.print("[Touch] Raw("); Serial.print(p.x);
    Serial.print(","); Serial.print(p.y);
    Serial.print(") -> Screen("); Serial.print(tx);
    Serial.print(","); Serial.print(ty); Serial.println(")");
    
    switch(currentScreen) {
        case SCR_MAIN: handleTouchMain(tx,ty); break;
        case SCR_WIFI: handleTouchWiFi(tx,ty); break;
    }
}
