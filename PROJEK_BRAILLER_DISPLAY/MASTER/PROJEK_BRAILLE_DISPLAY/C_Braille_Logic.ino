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

// ponytail: simple L-to-R calculator, no operator precedence
String evalCalc(const String &expr) {
    if (expr.length() == 0) return "0";
    float result = 0, current = 0;
    char op = '+';
    for (unsigned int i = 0; i <= expr.length(); i++) {
        char c = (i < expr.length()) ? expr.charAt(i) : '=';
        if (c >= '0' && c <= '9') { current = current * 10 + (c - '0'); }
        else {
            switch(op) {
                case '+': result += current; break;
                case '-': result -= current; break;
                case '*': result *= current; break;
                case '/': if(current!=0) result/=current; else return "Error"; break;
            }
            current = 0; op = c;
        }
    }
    if (result == (long)result) return String((long)result);
    return String(result, 2);
}

void handleButtonPress(int pin) {
    if (pin>=0 && pin<=5) {
        currentPattern^=(1<<pin);
        setSolenoids(currentPattern);
        Serial.print("[Braille] Titik "); Serial.print(pin+1);
        Serial.println((currentPattern&(1<<pin))?" ON":" OFF");
        Serial.print("[Braille] Pola: 0b");
        for (int i=5;i>=0;i--) Serial.print((currentPattern>>i)&1);
        Serial.println();
        
        if (currentScreen==SCR_MAIN) {
            drawBrailleDots();
            drawInputCardOnly();
        }
    }
    else if (pin==6) { // ENTER
        // ponytail: calc mode - empty pattern = evaluate
        if (currentMode==3 && currentPattern==0) {
            if (calcExpression.length()>0) {
                calcResult = evalCalc(calcExpression);
                Serial.print("[Calc] = "); Serial.println(calcResult);
                speak(calcResult, true);
                calcExpression = "";
                if (currentScreen==SCR_MAIN) drawMainScreen();
            }
            return;
        }
        if (currentPattern==0) { Serial.println("[Braille] Pola kosong, abaikan."); return; }
        char c=decodeBraille(currentPattern,(currentMode==1||currentMode==3));
        if (c=='?') {
            Serial.println("[Braille] Pola tidak dikenal!");
            speak("Pola salah");
        } else {
            Serial.print("[Braille] Karakter: "); Serial.println(c);
            if (currentMode==0||currentMode==1) {
                lastChar = c;
                speak(String(c), true);
                if (currentScreen==SCR_MAIN) drawMainScreen();
            } else if (currentMode==2) {
                currentWord+=c;
                Serial.print("[Braille] Kata: "); Serial.println(currentWord);
                speak(String(c), true);
                if (currentScreen==SCR_MAIN) drawMainScreen();
            } else if (currentMode==3) {
                calcExpression += c;
                Serial.print("[Calc] Expr: "); Serial.println(calcExpression);
                speak(String(c), true);
                if (currentScreen==SCR_MAIN) drawMainScreen();
            }
        }
        currentPattern=0;
    }
    else if (pin==7) { // SPASI
        if (currentMode==2 && currentWord.length()>0) {
            Serial.print("[Braille] Spasi. Ucapkan kata: "); Serial.println(currentWord);
            speak(currentWord, true);
            lastSpokenWord = currentWord;
            currentWord="";
            if (currentScreen==SCR_MAIN) drawMainScreen();
        } else if (currentMode==3 && calcExpression.length()>0) {
            // ponytail: SPASI = add/cycle operator
            const char ops[] = {'+','-','*','/'};
            const char* opNames[] = {"tambah","kurang","kali","bagi"};
            char last = calcExpression.charAt(calcExpression.length()-1);
            int idx = 0;
            bool isOp = (last=='+'||last=='-'||last=='*'||last=='/');
            if (isOp) {
                for(int i=0;i<4;i++) if(ops[i]==last){idx=(i+1)%4;break;}
                calcExpression.setCharAt(calcExpression.length()-1, ops[idx]);
            } else {
                calcExpression += ops[0]; idx=0;
            }
            char cur = calcExpression.charAt(calcExpression.length()-1);
            for(int i=0;i<4;i++) if(ops[i]==cur){idx=i;break;}
            speak(opNames[idx]);
            if (currentScreen==SCR_MAIN) drawInputCardOnly();
        } else {
            Serial.println("[Braille] Spasi hanya di mode Kata/Kalkulator.");
        }
    }
    else if (pin==8) { // HAPUS
        if (currentMode==2 && currentWord.length()>0) {
            currentWord.remove(currentWord.length()-1);
            Serial.print("[Braille] Hapus. Kata: "); Serial.println(currentWord);
            speak("Hapus");
            if (currentScreen==SCR_MAIN) drawMainScreen();
        } else if (currentMode==3 && calcExpression.length()>0) {
            calcExpression.remove(calcExpression.length()-1);
            Serial.print("[Calc] Hapus. Expr: "); Serial.println(calcExpression);
            speak("Hapus");
            if (currentScreen==SCR_MAIN) drawMainScreen();
        }
        currentPattern=0;
    }
    else if (pin==9) { // GANTI MODE
        currentMode=(currentMode+1)%4;
        currentPattern=0; currentWord=""; lastChar='-';
        calcExpression=""; calcResult="";
        const char* mn[]={"Mode Huruf","Mode Angka","Mode Kata","Mode Kalkulator"};
        Serial.print("[Braille] "); Serial.println(mn[currentMode]);
        speak(mn[currentMode]);
        if (currentScreen==SCR_MAIN) drawMainScreen();
    }
    // ponytail: operator buttons via MCP extender (pin 10=+, 11=-, 12=*, 13=/)
    else if (pin>=10 && pin<=13) {
        const char ops[] = {'+','-','*','/'};
        const char* opNames[] = {"tambah","kurang","kali","bagi"};
        int idx = pin - 10;
        if (currentMode==3) {
            // If last char is already an operator, replace it
            if (calcExpression.length()>0) {
                char last = calcExpression.charAt(calcExpression.length()-1);
                if (last=='+'||last=='-'||last=='*'||last=='/') {
                    calcExpression.setCharAt(calcExpression.length()-1, ops[idx]);
                } else {
                    calcExpression += ops[idx];
                }
            } else {
                calcExpression += ops[idx]; // allow starting with operator (edge case)
            }
            Serial.print("[Calc] Op: "); Serial.print(ops[idx]);
            Serial.print(" Expr: "); Serial.println(calcExpression);
            speak(opNames[idx]);
            if (currentScreen==SCR_MAIN) drawInputCardOnly();
        } else {
            Serial.println("[Braille] Tombol operator hanya di mode Kalkulator.");
        }
    }
}

void checkButtons() {
    uint16_t readState=mcp1.readGPIOAB();
    if (readState!=lastFlickerState) { lastDebounceTime=millis(); lastFlickerState=readState; }
    if ((millis()-lastDebounceTime)>50) {
        if (readState!=lastButtonState) {
            for (int i=0;i<14;i++) { // ponytail: 0-9 braille+ctrl, 10-13 calc operators
                bool isP=!(readState&(1<<i));
                bool wasP=!(lastButtonState&(1<<i));
                if (isP&&!wasP) handleButtonPress(i);
            }
            lastButtonState=readState;
        }
    }
}
