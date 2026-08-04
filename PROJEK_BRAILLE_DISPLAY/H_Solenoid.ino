// ============================================================
// SOLENOID BRAILLE PATTERN DISPLAY
// ============================================================

const uint8_t solenoidPins[6] = { 1, 3, 16, 17, 21, 22 };

// Spelling state variables
String spellingWord = "";
int spellingIndex = -1;
unsigned long nextSolenoidTime = 0;
bool spellingActive = false; // true: displaying letter, false: gap/clear
bool spellingPending = false;

void initSolenoids() {
    Serial.println("[Solenoid] Initializing pins...");
    for (int i = 0; i < 6; i++) {
        pinMode(solenoidPins[i], OUTPUT);
        digitalWrite(solenoidPins[i], LOW);
    }
}

void setSolenoids(uint8_t pattern) {
    // Actuate solenoids based on 6-bit pattern (bit 0 = dot 1, ..., bit 5 = dot 6)
    for (int i = 0; i < 6; i++) {
        digitalWrite(solenoidPins[i], (pattern >> i) & 1);
    }
}

uint8_t charToBraillePattern(char c) {
    c = tolower(c);
    switch(c) {
        case 'a': case '1': return 0b000001;
        case 'b': case '2': return 0b000011;
        case 'c': case '3': return 0b001001;
        case 'd': case '4': return 0b011001;
        case 'e': case '5': return 0b010001;
        case 'f': case '6': return 0b001011;
        case 'g': case '7': return 0b011011;
        case 'h': case '8': return 0b010011;
        case 'i': case '9': return 0b001010;
        case 'j': case '0': return 0b011010;
        case 'k': return 0b000101;
        case 'l': return 0b000111;
        case 'm': return 0b001101;
        case 'n': return 0b011101;
        case 'o': return 0b010101;
        case 'p': return 0b001111;
        case 'q': return 0b011111;
        case 'r': return 0b010111;
        case 's': return 0b001110;
        case 't': return 0b011110;
        case 'u': return 0b100101;
        case 'v': return 0b100111;
        case 'w': return 0b111010;
        case 'x': return 0b101101;
        case 'y': return 0b111101;
        case 'z': return 0b110101;
        default: return 0b000000;
    }
}

void startSolenoidSpelling(const String &word) {
    spellingWord = word;
    spellingIndex = -1;
    spellingPending = true;
    spellingActive = false;
    Serial.print("[Solenoid] Pending spelling for word: "); Serial.println(word);
}

void updateSolenoidSpelling() {
    // If pending, wait until audio actually starts playing
    if (spellingPending) {
        if (audio.isRunning()) {
            spellingPending = false;
            spellingIndex = 0;
            spellingActive = true;
            nextSolenoidTime = millis();
            Serial.println("[Solenoid] Audio started. Beginning spelling sequence.");
        } else {
            return;
        }
    }

    if (spellingIndex < 0 || spellingIndex >= (int)spellingWord.length()) {
        // No spelling active or completed
        return;
    }

    if (millis() >= nextSolenoidTime) {
        // Determine durations based on TTS speed
        uint32_t activeDuration = 600;
        uint32_t gapDuration = 200;
        if (ttsSpeedMode == 0) { // Normal
            activeDuration = 400;
            gapDuration = 150;
        } else if (ttsSpeedMode == 1) { // Lambat
            activeDuration = 750;
            gapDuration = 250;
        } else if (ttsSpeedMode == 2) { // Sangat Lambat
            activeDuration = 1400;
            gapDuration = 450;
        }

        if (spellingActive) {
            // Show the character
            char c = spellingWord.charAt(spellingIndex);
            uint8_t pattern = charToBraillePattern(c);
            setSolenoids(pattern);
            
            // Sync current pattern on UI
            currentPattern = pattern;
            if (currentScreen == SCR_MAIN) {
                drawBrailleDots();
                drawInputCardOnly();
            }

            Serial.print("[Solenoid] Spelling: '"); Serial.print(c); Serial.print("' -> ");
            for (int i=5; i>=0; i--) Serial.print((pattern>>i)&1);
            Serial.println();

            nextSolenoidTime = millis() + activeDuration;
            spellingActive = false; // Next change is to the gap
        } else {
            // Clear solenoids (gap between letters)
            setSolenoids(0);
            nextSolenoidTime = millis() + gapDuration;
            spellingActive = true; // Next change is showing the next character
            spellingIndex++;
            
            if (spellingIndex >= (int)spellingWord.length()) {
                // Completed
                spellingIndex = -1;
                Serial.println("[Solenoid] Spelling sequence completed.");
                currentPattern = 0;
                if (currentScreen == SCR_MAIN) {
                    drawBrailleDots();
                    drawInputCardOnly();
                }
            }
        }
    }
}
