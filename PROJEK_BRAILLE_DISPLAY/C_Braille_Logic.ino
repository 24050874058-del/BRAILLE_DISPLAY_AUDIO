// ============================================================
// BRAILLE LOGIC & KALKULATOR
// ============================================================

char decodeBraille(uint8_t p, int mode) {
    if (mode == 1 || mode == 3) { // Angka atau Kalkulator
        switch(p) {
            case 0b000001: return '1'; case 0b000011: return '2';
            case 0b001001: return '3'; case 0b011001: return '4';
            case 0b010001: return '5'; case 0b001011: return '6';
            case 0b011011: return '7'; case 0b010011: return '8';
            case 0b001010: return '9'; case 0b011010: return '0';
        }
        if (mode == 3) {
            switch(p) {
                case 0b010110: return '+'; // 2,3,5
                case 0b100100: return '-'; // 3,6
                case 0b100110: return '*'; // 2,3,6
                case 0b001100: return '/'; // 3,4
                case 0b110110: return '='; // 2,3,5,6
            }
        }
        if (mode == 1 || mode == 3) return '?';
    }
    
    // Mode 0 dan 2 (Huruf / Kata)
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

void calculateResult() {
    if (currentWord.length() == 0) return;
    
    int opPos = -1;
    char op = ' ';
    // Mulai dari index 1 agar angka pertama boleh minus
    for(int i=1; i<currentWord.length(); i++) {
        char c = currentWord[i];
        if(c=='+' || c=='-' || c=='*' || c=='/') { opPos = i; op = c; break; }
    }
    
    if(opPos != -1) {
        float num1 = currentWord.substring(0, opPos).toFloat();
        float num2 = currentWord.substring(opPos+1).toFloat();
        float res = 0;
        
        if(op=='+') res = num1+num2;
        else if(op=='-') res = num1-num2;
        else if(op=='*') res = num1*num2;
        else if(op=='/') {
            if (num2 != 0) res = num1/num2;
            else { 
                speak("Error bagi nol"); 
                currentWord="Err"; 
                if (currentScreen==SCR_MAIN) drawMainScreen(); 
                return; 
            }
        }
        
        String resStr = String(res, 2);
        if(resStr.endsWith(".00")) resStr = String((int)res);
        
        currentWord = resStr;
        Serial.print("[Kalkulator] Hasil: "); Serial.println(currentWord);
        speak("Sama dengan " + currentWord);
    } else {
        speak(currentWord);
    }
    if (currentScreen==SCR_MAIN) drawMainScreen();
}

void appendMathOp(char op) {
    currentWord += op;
    if (op == '+') speak("tambah");
    else if (op == '-') speak("kurang");
    else if (op == '*') speak("kali");
    else if (op == '/') speak("bagi");
    if (currentScreen==SCR_MAIN) drawMainScreen();
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
        if (currentPattern==0) { 
            if (currentMode==3) calculateResult(); 
            else Serial.println("[Braille] Pola kosong, abaikan."); 
            return; 
        }
        char c=decodeBraille(currentPattern, currentMode);
        if (c=='?') {
            Serial.println("[Braille] Pola tidak dikenal!");
            speak("Pola salah");
        } else {
            Serial.print("[Braille] Karakter: "); Serial.println(c);
            if (currentMode==0||currentMode==1) {
                lastChar = c;
                speak(String(c));
                if (currentScreen==SCR_MAIN) drawMainScreen();
            } else if (currentMode==2 || currentMode==3) {
                if (currentMode==3 && c == '=') {
                    calculateResult();
                } else {
                    currentWord+=c;
                    Serial.print("[Braille] Buffer: "); Serial.println(currentWord);
                    
                    if (c == '+') speak("tambah");
                    else if (c == '-') speak("kurang");
                    else if (c == '*') speak("kali");
                    else if (c == '/') speak("bagi");
                    else speak(String(c));
                    
                    if (currentScreen==SCR_MAIN) drawMainScreen(); // redraw full
                }
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
        } else if (currentMode==3) {
            currentWord="";
            speak("Reset Kalkulator");
            if (currentScreen==SCR_MAIN) drawMainScreen();
        } else {
            Serial.println("[Braille] Spasi hanya di mode Kata atau Kalkulator.");
        }
    }
    else if (pin==8) { // HAPUS
        if ((currentMode==2 || currentMode==3) && currentWord.length()>0) {
            currentWord.remove(currentWord.length()-1);
            Serial.print("[Braille] Hapus. Buffer: "); Serial.println(currentWord);
            speak("Hapus");
            if (currentScreen==SCR_MAIN) drawMainScreen();
        }
        currentPattern=0;
    }
    else if (pin == 9) { // GANTI MODE
        currentMode = (currentMode + 1) % 4;
        currentPattern = 0; currentWord = ""; lastChar = '-';
        const char* mn[] = {"Mode Huruf", "Mode Angka", "Mode Kata", "Mode Kalkulator"};
        speak(mn[currentMode]);
        if (currentScreen == SCR_MAIN) drawMainScreen();
    }
    // == TAMBAHAN TOMBOL FISIK KALKULATOR (PIN 10 - 13) ==
    else if (pin == 10) { appendMathOp('+'); } // Pin 10 = Tambah (+)
    else if (pin == 11) { appendMathOp('-'); } // Pin 11 = Kurang (-)
    else if (pin == 12) { appendMathOp('*'); } // Pin 12 = Kali (*)
    else if (pin == 13) { appendMathOp('/'); } // Pin 13 = Bagi (/)
}

void checkButtons() {
    uint32_t readState = mcp1.readGPIOAB();
    if (readState != lastFlickerState) { 
        lastDebounceTime = millis(); 
        lastFlickerState = readState; 
    }
    if ((millis() - lastDebounceTime) > 50) {
        if (readState != lastButtonState) {
            for (int i = 0; i < 14; i++) { // Ubah batas loop dari 10 menjadi 14 (mencakup pin 0-13)
                bool isP = !(readState & (1 << i));
                bool wasP = !(lastButtonState & (1 << i));
                if (isP && !wasP) handleButtonPress(i);
            }
            lastButtonState = readState;
        }
    }
}
