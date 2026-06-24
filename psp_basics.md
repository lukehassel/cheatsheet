# Pointer Cheatsheet (SPOS project) — for a beginner

> A pointer is **just a variable that holds a memory address** instead of a normal value.
> In this project, memory addresses are usually a `MemAddr` (a `uint16_t` = a number 0–65535). A pointer is the same number, but tagged "this is the address of a `Point`" so the compiler lets you read/write fields through it.

There are only **5 symbols** to learn. Memorize these and you can read everything here:

| Symbol | Name | One-line meaning |
|--------|------|------------------|
| `&x` | address-of | "where does `x` live in memory?" → gives an address |
| `*p` | dereference | "give me the value stored at the address `p` holds" |
| `*` after a type | pointer type | `Point *` = "a pointer to a `Point`" |
| `p->field` | arrow | read/write a struct field through a pointer (short for `(*p).field`) |
| `(Type *)val` | cast | "treat this number/pointer as if it were a `Type *`" |

Bonus: `arr[i]` is pointer math in disguise — `arr[i]` means `*(arr + i)`.

---

## 1. `&` — address-of ("give me the address")

From `os_memheap_drivers.h`:

```c
extern Heap intHeap__;          // a real struct, living in memory
#define intHeap (&intHeap__)    // intHeap  =  "the ADDRESS OF intHeap__"
```

So whenever you write `intHeap` in code, you're not passing the whole struct — you're passing its **address**. `intHeap` is a `Heap *` (pointer), `intHeap__` is a `Heap` (the real thing).

- `x` = the value
- `&x` = where `x` lives

---

## 2. `*` — two completely different jobs

### 2a. After a type → "this is a pointer type"

```c
Point *p;          // p holds an address, and at that address lives a Point
volatile uint8_t *ptr;   // ptr holds an address, and at that address lives a byte
const MemDriver *driver; // driver holds an address of a MemDriver (read from os_memheap_drivers.h)
```

Read it right-to-left: "`*p` is a `Point`", so `p` is a `Point *`.

### 2b. In front of a variable → dereference ("read the value AT that address")

From `os_mem_drivers.c`:

```c
volatile uint8_t *ptr = (volatile uint8_t *)addr;  // ptr now holds 'addr'
return *ptr;   // read the byte sitting at that address
```

- `ptr` = the address (a number)
- `*ptr` = the byte that lives there

It also works for writing: `*ptr = 5;` stores 5 at that address.

---

## 3. `->` — struct field through a pointer ("arrow")

When you have a **pointer to a struct**, use `->` to reach a field. From `my_program1.c`:

```c
Point *p = (Point *)os_malloc(heap, sizeof(Point));
p->x = 10;     // writes the 'x' field of the Point that p points at
p->y = 20;     // writes the 'y' field
```

**Rule of thumb:**

- real struct value → use a dot: `proc.priority`
- pointer to a struct → use an arrow: `proc->priority`

`p->x` is literally shorthand for `(*p).x`. The arrow exists only to save you writing the ugly `(*p)` every time.

### Chained arrows

From everywhere in the OS code:

```c
heap->driver->read(addr)
```

Read it left to right:

- `heap->driver` — get the `driver` field (which is itself a pointer) of the struct `heap` points to
- `->read(addr)` — call the `read` function that `driver` points to

---

## 4. `(Type *)` — cast ("pretend this is a `Type *`")

This comes up constantly in this project because memory is returned as a plain integer (`MemAddr`) and you want to use it as a struct.

From `my_program1.c`:

```c
// os_malloc returns a MemAddr (a number). Cast it to "pointer to Point".
Point *p = (Point *)os_malloc(heap, sizeof(Point));
```

What it does:

- `os_malloc(heap, sizeof(Point))` → returns an address like `0x1100`, as a `uint16_t`
- `(Point *)` → tells the compiler "treat `0x1100` as the address of a `Point`"
- now `p->x` works, because the compiler knows a `Point` starts at `0x1100`

