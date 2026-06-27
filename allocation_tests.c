
#include "my_program.h"
#include "lcd.h"
#include "led.h"
#include "os_process.h"
#include "os_scheduler.h"
#include "os_memory.h"
#include "os_memheap_drivers.h"
#include "util.h"

#include <avr/pgmspace.h>
#include <stdbool.h>

void myProgram2(void) {
  uint16_t counter = 0;

  led_init();

  while (1) {
    os_enterCriticalSection();
    for (uint8_t i = 0; i < 5; i++) {
      ProcessID pid = os_getCurrentProc();
      Process *proc = os_getProcessSlot(pid);

      lcd_clear();
      lcd_writeProgString(PSTR("PID: "));
      lcd_writeDec(pid);
      lcd_line2();
      lcd_writeProgString(PSTR("C:"));
      lcd_writeDec(counter);
      lcd_writeProgString(PSTR(" P:"));
      lcd_writeDec(proc->priority);

      led_setBar(counter & 0xFF);

      counter++;
      delayMs(200);
    }
    delayMs(2000);
    os_leaveCriticalSection();
  }
}

void myProgram(void) {
  uint16_t counter = 0;

  led_init();

  while (1) {
    os_enterCriticalSection();
    for (uint8_t i = 0; i < 5; i++) {
      ProcessID pid = os_getCurrentProc();
      Process *proc = os_getProcessSlot(pid);

      lcd_clear();
      lcd_writeProgString(PSTR("PID: "));
      lcd_writeDec(pid);
      lcd_line2();
      lcd_writeProgString(PSTR("C:"));
      lcd_writeDec(counter);
      lcd_writeProgString(PSTR(" P:"));
      lcd_writeDec(proc->priority);

      led_setBar((uint16_t)1 << (counter % 16));

      counter++;
      delayMs(200);
    }
    delayMs(2002);
    os_leaveCriticalSection();
  }
}


static void displayStatus(const char *line1, const char *line2) {
    os_enterCriticalSection();
    lcd_clear();
    lcd_writeProgString(line1);
    if (line2) {
        lcd_line2();
        lcd_writeProgString(line2);
    }
    os_leaveCriticalSection();
    delayMs(1500);
}

typedef struct {
	uint8_t id;
	uint16_t score;
	char grade;
} Student;


void writeBlock(Heap *heap, MemAddr dest, const void *src, uint16_t size) {
	const uint8_t *srcBytes = (const uint8_t *)src;
	for (uint16_t i = 0; i < size; i++) {
		heap->driver->write(dest + i, srcBytes[i]);
	}
}
void* readBlock(Heap *heap, void *dest, MemAddr src, uint16_t size) {
	uint8_t *destBytes = (uint8_t *)dest;
	for (uint16_t i = 0; i < size; i++) {
		destBytes[i] = heap->driver->read(src + i);
	}
	return dest;
}



void myProgram3(void) {
    led_init();
    
    displayStatus(PSTR("Versuch 3 Demo"), PSTR("Starting now..."));
    
    while (1) {
        AllocStrategy strategy = OS_MEM_FIRST;
        os_setAllocationStrategy(intHeap, strategy);
		
		Student localStudent;
		localStudent.id = 42;
		localStudent.score = 950;
		localStudent.grade = 'A';
		
        
		MemAddr addr = os_malloc(intHeap, sizeof(Student));
		writeBlock(intHeap, addr, &localStudent, sizeof(Student));
		Student *internal_fetch = (Student *)readBlock(intHeap, &localStudent, addr, sizeof(Student));
		lcd_writeDec(internal_fetch->score);
		
		delayMs(1500);
		lcd_clear();
		
		MemAddr addr_ext = os_malloc(extHeap, sizeof(Student));
		writeBlock(extHeap, addr_ext, &localStudent, sizeof(Student));
		Student *fetched_s = (Student *)readBlock(extHeap, &localStudent, addr_ext, sizeof(Student));
		lcd_writeDec(fetched_s->id);
		
		delayMs(1500);
		lcd_clear();
		
		MemAddr newAddr = os_realloc(extHeap, addr_ext, sizeof(Student) * 4);
		(void)newAddr;
		writeBlock(extHeap, newAddr, &localStudent, sizeof(Student));
		Student *relocated_fetch_s = (Student *)readBlock(extHeap, &localStudent, newAddr, sizeof(Student));
		lcd_writeDec(relocated_fetch_s->score);
		
		os_free(extHeap, newAddr);
		//os_free(extHeap, newAddr);
        
        delayMs(1500);
        os_enterCriticalSection();
        lcd_clear();
        lcd_writeProgString(PSTR("Allocated at:"));
        lcd_line2();
        lcd_writeProgString(PSTR("Addr: 0x"));
        lcd_writeHexWord(addr);
        os_leaveCriticalSection();
        delayMs(1500);
        
        os_free(intHeap, addr);
        displayStatus(PSTR("Freed block"), PSTR("Map is clean"));
        
        delayMs(2000);
    }
}
