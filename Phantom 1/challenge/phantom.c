
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>

#define CODE_SIZE   2048
#define STACK_SIZE  64
#define REG_COUNT   16
#define MAX_INPUT   (CODE_SIZE * 2 + 2) 


#define OP_NOP    0x00
#define OP_HALT   0x01
#define OP_PUSH   0x02  
#define OP_POP    0x03  
#define OP_ADD    0x04  
#define OP_SUB    0x05  
#define OP_MUL    0x06  
#define OP_XOR    0x07  
#define OP_AND    0x08  
#define OP_OR     0x09  
#define OP_SHR    0x0A  
#define OP_SHL    0x0B  
#define OP_CMP    0x0C 
#define OP_JMP    0x0D  
#define OP_JZ     0x0E  
#define OP_JNZ    0x0F  
#define OP_PRINT  0x10 
#define OP_DUP    0x11  
#define OP_SWAP   0x12 
#define OP_PUSHR  0x13 
#define OP_PEEK   0x14  
#define OP_POKE   0x15  
#define OP_MOV    0x16  
#define OP_INC    0x17  

typedef struct {
    uint8_t  code[CODE_SIZE];
    int32_t  ip; 
    int32_t  sp; 
    int32_t  running;
    int32_t  flags;
    int64_t  regs[REG_COUNT];
    int64_t  vm_stack[STACK_SIZE];      
} PhantomVM;

static void vm_error(PhantomVM *vm, const char *msg) {
    fprintf(stderr, "[!] VM Error: %s (ip=%d)\n", msg, vm->ip - 1);
    vm->running = 0;
}

static inline int check_push(PhantomVM *vm) {
    if (vm->sp >= STACK_SIZE) {
        vm_error(vm, "stack overflow");
        return 0;
    }
    return 1;
}

static inline int check_pop(PhantomVM *vm, int n) {
    if (vm->sp < n) {
        vm_error(vm, "stack underflow");
        return 0;
    }
    return 1;
}

static inline int valid_reg(PhantomVM *vm, uint8_t r) {
    if (r >= REG_COUNT) {
        vm_error(vm, "invalid register");
        return 0;
    }
    return 1;
}

