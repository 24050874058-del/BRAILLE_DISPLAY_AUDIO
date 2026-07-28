// ============================================================
// WIFI CONNECTION & LOGIC
// ============================================================

bool connectWiFi(unsigned long timeoutMs) {
    Serial.print("[WiFi] Menghubungkan ke ["); Serial.print(WIFI_SSID); Serial.print("] ");
    WiFi.disconnect(true); delay(500);
    WiFi.mode(WIFI_STA); WiFi.setAutoReconnect(true);
    
    if (WIFI_IS_ENT) {
        Serial.println("(WPA2-Enterprise)");
#if __has_include("esp_eap_client.h")
        esp_eap_client_set_identity((uint8_t *)WIFI_USER.c_str(), WIFI_USER.length());
        esp_eap_client_set_username((uint8_t *)WIFI_USER.c_str(), WIFI_USER.length());
        esp_eap_client_set_password((uint8_t *)WIFI_PASSWORD.c_str(), WIFI_PASSWORD.length());
        esp_wifi_sta_enterprise_enable();
#elif __has_include("esp_wpa2.h")
        esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)WIFI_USER.c_str(), WIFI_USER.length());
        esp_wifi_sta_wpa2_ent_set_username((uint8_t *)WIFI_USER.c_str(), WIFI_USER.length());
        esp_wifi_sta_wpa2_ent_set_password((uint8_t *)WIFI_PASSWORD.c_str(), WIFI_PASSWORD.length());
        esp_wifi_sta_wpa2_ent_enable();
#endif
        WiFi.begin(WIFI_SSID.c_str());
    } else {
        if (WIFI_PASSWORD.length()==0||WIFI_PASSWORD=="-") WiFi.begin(WIFI_SSID.c_str());
        else WiFi.begin(WIFI_SSID.c_str(),WIFI_PASSWORD.c_str());
    }
    
    unsigned long t=millis();
    while (WiFi.status()!=WL_CONNECTED) {
        delay(500); Serial.print(".");
        if (millis()-t>timeoutMs) {
            WiFi.disconnect(true);
            Serial.println(" Timeout! Gagal.");
            int st=WiFi.status();
            if (st==4) Serial.println("[WiFi] Password salah!");
            else if (st==WL_NO_SSID_AVAIL) Serial.println("[WiFi] SSID tidak ditemukan!");
            return false;
        }
    }
    Serial.println(" Terhubung!");
    Serial.print("[WiFi] IP: "); Serial.println(WiFi.localIP());
    return true;
}

bool autoConnectWiFi() {
    String ssid,pass,user; bool isEnt;
    if (!eepromLoadWiFi(ssid,pass,user,isEnt)) {
        Serial.println("[WiFi] Tidak ada kredensial di EEPROM.");
        return false;
    }
    Serial.print("[WiFi] Auto-konek ke ["); Serial.print(ssid); Serial.println("]");
    WIFI_SSID=ssid; WIFI_PASSWORD=pass; WIFI_USER=user; WIFI_IS_ENT=isEnt;
    if (connectWiFi(15000)) return true;
    Serial.println("[WiFi] Jaringan tidak tersedia. Lanjut tanpa WiFi.");
    return false;
}
