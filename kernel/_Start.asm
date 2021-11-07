KERNEL_STACK_TOP equ 0x80001000

[section .text]
[bits 32]

extern entry

[section .text]
global _START

_START:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, KERNEL_STACK_TOP

    call entry

CPU_HLT:
    hlt
    jmp CPU_HLT