static void run_vm(uint8_t *bytecode, size_t len) {
    PhantomVM vm;
    memset(&vm, 0, sizeof(vm));

    if (len > CODE_SIZE) len = CODE_SIZE;
    memcpy(vm.code, bytecode, len);
    vm.running = 1;

    while (vm.running && vm.ip >= 0 && vm.ip < (int32_t)len) {
        uint8_t op = vm.code[vm.ip++];

        switch (op) {

        case OP_NOP:
            break;

        case OP_HALT:
            vm.running = 0;
            break;


        case OP_PUSH: {
            if (vm.ip + 8 > (int32_t)len) { vm_error(&vm, "truncated PUSH"); break; }
            if (!check_push(&vm)) break;
            int64_t val;
            memcpy(&val, &vm.code[vm.ip], 8);
            vm.ip += 8;
            vm.vm_stack[vm.sp++] = val;
            break;
        }

    
        case OP_POP: {
            if (vm.ip >= (int32_t)len) { vm_error(&vm, "truncated POP"); break; }
            uint8_t r = vm.code[vm.ip++];
            if (!valid_reg(&vm, r)) break;
            if (!check_pop(&vm, 1)) break;
            vm.regs[r] = vm.vm_stack[--vm.sp];
            break;
        }

       
        case OP_ADD: case OP_SUB: case OP_MUL:
        case OP_XOR: case OP_AND: case OP_OR:
        case OP_SHR: case OP_SHL: {
            if (!check_pop(&vm, 2)) break;
            int64_t b = vm.vm_stack[--vm.sp];
            int64_t a = vm.vm_stack[--vm.sp];
            int64_t result = 0;
            switch (op) {
                case OP_ADD: result = a + b; break;
                case OP_SUB: result = a - b; break;
                case OP_MUL: result = a * b; break;
                case OP_XOR: result = a ^ b; break;
                case OP_AND: result = a & b; break;
                case OP_OR:  result = a | b; break;
                case OP_SHR: result = (int64_t)((uint64_t)a >> (b & 63)); break;
                case OP_SHL: result = a << (b & 63); break;
            }
            vm.vm_stack[vm.sp++] = result;
            break;
        }

        
        case OP_CMP: {
            if (!check_pop(&vm, 2)) break;
            int64_t b = vm.vm_stack[--vm.sp];
            int64_t a = vm.vm_stack[--vm.sp];
            if (a == b)      vm.flags = 0;
            else if (a < b)  vm.flags = -1;
            else             vm.flags = 1;
            break;
        }

        
        case OP_JMP: {
            if (vm.ip + 2 > (int32_t)len) { vm_error(&vm, "truncated JMP"); break; }
            int16_t off;
            memcpy(&off, &vm.code[vm.ip], 2);
            vm.ip = (int32_t)off;
            break;
        }
        case OP_JZ: {
            if (vm.ip + 2 > (int32_t)len) { vm_error(&vm, "truncated JZ"); break; }
            int16_t off;
            memcpy(&off, &vm.code[vm.ip], 2);
            vm.ip += 2;
            if (vm.flags == 0) vm.ip = (int32_t)off;
            break;
        }
        case OP_JNZ: {
            if (vm.ip + 2 > (int32_t)len) { vm_error(&vm, "truncated JNZ"); break; }
            int16_t off;
            memcpy(&off, &vm.code[vm.ip], 2);
            vm.ip += 2;
            if (vm.flags != 0) vm.ip = (int32_t)off;
            break;
        }

      
        case OP_PRINT: {
            if (!check_pop(&vm, 1)) break;
            int64_t val = vm.vm_stack[--vm.sp];
            printf("0x%016lx\n", (unsigned long)val);
            fflush(stdout);
            break;
        }

       
        case OP_DUP: {
            if (!check_pop(&vm, 1)) break;
            if (!check_push(&vm)) break;
            vm.vm_stack[vm.sp] = vm.vm_stack[vm.sp - 1];
            vm.sp++;
            break;
        }
        case OP_SWAP: {
            if (!check_pop(&vm, 2)) break;
            int64_t tmp = vm.vm_stack[vm.sp - 1];
            vm.vm_stack[vm.sp - 1] = vm.vm_stack[vm.sp - 2];
            vm.vm_stack[vm.sp - 2] = tmp;
            break;
        }

        
        case OP_PUSHR: {
            if (vm.ip >= (int32_t)len) { vm_error(&vm, "truncated PUSHR"); break; }
            uint8_t r = vm.code[vm.ip++];
            if (!valid_reg(&vm, r)) break;
            if (!check_push(&vm)) break;
            vm.vm_stack[vm.sp++] = vm.regs[r];
            break;
        }
        case OP_MOV: {
            if (vm.ip + 2 > (int32_t)len) { vm_error(&vm, "truncated MOV"); break; }
            uint8_t r1 = vm.code[vm.ip++];
            uint8_t r2 = vm.code[vm.ip++];
            if (!valid_reg(&vm, r1) || !valid_reg(&vm, r2)) break;
            vm.regs[r1] = vm.regs[r2];
            break;
        }
        case OP_INC: {
            if (vm.ip >= (int32_t)len) { vm_error(&vm, "truncated INC"); break; }
            uint8_t r = vm.code[vm.ip++];
            if (!valid_reg(&vm, r)) break;
            vm.regs[r]++;
            break;
        }

        
        case OP_PEEK: {
            if (vm.ip >= (int32_t)len) { vm_error(&vm, "truncated PEEK"); break; }
            uint8_t r = vm.code[vm.ip++];
            if (!valid_reg(&vm, r)) break;
            if (!check_push(&vm)) break;
            int64_t idx = vm.regs[r];
            
        }

      
        case OP_POKE: {
            if (vm.ip >= (int32_t)len) { vm_error(&vm, "truncated POKE"); break; }
            uint8_t r = vm.code[vm.ip++];
            if (!valid_reg(&vm, r)) break;
            if (!check_pop(&vm, 1)) break;
            int64_t idx = vm.regs[r];
            vm.vm_stack[idx] = vm.vm_stack[--vm.sp];
            break;
        }

        default:
            vm_error(&vm, "unknown opcode");
            break;
        }
    }
}

static ssize_t hex_decode(const char *hex, uint8_t *out, size_t max_out) {
    size_t hex_len = strlen(hex);
    while (hex_len > 0 && isspace((unsigned char)hex[hex_len - 1]))
        hex_len--;

    if (hex_len == 0 || hex_len % 2 != 0) return -1;
    size_t out_len = hex_len / 2;
    if (out_len > max_out) return -1;

    for (size_t i = 0; i < out_len; i++) {
        char byte_str[3] = { hex[i*2], hex[i*2+1], 0 };
        char *end;
        unsigned long val = strtoul(byte_str, &end, 16);
        if (*end != '\0') return -1;
        out[i] = (uint8_t)val;
    }
    return (ssize_t)out_len;
}

static void print_banner(void) {
    puts("");
    puts("  ╔═══════════════════════════════════════════════╗");
    puts("  ║                  PHANTOM                      ║");
    puts("  ║          Spectral Bytecode Engine             ║");
    puts("  ║                                               ║");
    puts("  ║          \"The dead speak in bytecode...\"     ║");
    puts("  ╚═══════════════════════════════════════════════╝");
    puts("");
    puts("  Submit your phantom script as hex-encoded bytecode.");
    printf("  Max code size: %d bytes (%d hex chars)\n", CODE_SIZE, CODE_SIZE * 2);
    puts("");
}

int main(void) {
    char input[MAX_INPUT];
    uint8_t bytecode[CODE_SIZE];

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    signal(SIGALRM, SIG_DFL);
    alarm(30);

    print_banner();

    printf("phantom> ");

    if (fgets(input, sizeof(input), stdin) == NULL) {
        puts("[!] No input received.");
        return 1;
    }

    ssize_t code_len = hex_decode(input, bytecode, CODE_SIZE);
    if (code_len < 0) {
        puts("[!] Invalid hex input.");
        return 1;
    }

    printf("Loaded %zd bytes of bytecode. Executing...\n\n", code_len);

    run_vm(bytecode, (size_t)code_len);

    puts("\nExecution complete.");
    return 0;
}
