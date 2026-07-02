#include "queue.h"

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

// Initialisiert die Queue
void queue_init(DynamicQueue *queue, Heap *heap, uint16_t elementSize, uint16_t initialCapacity) {
    queue->heap = heap;
    queue->elementSize = elementSize;
    queue->capacity = initialCapacity;
    queue->size = 0;
    queue->dataAddr = os_malloc(heap, initialCapacity * elementSize);
}

// Element am Ende hinzufügen (FIFO: In am Ende)
bool queue_enqueue(DynamicQueue *queue, const void *element) {
    // Wenn voll, verdoppeln wir die Kapazität
    if (queue->size >= queue->capacity) {
        uint16_t newCapacity = queue->capacity * 2;
        MemAddr newAddr = os_realloc(queue->heap, queue->dataAddr, newCapacity * queue->elementSize);
        if (newAddr == 0) {
            return false; // Out of memory
        }
        queue->dataAddr = newAddr;
        queue->capacity = newCapacity;
    }

    // Offset berechnen: startAdresse + (index * elementGroesse)
    MemAddr dest = queue->dataAddr + (queue->size * queue->elementSize);
    
    // Element ans Ende schreiben
    writeBlock(queue->heap, dest, element, queue->elementSize);
    
    queue->size++;
    return true;
}

// Element von vorne holen und entfernen (FIFO: Out am Anfang)
bool queue_dequeue(DynamicQueue *queue, void *out_element) {
    if (queue->size == 0) {
        // Underflow: Queue ist leer
        return false; 
    }

    // 1. Das vorderste Element (Index 0) auslesen
    readBlock(queue->heap, out_element, queue->dataAddr, queue->elementSize);

    // 2. Alle restlichen Elemente um 1 Position nach vorne (links) verschieben
    if (queue->size > 1) {
        uint16_t bytesToShift = (queue->size - 1) * queue->elementSize;
        for (uint16_t i = 0; i < bytesToShift; i++) {
            MemAddr src = queue->dataAddr + queue->elementSize + i;
            MemAddr dest = queue->dataAddr + i;
            queue->heap->driver->write(dest, queue->heap->driver->read(src));
        }
    }

    queue->size--;

    // 3. Optional: Speicher freigeben, wenn weniger als 25% Auslastung vorliegt
    if (queue->capacity > 4 && queue->size <= queue->capacity / 4) {
        uint16_t newCapacity = queue->capacity / 2;
        MemAddr newAddr = os_realloc(queue->heap, queue->dataAddr, newCapacity * queue->elementSize);
        if (newAddr != 0) {
            queue->dataAddr = newAddr;
            queue->capacity = newCapacity;
        }
    }

    return true;
}

// Queue freigeben
void queue_free(DynamicQueue *queue) {
    if (queue->dataAddr != 0) {
        os_free(queue->heap, queue->dataAddr);
        queue->dataAddr = 0;
    }
    queue->size = 0;
    queue->capacity = 0;
}
