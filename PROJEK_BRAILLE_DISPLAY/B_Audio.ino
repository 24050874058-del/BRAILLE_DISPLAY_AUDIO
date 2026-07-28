// ============================================================
// AUDIO / TTS
// ============================================================

void initAudio() {
    Serial.println("[Audio] Init I2S...");
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(100);
    Serial.print("[Audio] BCLK="); Serial.print(I2S_BCLK);
    Serial.print(" LRC="); Serial.print(I2S_LRC);
    Serial.print(" DATA="); Serial.println(I2S_DOUT);
    delay(800);
}

void speak(const String &text) {
    if (WiFi.status() == WL_CONNECTED) {
        // Kecepatan suara bisa diatur lewat parameter URL
        audio.connecttospeech(text.c_str(), getTtsSpeedParam().c_str());
    }
}
