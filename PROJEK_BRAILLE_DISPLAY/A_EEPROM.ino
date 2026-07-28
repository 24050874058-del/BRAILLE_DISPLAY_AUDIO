// ============================================================
// EEPROM MANAGEMENT
// ============================================================

String getTtsSpeedParam() {
    if (ttsSpeedMode == 0) return "id";
    if (ttsSpeedMode == 1) return "id&ttsspeed=0.6";
    return "id&ttsspeed=0.3";
}

void eepromSaveSpeed() {
    EEPROM.write(EEPROM_SPEED_ADDR, ttsSpeedMode);
    EEPROM.commit();
}

void eepromLoadSpeed() {
    ttsSpeedMode = EEPROM.read(EEPROM_SPEED_ADDR);
    if (ttsSpeedMode > 2) ttsSpeedMode = 2; // Default sangat lambat
}

void eepromSaveWiFi(const String &ssid, const String &pass, const String &user, bool isEnt) {
    EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VAL);
    int ls = min((int)ssid.length(), EEPROM_MAX_LEN);
    for (int i=0; i<ls; i++) EEPROM.write(EEPROM_SSID_ADDR+i, ssid[i]);
    EEPROM.write(EEPROM_SSID_ADDR+ls, 0);
    
    int lp = min((int)pass.length(), EEPROM_MAX_LEN);
    for (int i=0; i<lp; i++) EEPROM.write(EEPROM_PASS_ADDR+i, pass[i]);
    EEPROM.write(EEPROM_PASS_ADDR+lp, 0);
    
    int lu = min((int)user.length(), EEPROM_MAX_LEN);
    for (int i=0; i<lu; i++) EEPROM.write(EEPROM_USER_ADDR+i, user[i]);
    EEPROM.write(EEPROM_USER_ADDR+lu, 0);
    
    EEPROM.write(EEPROM_ENT_ADDR, isEnt ? 1 : 0);
    EEPROM.commit();
    Serial.println("[EEPROM] Kredensial disimpan.");
}

bool eepromLoadWiFi(String &ssid, String &pass, String &user, bool &isEnt) {
    if (EEPROM.read(EEPROM_MAGIC_ADDR) != EEPROM_MAGIC_VAL) return false;
    
    ssid = ""; 
    for (int i=0; i<EEPROM_MAX_LEN; i++) { 
        char c=(char)EEPROM.read(EEPROM_SSID_ADDR+i); 
        if(!c) break; 
        ssid+=c; 
    }
    
    pass = ""; 
    for (int i=0; i<EEPROM_MAX_LEN; i++) { 
        char c=(char)EEPROM.read(EEPROM_PASS_ADDR+i); 
        if(!c) break; 
        pass+=c; 
    }
    
    user = ""; 
    for (int i=0; i<EEPROM_MAX_LEN; i++) { 
        char c=(char)EEPROM.read(EEPROM_USER_ADDR+i); 
        if(!c) break; 
        user+=c; 
    }
    
    isEnt = (EEPROM.read(EEPROM_ENT_ADDR) == 1);
    return ssid.length() > 0;
}

void eepromClearWiFi() {
    EEPROM.write(EEPROM_MAGIC_ADDR, 0x00);
    EEPROM.commit();
    Serial.println("[EEPROM] Kredensial dihapus.");
}
