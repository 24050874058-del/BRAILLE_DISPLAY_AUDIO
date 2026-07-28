// ============================================================
// BRAILLE LOGIC
// ============================================================

char decodeBraille(uint8_t p, bool isNum) {
    if (isNum) { 
        switch(p) {
            case 0b000001: return '1'; case 0b000011: return '2';
            case 0b001001: return '3'; case 0b011001: return '4';
            case 0b010001: return '5'; case 0b001011: return '6';
            case 0b011011: return '7'; case 0b010011: return '8';
            case 0b001010: return '9'; case 0b011010: return '0';
            default: return '?'; 
        } 
    }
    switch(p) {
        case 0b000001: return 'a'; case 0b000011: return 'b';
        case 0b001001: return 'c'; case 0b011001: return 'd';
        case 0b010001: return 'e'; case 0b001011: return 'f';
        case 0b011011: return 'g'; case 0b010011: return 'h';
        case 0b001010: return 'i'; case 0b011010: return 'j';
        case 0b000101: return 'k'; case 0b000111: return 'l';
        case 0b001101: return 'm'; case 0b011101: return 'n';
        case 0b010101: return 'o'; case 0b001111: return 'p';
        case 0b011111: return 'q'; case 0b010111: return 'r';
        case 0b001110: return 's'; case 0b011110: return 't';
        case 0b100101: return 'u'; case 0b100111: return 'v';
        case 0b111010: return 'w'; case 0b101101: return 'x';
        case 0b111101: return 'y'; case 0b110101: return 'z';
        default: return '?';
    }
}

void handleButtonPress(int pin) {
    if (pin>=0 && pin<=5) {
        currentPattern^=(1<<pin);
        Serial.print("[Braille] Titik "); Serial.print(pin+1);
        Serial.println((currentPattern&(1<<pin))?" ON":" OFF");
        Serial.print("[Braille] Pola: 0b");
        for (int i=5;i>=0;i--) Serial.print((currentPattern>>i)&1);
        Serial.println();
        
        if (currentScreen==SCR_MAIN) {
            int dotR=22, dotGapX=78, dotGapY=58, sX=55, sY=148;
            for (int i=0;i<6;i++) {
                int col=i/3, row=i%3;
                int cx=sX+col*dotGapX, cy=sY+row*dotGapY;
                bool act=(currentPattern>>i)&1;
                tft.fillCircle(cx,cy,dotR,act?C_ACCENT:C_DGRAY);
                tft.drawCircle(cx,cy,dotR,act?C_WHITE:C_GRAY);
                tft.setFont(); tft.setTextSize(2);
                tft.setTextColor(act?C_BG:C_GRAY,act?C_ACCENT:C_DGRAY);
                tft.setCursor(cx-5,cy-8); tft.print(i+1);
            }
        }
    }
    else if (pin==6) { // ENTER
        if (currentPattern==0) { Serial.println("[Braille] Pola kosong, abaikan."); return; }
        char c=decodeBraille(currentPattern,(currentMode==1));
        if (c=='?') {
            Serial.println("[Braille] Pola tidak dikenal!");
            speak("Pola salah");
        } else {
            Serial.print("[Braille] Karakter: "); Serial.println(c);
            if (currentMode==0||currentMode==1) {
                lastChar = c;
                speak(String(c));
                if (currentScreen==SCR_MAIN) drawMainScreen();
            } else if (currentMode==2) {
                currentWord+=c;
                Serial.print("[Braille] Kata: "); Serial.println(currentWord);
                speak(String(c));
                if (currentScreen==SCR_MAIN) drawMainScreen(); // redraw full to clear transparent font overlap
            }
        }
        currentPattern=0;
    }
    else if (pin==7) { // SPASI
        if (currentMode==2 && currentWord.length()>0) {
            Serial.print("[Braille] Spasi. Ucapkan kata: "); Serial.println(currentWord);
            speak(currentWord);
            currentWord="";
            if (currentScreen==SCR_MAIN) drawMainScreen();
        } else {
            Serial.println("[Braille] Spasi hanya di mode Kata.");
        }
    }
    else if (pin==8) { // HAPUS
        if (currentMode==2 && currentWord.length()>0) {
            currentWord.remove(currentWord.length()-1);
            Serial.print("[Braille] Hapus. Kata: "); Serial.println(currentWord);
            speak("Hapus");
            if (currentScreen==SCR_MAIN) drawMainScreen();
        }
        currentPattern=0;
    }
    else if (pin==9) { // GANTI MODE
        currentMode=(currentMode+1)%3;
        currentPattern=0; currentWord=""; lastChar='-';
        const char* mn[]={"Mode Huruf","Mode Angka","Mode Kata"};
        Serial.print("[Braille] "); Serial.println(mn[currentMode]);
        speak(mn[currentMode]);
        if (currentScreen==SCR_MAIN) drawMainScreen();
    }
}

void checkButtons() {
    uint16_t readState=mcp1.readGPIOAB();
    if (readState!=lastFlickerState) { lastDebounceTime=millis(); lastFlickerState=readState; }
    if ((millis()-lastDebounceTime)>50) {
        if (readState!=lastButtonState) {
            for (int i=0;i<10;i++) {
                bool isP=!(readState&(1<<i));
                bool wasP=!(lastButtonState&(1<<i));
                if (isP&&!wasP) handleButtonPress(i);
            }
            lastButtonState=readState;
        }
    }
}
