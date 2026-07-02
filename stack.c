#include "stack.h"


static void writeBlock(Heap *heap, MemAddr dest, const void *src, uint16_t size) {
    const uint8_t *srcBytes = (const uint8_t *)src;
    for (uint16_t i = 0; i < size; i++) {
        heap->driver->write(dest + i, srcBytes[i]);
    }
}

static void* readBlock(Heap *heap, void *dest, MemAddr src, uint16_t size) {
    uint8_t *destBytes = (uint8_t *)dest;
    for (uint16_t i = 0; i < size; i++) {
        destBytes[i] = heap->driver->read(src + i);
    }
    return dest;
}

// Initialisiert den Stack
void stack_init(DynamicStack *stack, Heap *heap, uint16_t elementSize, uint16_t initialCapacity) {
    stack->heap = heap;
    stack->elementSize = elementSize;
    stack->capacity = initialCapacity;
    stack->size = 0;
    stack->dataAddr = os_malloc(heap, initialCapacity * elementSize);
}

// Element auf den Stack legen (LIFO)
bool stack_push(DynamicStack *stack, const void *element) {
    // Wenn voll, verdoppeln wir die Kapazität
    if (stack->size >= stack->capacity) {
        uint16_t newCapacity = stack->capacity * 2;
        MemAddr newAddr = os_realloc(stack->heap, stack->dataAddr, newCapacity * stack->elementSize);
        if (newAddr == 0) {
            return false; // Out of memory (Allokationsfehler)
        }
        stack->dataAddr = newAddr;
        stack->capacity = newCapacity;
    }

    // Offset berechnen: startAdresse + (index * elementGroesse)
    MemAddr dest = stack->dataAddr + (stack->size * stack->elementSize);
    
    // Element kopieren
    writeBlock(stack->heap, dest, element, stack->elementSize);
    
    stack->size++;
    return true;
}

// Element vom Stack holen (LIFO)
bool stack_pop(DynamicStack *stack, void *out_element) {
    if (stack->size == 0) {
        return false; // Underflow (Stack ist leer)
    }

    stack->size--;

    // Offset berechnen: startAdresse + (index * elementGroesse)
    MemAddr src = stack->dataAddr + (stack->size * stack->elementSize);
    
    // Element kopieren
    readBlock(stack->heap, out_element, src, stack->elementSize);

    // Optional: Speicher freigeben, wenn weniger als 25% Auslastung vorliegt
    // (und die Kapazität nicht unter ein vernünftiges Minimum von 4 sinkt)
    if (stack->capacity > 4 && stack->size <= stack->capacity / 4) {
        uint16_t newCapacity = stack->capacity / 2;
        MemAddr newAddr = os_realloc(stack->heap, stack->dataAddr, newCapacity * stack->elementSize);
        if (newAddr != 0) {
            stack->dataAddr = newAddr;
            stack->capacity = newCapacity;
        }
    }

    return true;
}

// Stack freigeben
void stack_free(DynamicStack *stack) {
    if (stack->dataAddr != 0) {
        os_free(stack->heap, stack->dataAddr);
        stack->dataAddr = 0;
    }
    stack->size = 0;
    stack->capacity = 0;
}
