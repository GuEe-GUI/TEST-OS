KERNEL_STACK_TOP equ 0x80002000

[section .text]
[bits 32]

extern entry

global _start
_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, KERNEL_STACK_TOP

    jmp entry