It costs **nothing at runtime** — the bits don't change, only how the compiler thinks about them.

### The reverse cast: pointer → integer

```c
Point *arr = ...;
os_free(heap, (MemAddr)arr);   // os_free wants a MemAddr, so cast the pointer back to a number
```

Same idea, opposite direction. The round trip `pointer ↔ integer address` is free.

### `volatile` cast

From `os_mem_drivers.c`:

```c
volatile uint8_t *ptr = (volatile uint8_t *)addr;
```

`volatile` means "compiler, don't optimize reads/writes away — every time I touch this address, do it for real." Used for memory-mapped hardware. The cast just says "treat `addr` as a pointer to a volatile byte."

---

## 5. `[]` — indexing is pointer math

```c
Point *arr = (Point *)os_realloc(heap, (MemAddr)p, sizeof(Point) * NUM_POINTS);
arr[1].x = 11;   // second Point's  x field
arr[3].y = 23;   // fourth Point's y field
```

`arr[1]` secretly means `*(arr + 1)`, i.e. "start at `arr`, step forward by 1 element, read what's there." The size of one element (`sizeof(Point)`) is added automatically — that's why the compiler needs to know `arr` is a `Point *` and not a plain number.

So `arr[i].field` = "the `field` of the i-th Point in the row starting at `arr`."

---

## 6. `NULL` — "points at nothing"

From `defines.h`:

```c
#define NULL ((void *)0)
```

`NULL` is address 0, the universal "this pointer doesn't point anywhere" value. You'll see it used as:

- a failure return: `if (!p)` or `if (p == NULL)` → "malloc failed, nothing was allocated"
- a sentinel: `os_lookupHeap(2)` returns `NULL` for an invalid index

`!p` (the bang) is true when `p` is `NULL` (address 0). So `if (!p)` reads as "if p doesn't point at anything."

---

## 7. Function pointers — a pointer that points to *code*

The trickiest one. From `os_mem_drivers.h`:

```c
typedef struct MemDriver {
  void (*init)(void);
  MemValue (*read)(MemAddr addr);
  void (*write)(MemAddr addr, MemValue value);
  ...
} MemDriver;
```

These are **pointers to functions**, stored inside a struct.

- `void (*init)(void);` reads as: "`init` is a pointer to a function that takes no arguments and returns `void`."
- You can't call the function directly — you call it *through the pointer*.

How they're used (`os_mem_drivers.c`):

```c
const MemDriver intSRAM__ = {
    .init = initSRAM,    // store the ADDRESS of the initSRAM function
    .read = readSRAM,    // store the ADDRESS of the readSRAM function
    .write = writeSRAM,
    ...
};
```

A function name **without parentheses** gives you its address (like `&` for functions, but the `&` is optional).

Then calling through the pointer:

```c
intSRAM->init();                 // calls initSRAM()
intSRAM->read(0x1100);           // calls readSRAM(0x1100)
heap->driver->write(addr, 5);    // heap -> driver -> the write function -> call it
```

Why? So the same OS code works for **both** heaps: `intSRAM` (real RAM) and `extSRAM` (external SPI RAM). The struct points at different functions depending on which driver — the heap code never changes.

---

## 8. "Out parameter" pattern — writing through a passed-in pointer

A function can't return two things. Trick: pass a pointer to where the answer should go. From `my_program.c` (the stack demo):

```c
static bool stackPop(Heap *heap, MemAddr header, MemValue *out) {
    ...
    if (out) *out = heap->driver->read(data + (len - 1) * ELEM_SIZE);
    ...
}
```

- `MemValue *out` → "give me the address of a `MemValue`, I'll put the answer there"
- `*out = ...` → "write the value into that address"
- Caller side:

```c
MemValue v;
stackPop(heap, header, &v);   // pass the ADDRESS of v; the function fills v
// now v holds the popped value
```

`&v` (address-of) hands the address over; `*out` (dereference) writes through it. Same two operators, now working as a pair.

