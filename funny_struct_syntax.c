#include <stdio.h>

typedef struct MemDriver {
    void (*init)(void);
} MemDriver;

void initSRAM(void) {
    printf("initialized!\n");
}

void initEEPROM(void) {
    printf(initialized!\n");
}

int main(void) {
    MemDriver sram_driver   = { .init = initSRAM };
    MemDriver eeprom_driver = { .init = initEEPROM };

    sram_driver.init();
    eeprom_driver.init();
    return 0;
}
