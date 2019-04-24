#include "Arduino.h"

extern boolean cfg_slog_on;

void log(String text) {
    if (cfg_slog_on) {
        Serial.begin(115200);
        Serial.println("Serial begin");
        Serial.println(text);
    }
}