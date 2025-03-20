/* 
Implementation of co_swap routine for x86_64 in fluent bit.
First pointer to next coro saved regs on stack are passed in rdi, current coro saved regs in rsi.
0x48, 0x89, 0x26,        mov [rsi],rsp    # save current coro stack pointer
0x48, 0x8b, 0x27,        mov rsp,[rdi]    # restore next coro stack pointer into stack pointer register
0x58,                    pop rax          # pop next coro ret addr to rax
0x48, 0x89, 0x6e, 0x08,  mov [rsi+ 8],rbp # save remaining current coro callee regs
0x48, 0x89, 0x5e, 0x10,  mov [rsi+16],rbx 
0x4c, 0x89, 0x66, 0x18,  mov [rsi+24],r12 
0x4c, 0x89, 0x6e, 0x20,  mov [rsi+32],r13 
0x4c, 0x89, 0x76, 0x28,  mov [rsi+40],r14 
0x4c, 0x89, 0x7e, 0x30,  mov [rsi+48],r15 
0x48, 0x8b, 0x6f, 0x08,  mov rbp,[rdi+ 8] # restore remaining next coro callee regs
0x48, 0x8b, 0x5f, 0x10,  mov rbx,[rdi+16] 
0x4c, 0x8b, 0x67, 0x18,  mov r12,[rdi+24] 
0x4c, 0x8b, 0x6f, 0x20,  mov r13,[rdi+32] 
0x4c, 0x8b, 0x77, 0x28,  mov r14,[rdi+40] 
0x4c, 0x8b, 0x7f, 0x30,  mov r15,[rdi+48] 
0xff, 0xe0,              jmp rax          # return aka jump to next coro and continue execution
*/

#include <stdio.h>
#include <unistd.h>

struct __attribute__((packed)) co_callee_reg_state_t {
    uint64_t x8;
    uint64_t x9;
    uint64_t x10;
    uint64_t x11;
    uint64_t x12;
    uint64_t x13;
    uint64_t x14;
    uint64_t x15;
    char padding[8];
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
    uint64_t x29;
    uint64_t sp;
    uint64_t ret_addr;
};

struct coroutine_t {
    struct co_callee_reg_state_t co_state;
    void (*func)(int);
    char stack[256];
};

void co_switch_aarch64(struct co_callee_reg_state_t *next, struct co_callee_reg_state_t *current);

/* We must remain 16 byte aligned on stack. We add 8 bytes of padding between x15 and x19 storage.
   We save and restore all callee saved registers as well as the stack pointer sp, stack frame pointer x29
   and return address x30. We first store 2 registers a time whenever possible using stp or str for a single
   register otherwise. We store to address pointed to by arg 2. We then do the same in reverse and load the
   registers from address pointed to by arg 1.
*/
asm (
    ".text\n"
    ".globl co_switch_aarch64\n"
    ".globl _co_switch_aarch64\n"
    "co_switch_aarch64:\n"
    "_co_switch_aarch64:\n"
    "  stp x8,  x9,  [x1]\n"
    "  stp x10, x11, [x1, #16]\n"
    "  stp x12, x13, [x1, #32]\n"
    "  stp x14, x15, [x1, #48]\n"
    "  str x19, [x1, #72]\n"
    "  stp x20, x21, [x1, #80]\n"
    "  stp x22, x23, [x1, #96]\n"
    "  stp x24, x25, [x1, #112]\n"
    "  stp x26, x27, [x1, #128]\n"
    "  stp x28, x29, [x1, #144]\n"
    "  mov x16, sp\n"
    "  stp x16, x30, [x1, #160]\n"

    "  ldp x8,  x9,  [x0]\n"
    "  ldp x10, x11, [x0, #16]\n"
    "  ldp x12, x13, [x0, #32]\n"
    "  ldp x14, x15, [x0, #48]\n"
    "  ldr x19, [x0, #72]\n"
    "  ldp x20, x21, [x0, #80]\n"
    "  ldp x22, x23, [x0, #96]\n"
    "  ldp x24, x25, [x0, #112]\n"
    "  ldp x26, x27, [x0, #128]\n"
    "  ldp x28, x29, [x0, #144]\n"
    "  ldp x16, x17, [x0, #160]\n"
    "  mov sp, x16\n"
    "  br x17\n"
    ".previous\n"
);

static struct coroutine_t x;
static struct coroutine_t y;

void co_swap(struct coroutine_t *next, struct coroutine_t *current) {
    co_switch_aarch64(&(*next).co_state, &(*current).co_state);
}

struct coroutine_t create_coro(void (*func)(int)) {
    struct coroutine_t coro = {0};
    coro.func = func;
    coro.co_state.ret_addr = (unsigned long long)func; // For some reason compiler treats func addr as 32 bit initially. We extend to 64.
    coro.co_state.sp = (unsigned long long)&coro.stack[255] + 1; // Set sp to end of coro stack.
    coro.co_state.x29 = (unsigned long long)&coro.stack[255] + 1; // Set frame pointer to end of coro stack.
    return coro;
}

void work_y() {
    while (1) {
        printf("Hello from coroutine!\n");
        printf("x sp: %llu\n", x.co_state.sp);
        printf("x bp: %llu\n", x.co_state.x29);
        printf("x x8: %llu\n", x.co_state.x8);
        printf("x ret_addr: %llu\n", x.co_state.ret_addr);

        printf("y sp: %llu\n", y.co_state.sp);
        printf("y bp: %llu\n", y.co_state.x29);
        printf("y x8: %llu\n", y.co_state.x8);
        printf("y ret_addr: %llu\n", y.co_state.ret_addr);
        co_swap(&x, &y);
    }
}

int main() {
    /* We do not care about the x coros initialization state.
       On the first co_swap the x coro objects state will be overwritten
       with the current program state of main.
    */
    x = create_coro(NULL);
    y = create_coro(work_y);

    while (1) {
        printf("Hello from main!\n");
        printf("x sp: %llu\n", x.co_state.sp);
        printf("x bp: %llu\n", x.co_state.x29);
        printf("x x8: %llu\n", x.co_state.x8);
        printf("x ret_addr: %llu\n", x.co_state.ret_addr);

        printf("y sp: %llu\n", y.co_state.sp);
        printf("y bp: %llu\n", y.co_state.x29);
        printf("y x8: %llu\n", y.co_state.x8);
        printf("y ret_addr: %llu\n", y.co_state.ret_addr);
        co_swap(&y, &x);
        sleep(1);
    }

    return 0;
}
