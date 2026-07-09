#include "my_program.h"
#include "lcd.h"
#include "led.h"
#include "os_memheap_drivers.h"
#include "os_memory.h"
#include "os_process.h"
#include "os_scheduler.h"
#include "util.h"

#include <avr/pgmspace.h>
#include <stdbool.h>

#define BUFFER_SIZE 3

typedef struct {
    uint8_t data[BUFFER_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} CircularBuffer;

static MemAddr sharedBufferAddr = 0;

void producerProcess(void) {
    uint8_t item = 1;
    CircularBuffer *buf = (CircularBuffer *)sharedBufferAddr;
    
    while (1) {
        os_enterCriticalSection();
        bool full = (buf->count == BUFFER_SIZE);
        os_leaveCriticalSection();
        
        if (full) {
            // Buffer is full! Yield CPU and wait for consumer to consume
            os_yield();
            continue;
        }
        
        os_enterCriticalSection();
        buf->data[buf->head] = item;
        buf->head = (buf->head + 1) % BUFFER_SIZE;
        buf->count++;
        
        lcd_clear();
        lcd_writeProgString(PSTR("Prod: Wrote "));
        lcd_writeDec(item);
        lcd_line2();
        lcd_writeProgString(PSTR("Buf Count: "));
        lcd_writeDec(buf->count);
        os_leaveCriticalSection();
        
        item++;
        delayMs(1000);
    }
}

void consumerProcess(void) {
    CircularBuffer *buf = (CircularBuffer *)sharedBufferAddr;
    
    while (1) {
        os_enterCriticalSection();
        bool empty = (buf->count == 0);
        os_leaveCriticalSection();
        
        if (empty) {
            // Buffer is empty! Yield CPU and wait for producer to produce
            os_yield();
            continue;
        }
        
        os_enterCriticalSection();
        uint8_t item = buf->data[buf->tail];
        buf->tail = (buf->tail + 1) % BUFFER_SIZE;
        buf->count--;
        
        lcd_clear();
        lcd_writeProgString(PSTR("Cons: Read "));
        lcd_writeDec(item);
        lcd_line2();
        lcd_writeProgString(PSTR("Buf Count: "));
        lcd_writeDec(buf->count);
        os_leaveCriticalSection();
        
        delayMs(1500); // Consumer is slightly slower than Producer
    }
}

void startRingBufferDemo(void) {
    led_init();
    
    // Allocate the Ring Buffer in Shared Memory
    sharedBufferAddr = os_sh_malloc(intHeap, sizeof(CircularBuffer));
    if (sharedBufferAddr == 0) {
        os_error("SH Malloc failed");
        return;
    }
    
    // Initialize the Ring Buffer
    CircularBuffer *buf = (CircularBuffer *)sharedBufferAddr;
    buf->head = 0;
    buf->tail = 0;
    buf->count = 0;
    
    // Spawn Producer and Consumer
    os_exec(producerProcess, DEFAULT_PRIORITY);
    os_exec(consumerProcess, DEFAULT_PRIORITY);
    
    // The main coordinator task terminates
    os_kill(os_getCurrentProc());
}