---

## Quick reference — reading real lines

```c
// my_program1.c
Point *p = (Point *)os_malloc(heap, sizeof(Point));
//  ^^^^^ pointer type   ^^^^^^ cast: integer-address -> Point*
p->x = 10;
//  ^^ arrow: write the x field through the pointer
Point *arr = (Point *)os_realloc(heap, (MemAddr)p, sizeof(Point) * NUM_POINTS);
//                                    ^^^^^^^^^^  reverse cast: pointer -> integer address
arr[3].x = 13;
//  ^^^ index into the Point array, then dot (because arr[3] is a real Point, not a pointer)
os_free(heap, (MemAddr)arr);
//            ^^^^^^^^^^ pointer -> integer address again

// os_mem_drivers.c
volatile uint8_t *ptr = (volatile uint8_t *)addr;
return *ptr;                              // dereference: read byte at that address

// os_memheap_drivers.h
const MemDriver *driver;                  // pointer to a (const) MemDriver
extern Heap intHeap__;
#define intHeap (&intHeap__)              // address-of: intHeap is the address of the struct

// function pointers (os_mem_drivers.h / .c)
void (*write)(MemAddr addr, MemValue value);   // write IS a pointer to such a function
.write = writeSRAM,                             // store function's address
heap->driver->write(addr, 5);                  // chain: heap -> driver -> write function -> call
```

---

## The mental model in one paragraph

A pointer is an address (a number). `&` turns a value into its address. `*` turns an address back into the value (to read or write it). When the value at the address is a struct, `->` is the shortcut for "go to the address, then grab that field." A cast `(Type *)` is how you tell the compiler what kind of thing lives at a raw integer address — it changes nothing about the bits, only the compiler's assumption. Function pointers are the same idea, but the address points at code instead of data, and adding `()` after the pointer runs that code.

---

# Part 2 — How this project defines structs

This codebase has a **very consistent** struct style. Learn the four patterns below and you'll recognise every struct in the OS.

## 9. `typedef struct { ... } Name;` — the standard project pattern

Almost everything is defined with `typedef` so you can use the bare name (`Point`, `Heap`, `Process`) instead of writing `struct Point` every time.

From `my_program1.c`:

```c
typedef struct {
    uint8_t x;
    uint8_t y;
} Point;
```

- `struct { ... }` → the anonymous struct layout
- `typedef ... Point` → "give this layout the short name `Point`"
- After this, write `Point p;` (no `struct` keyword needed)

The bigger real one, from `os_memheap_drivers.h`:

```c
typedef struct Heap {
  const MemDriver *driver;   // pointer to a driver struct
  MemAddr mapStart;
  MemAddr useStart;
  uint16_t mapSize;
  uint16_t useSize;
  AllocStrategy allocStrat;
  const char *name;
  uint16_t allocCount[MAX_NUMBER_OF_PROCESSES];  // an array inside the struct
} Heap;
```

Note the struct is **tagged** (`struct Heap`) **and** typedef'd — both `struct Heap` and `Heap` work. The fields mix simple integers, a pointer (`*driver`, `*name`), an enum (`allocStrat`), and an inline array (`allocCount`). That's normal: a struct is just a bundle of whatever fields you need.

## 10. `typedef enum { ... } Name;` — an enum bundled with a type name

From `os_process.h`:

```c
typedef enum ProcessState {
    OS_PS_UNUSED,
    OS_PS_READY,
    OS_PS_RUNNING,
    OS_PS_BLOCKED
} ProcessState;
```

An enum is a list of named integer constants (`OS_PS_UNUSED = 0`, `OS_PS_READY = 1`, …). `typedef` gives it the short name `ProcessState` so you can write `ProcessState state;` as a struct field. Same tag-+-typedef habit as the structs.

There's also the heap-strategy one in `os_memheap_drivers.h`:

