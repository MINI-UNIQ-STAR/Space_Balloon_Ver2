#include <stdint.h>
#include <stdbool.h>
#include "defs.h" 

// Mock replacing owlink.c matching function signatures

char owTouchReset(void) {
    // Return TRUE (1) to indicate presence
    return 1;
}

char owReadBit(void) {
    return 1;
}

void owWriteBit(char bit) {
    // Mock
}

void owWriteByte(unsigned char data) {
    // Mock
}

unsigned char owReadByte(void) {
    // Mock
    // To allow ds18b20_getTemp to return something valid, we might want 
    // to return a specific sequence if we tracked state.
    // But for simple compilation/run, returning 0 is fine (Temp 0).
    // Returning 0x00 0x01 = 16 * 0.0625 = 1.0C
    // Let's return a static byte that toggles or just 0.
    return 0;
}
