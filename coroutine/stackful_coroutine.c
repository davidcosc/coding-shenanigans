#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct __attribute__((packed, aligned(16))) co_callee_reg_state_t {
    uint64_t x8;
    uint64_t x9;
    uint64_t x10;
    uint64_t x11;
    uint64_t x12;
    uint64_t x13;
    uint64_t x14;
    uint64_t x15;
    uint64_t x19;
    uint64_t x20;
    uint64_t x21;
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28;
    uint64_t fp;
    uint64_t sp;
    uint64_t ret;
};

/*
We must remain 16 byte aligned on stack. Keeping both structs aligned ensures this.
For alignment padding is added at the end.
*/
struct __attribute__((packed, aligned(16))) coroutine_t {
    struct co_callee_reg_state_t co_state;
    char stack[4096];
    void (*func)(int);
};

/*
We use attribute naked to prevent function prologue and epilogue creation.

We save and restore all callee saved registers as well as the stack pointer sp, stack frame pointer x29
and return address lr x30.
*/
__attribute__((naked)) void switch_state_aarch64(void *store_state, void *load_state) {
    __asm__(
        "str x8,[x0]\n"
        "str x9,[x0,#0x08]\n"
        "str x10,[x0,#0x10]\n"
        "str x11,[x0,#0x18]\n"
        "str x12,[x0,#0x20]\n"
        "str x13,[x0,#0x28]\n"
        "str x14,[x0,#0x30]\n"
        "str x15,[x0,#0x38]\n"
        "str x19,[x0,#0x40]\n"
        "str x20,[x0,#0x48]\n"
        "str x21,[x0,#0x50]\n"
        "str x22,[x0,#0x58]\n"
        "str x23,[x0,#0x60]\n"
        "str x24,[x0,#0x68]\n"
        "str x25,[x0,#0x70]\n"
        "str x26,[x0,#0x78]\n"
        "str x27,[x0,#0x80]\n"
        "str x28,[x0,#0x88]\n"
        "str fp,[x0,#0x90]\n"
        "mov x16,sp\n"
        "str x16,[x0,#0x98]\n"
        "str lr,[x0,#0xa0]\n"

        "ldr x8,[x1]\n"
        "ldr x9,[x1,#0x08]\n"
        "ldr x10,[x1,#0x10]\n"
        "ldr x11,[x1,#0x18]\n"
        "ldr x12,[x1,#0x20]\n"
        "ldr x13,[x1,#0x28]\n"
        "ldr x14,[x1,#0x30]\n"
        "ldr x15,[x1,#0x38]\n"
        "ldr x19,[x1,#0x40]\n"
        "ldr x20,[x1,#0x48]\n"
        "ldr x21,[x1,#0x50]\n"
        "ldr x22,[x1,#0x58]\n"
        "ldr x23,[x1,#0x60]\n"
        "ldr x24,[x1,#0x68]\n"
        "ldr x25,[x1,#0x70]\n"
        "ldr x26,[x1,#0x78]\n"
        "ldr x27,[x1,#0x80]\n"
        "ldr x28,[x1,#0x88]\n"
        "ldr fp,[x1,#0x90]\n"
        "ldr x16,[x1,#0x98]\n"
        "mov sp,x16\n"
        "ldr lr,[x1,#0xa0]\n"
        "ret\n"
    );
}

struct coroutine_t create_coro(void (*func)(int)) {
    struct coroutine_t coro;

    coro.func = func;
    coro.co_state.ret = (unsigned long long)func; // For some reason compiler treats func addr as 32 bit initially. We extend to 64.
    coro.co_state.sp = (unsigned long long)&coro.stack[4095] + 1; // Set sp to end of coro stack.
    coro.co_state.fp = (unsigned long long)&coro.stack[4095] + 1; // Set frame pointer to end of coro stack.
    return coro;
}

static struct coroutine_t x;
static struct coroutine_t y;
static struct coroutine_t z;

void work_y() {
    while (1) {
        printf("Hello from y 1!\n");
        switch_state_aarch64((void *)&y, (void *)&x);
        printf("Hello from y 2!\n");
        switch_state_aarch64((void *)&y, (void *)&x);
    }
}

void work_z() {
    while (1) {
        printf("Hello from z!\n");
        switch_state_aarch64((void *)&z, (void *)&x);
    }
}

int main() {
    /* We do not care about the x coros initialization state.
       On the first co_swap the x coro objects state will be overwritten
       with the current program state of main.
    */
    x = create_coro(NULL);
    y = create_coro(work_y);
    z = create_coro(work_z);

    while (1) {
        printf("Hello from x 1!\n");
        switch_state_aarch64((void *)&x, (void *)&y);
        printf("Hello from x 2!\n");
        switch_state_aarch64((void *)&x, (void *)&z);
        sleep(1);
    }

    return 0;
}
