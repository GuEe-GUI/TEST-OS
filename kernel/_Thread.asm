[section .text]
[bits 32]

; 引用intel手册原文：
; All registers on the Intel386 are global and thus visible to both a calling
; and a called function. Registers %ebp, %ebx, %edi, %esi, and %esp ‘‘belong’’
; to the calling function. In other words, a called function must preserve these
; registers’ values for its caller. Remaining registers ‘‘belong’’ to the called
; function. If a calling function wants to preserve such a register value across
; a function call, it must save the value in its local stack frame.

global switch_to_next_thread

; void switch_to_next_thread(void *prev, void *next);
switch_to_next_thread:
    mov eax, [esp + 4]  ; prev
    mov edx, [esp + 8]  ; next

    ; 保存context相关寄存器信息到prev
    mov [eax +  0], edi
    mov [eax +  4], esi
    mov [eax +  8], ebx
    mov [eax + 12], ebp
    mov [eax + 16], esp

    ; 从next恢复context相关寄存器信息
    mov edi, [edx +  0]
    mov esi, [edx +  4]
    mov ebx, [edx +  8]
    mov ebp, [edx + 12]
    mov esp, [edx + 16]

    ; 调用方关闭中断，要重新打开
    sti

    ret