```c
typedef enum AllocStrategy {
  OS_MEM_FIRST, OS_MEM_NEXT, OS_MEM_BEST, OS_MEM_WORST
} AllocStrategy;
```

## 11. Designated initializers — `.{field} = value`

This is how every global struct in the project gets filled in. From `os_memheap_drivers.c`:

```c
Heap intHeap__ = {
    .driver     = intSRAM,
    .mapStart   = AVR_SRAM_START + HEAPOFFSET,
    .mapSize    = MAP_SIZE,
    .useStart   = AVR_SRAM_START + HEAPOFFSET + MAP_SIZE,
    .useSize    = USE_SIZE,
    .allocStrat = OS_MEM_FIRST,
    .name       = "intHeap"
};
```

- `.driver = intSRAM` → "set the `driver` field to `intSRAM`"
- Order doesn't matter — you name fields explicitly, so you can't accidentally shift them.
- Fields you don't list are zero-initialized.
- `intSRAM` here is itself a macro that expands to `(&extSRAM__)` — a pointer (see section 1), so `driver` (a `const MemDriver *`) is being set to an address. Same for the driver struct itself:

```c
const MemDriver intSRAM__ = {
    .init  = initSRAM,    // function name without () = its address (function pointer!)
    .read  = readSRAM,
    .write = writeSRAM,
    .start = AVR_SRAM_START,
    .size  = AVR_MEMORY_SRAM
};
```

The `.init = initSRAM` lines are where **function pointers meet struct initialization** — `initSRAM` (no parens) is the address of the function, stored into the function-pointer field `init`.

## 12. The "real struct + pointer macro + extern" trio

This three-line pattern repeats for every global struct the OS exports (`intHeap`, `extHeap`, `intSRAM`, `extSRAM`):

```c
// .h file: declare the struct, declare a real instance, expose a pointer macro
extern Heap intHeap__;           // the real struct lives here (defined in .c)
#define intHeap (&intHeap__)     // short pointer alias

// .c file: actually define + initialize it
Heap intHeap__ = { .driver = intSRAM, ... };
```

So everywhere else, code writes `intHeap` and gets a `Heap *` — a pointer to the real struct — for free, without `&` at every call site. The ugly `intHeap__` double-underscore name is the real variable; the clean `intHeap` is just `(&intHeap__)`. This is why `os_malloc(intHeap, ...)` works: `intHeap` is already a `Heap *`.

---

# Part 3 — Advanced / complex syntax used in this project

These appear less often but are real and load-bearing. Each is explained with the actual line it comes from.

## 13. `union` — one piece of memory, read as different types

From `os_process.h`:

```c
typedef union StackPointer {
    uint16_t as_int;   // same bytes, interpreted as a 16-bit integer
    uint8_t *as_ptr;   // same bytes, interpreted as a pointer
} StackPointer;
```

A `union` overlays its members on the **same memory** — all fields start at offset 0. You write `as_int`, read `as_ptr`, and you're looking at the same bits two ways. The comment says it directly: "We use a union so we can reduce the number of explicit casts." This is the textbook use case: a stack pointer is both a number (to push bytes onto) and a pointer (to dereference) — same 16 bits, two interpretations — so a union replaces a cast.

### Choosing a struct *or* a union

- `struct` → fields laid out one after another, each at its own offset (a **record**).
- `union` → fields all at offset 0, sharing memory (an **interpretation switch**).

## 14. Pointer-to-pointer `**sp` and `(*sp)--`

From `os_scheduler.c`:

```c
static void os_write16BitToStack(uint8_t **sp, uint16_t word) {
    **sp = word & 0xFF;     // write low byte to where *sp points
    (*sp)--;                // move the pointer itself back one byte
    **sp = (word >> 8) & 0xFF;  // write high byte to the new location
    ...
}
```

`uint8_t **sp` is a **pointer to a pointer** — `sp` holds the address of *another* `uint8_t *`. This is the "out parameter applied to a pointer itself" pattern (extension of section 8):

