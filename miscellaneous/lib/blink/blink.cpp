#include <blink.h>

void turnLEDon(int port) {
    digitalWrite(port, HIGH);
}

void turnLEDoff(int port) {
    digitalWrite(port, LOW);
}