- `*sp` → the pointer we're being asked to adjust (the process's stack pointer)
- `**sp` → the byte that pointer currently points at (dereferencing twice)
- `(*sp)--` → decrement *the pointer itself*, not what it points at. The parens matter: `*sp--` would dereference first then move the *outer* pointer, which is wrong. `(*sp)--` reads as "decrement the thing `sp` points at."

Read the levels from right to left: `uint8_t **sp` = "pointer to pointer to byte."

## 15. Bit manipulation — `<<`, `>>`, `&`, `~`, `|`

AVR code loves bits; the whole memory map is nibble-packed. From `os_mem_drivers.c`:

```c
os_spi_send((addr >> 8) & 0xFF);   // high byte of a 16-bit address
os_spi_send(addr & 0xFF);          // low byte
```

- `addr >> 8` → shift `addr` right by 8 bits (drops the low byte, brings the high byte down)
- `& 0xFF` → mask: keep only the lowest 8 bits (zero out everything above)
- `<< 8` (in `heapReadW`) → shift left to put a byte in the high position before OR-ing

The build-or-clear macros from `util.h`:

```c
#define sbi(x, b) x |= (1 << (b))   // set bit b (set)
#define cbi(x, b) x &= ~(1 << (b))  // clear bit b (clear)
#define gbi(x, b) (((x) >> (b)) & 1) // get bit b (read)
```

- `1 << b` → a 1 shifted to bit position `b` (a single-bit mask)
- `x |= mask` → set those bits (OR)
- `~mask` → invert the mask; `x &= ~mask` → clear those bits (AND with the complement)
- `(x >> b) & 1` → pull bit `b` down to position 0 and isolate it → 0 or 1

Used directly in `os_scheduler.c`: `TIMSK2 |= (1 << OCIE2A);` → set the OCIE2A bit in the timer-interrupt mask register. This is the bread and butter of talking to AVR hardware registers.

### Nibble packing (advanced bit math)

From `os_memory.c`, the heap stores **two** map entries per byte (high nibble + low nibble):

```c
static MemValue getHighNibble(const Heap *heap, MemAddr addr) {
  return (heap->driver->read(addr) >> 4) & 0x0F;   // top 4 bits
}
static MemValue getLowNibble(const Heap *heap, MemAddr addr) {
  return heap->driver->read(addr) & 0x0F;          // bottom 4 bits
}
static void setHighNibble(const Heap *heap, MemAddr addr, MemValue value) {
  MemValue byte = heap->driver->read(addr);
  byte = (byte & 0xF0) | ((value & 0x0F) << 4);    // clear top, place new top
  heap->driver->write(addr, byte);
}
```

`(byte & 0xF0)` keeps the high nibble, zeroes the low; `((value & 0x0F) << 4)` shifts the new value up into the high nibble; `|` combines them. This is the same set/clear pattern from `sbi`/`cbi`, just with 4-bit fields instead of 1-bit.

## 16. Function pointers (recap, with the full chain)

Already in section 7, but the key reminder: a function name **without `()`** is its address.

```c
typedef struct MemDriver {
  void (*init)(void);                       // field "init": pointer to a void(void) function
  MemValue (*read)(MemAddr addr);
  void (*write)(MemAddr addr, MemValue value);
} MemDriver;

const MemDriver intSRAM__ = { .init = initSRAM, .read = readSRAM, ... };

intSRAM->init();            // call through the pointer
heap->driver->write(a, v); // chained: heap -> driver field -> write field -> call
```

The whole OS is polymorphic through this: `heap->driver->read(addr)` calls a different concrete function for `intHeap` vs `extHeap` — same source line, different behaviour.

## 17. `ISR(...)` and `__attribute__((naked))` — interrupt + raw functions

From `os_scheduler.c`:

```c
ISR(TIMER2_COMPA_vect)
__attribute__((naked));
```

- `ISR(TIMER2_COMPA_vect)` is an AVR-libc macro that defines an **Interrupt Service Routine** — a function the hardware jumps to when Timer 2 Compare Match A fires. It's not called by your code; the MCU calls it.
- `__attribute__((naked))` is a GCC extension meaning "don't emit the normal function prologue/epilogue" — the compiler won't save/restore registers for you. This is used together with `saveContext()`/`restoreContext()` (the giant asm blocks in `util.h`) so a context switch can hand control to another process manually. You will not write this yourself, but you'll see it in `os_scheduler.c`.

## 18. `REGISTER_AUTOSTART` macro — function pointers + linked list + constructor

The most advanced pattern in the whole project, from `os_process.h`:

```c
#define REGISTER_AUTOSTART(PROGRAM_FUNCTION)                                         \
    Program PROGRAM_FUNCTION;                                                        \
    void __attribute__((constructor)) register_autostart_##PROGRAM_FUNCTION(void) {  \
        static struct program_linked_list_node node = {.program = PROGRAM_FUNCTION}; \
        node.next = autostart_head;                                                  \
        autostart_head = &node;                                                      \
    }
```

Used as `REGISTER_AUTOSTART(myProgram1);` and then `myProgram1` is automatically run at boot. A lot of syntax in one macro — broken down:

- The `\` at line ends continues the macro across lines.
- `Program PROGRAM_FUNCTION;` → `Program` is `typedef void Program(void);` (a function **type**, from `os_process.h`) so this is a **forward declaration** of your program function.
- `__attribute__((constructor))` → GCC runs this function **automatically before `main()`** (a "constructor"). So the registration happens at startup without anyone calling it.
- `register_autostart_##PROGRAM_FUNCTION` → the `##` **token-pastes** the function name into a unique symbol (`register_autostart_myProgram1`), so multiple programs can each register without clashing.
- `static struct program_linked_list_node node = {.program = PROGRAM_FUNCTION};` → a designated-initializer (section 11) for one node, holding the function pointer (section 16). It's `static` so it persists.
- `node.next = autostart_head; autostart_head = &node;` → classic **linked-list prepend** (head insertion): point the new node at the old head, then make the new node the head.

It combines: a function type, a function pointer, a tagged struct (`struct program_linked_list_node` with a self-referential `*next` pointer — a recursive type), designated init, token pasting, a GCC constructor attribute, and a pointer-based linked list. If you can read this macro, you've understood everything above.

## 19. Linked-list self-referential struct — `struct foo *next`

The node type behind the macro above, from `os_process.h`:

```c
struct program_linked_list_node {
    Program *program;
    struct program_linked_list_node *next;   // points to another node of the SAME type
};
```

A struct that contains a pointer to its own type — how you build linked lists, trees, queues. Note it's a **tagged struct without a typedef** (`struct program_linked_list_node`), which is why you see that long name repeated. The `*next` either points to the next node or is `NULL` (end of list). This is section 1 (`&`) + section 7 (pointers) + section 9 (structs) combined into one recursive structure.

---

## Big-picture summary (Part 3 → Part 1 mapping)

| Advanced pattern | Which basics it combines |
|-------------------|--------------------------|
| Designated initializers (§11) | structs (§9) + function pointers (§7) + `&`/macros (§12) |
| `union StackPointer` (§13) | structs (§9) + the cast idea it replaces |
| `**sp`, `(*sp)--` (§14) | pointers (§2) + out-params (§8) |
| Nibble packing (§15) | bit ops + driver function pointers (§16) |
| `ISR` + `__attribute__` (§17) | hardware/bit-level, standalone |
| `REGISTER_AUTOSTART` (§18) | function pointers (§7) + tagged struct (§19) + designated init (§11) + `&` (§1) |
| Self-referential `*next` list (§19) | tagged structs + `NULL` (§6) + `&` (§1) |

If a line ever looks impenetrable, find which rows of the tables in Part 1 it's built from — every complex construct here is just several of those five basic symbols stacked together.
