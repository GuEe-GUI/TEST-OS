RES_0                   equ 0x112           ; VBE 模式: 640  x  480 x 24 Bits
RES_1                   equ 0x115           ; VBE 模式: 800  x  600 x 24 Bits
RES_2                   equ 0x118           ; VBE 模式: 1024 x  768 x 24 Bits
RES_3                   equ 0x11B           ; VBE 模式: 1280 x 1024 x 24 Bits

VBEMODE                 equ RES_1

DISPLAY_MODE_FLAG       equ 0x700           ; 保存显存信息的物理地址（实模式）
VCOLOR                  equ 0               ; 保存颜色数的偏移
SCREENX                 equ 2               ; 保存分辨率X的偏移
SCREENY                 equ 4               ; 保存分辨率Y的偏移
VRAM                    equ 6               ; 保存显存地址的偏移

ARDS_SAVE_ADDR          equ 0x701           ; 保存ARDS信息的物理地址（实模式）
ARDS_N                  equ 0               ; 保存ARDS数量的偏移
ARDS_BUF                equ 4

LINE_WIDE               equ 160             ; VGA字符显存宽度
VIDEO_MEMORY            equ 0xb800          ; VGA字符显存地址（实模式）
VIDEO_MEMORY_PROTECT    equ 0xb8000         ; VGA字符显存地址（保护模式）
VIDEO_MEMORY_PAGE       equ 0x800b8000      ; VGA字符显存地址（分页模式）
FONT_BACKGROUND_COLOR   equ 0x03            ; VGA字符前景和背景

PAGE_DIR_ADDR           equ 0x3000          ; 页表物理地址
VBE_PAGE_ADDR           equ 0x4000          ; 显存页目录物理地址
PA_PAGE_ADDR            equ 0x5000          ; 物理内存页目录物理地址
VA_PAGE_ADDR            equ 0x6000          ; 虚拟内存页目录物理地址

KERNEL_START_SECTOR     equ 0x9             ; 存放内核的扇区号
KERNEL_SECTORS          equ 348             ; 存放内核占用的扇区数量
KERNEL_BIN_BASE_ADDR    equ 0x10000         ; 内核加载的物理地址（实模式）
VIR_KERNEL_ENTRY        equ 0x80100000      ; 内核的虚拟地址入口

[bits 16]

org 0x70000

; 初始化所有段寄存器
START:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0

    mov si, BOOT_OK_MSG
    mov cx, 12
    mov di, 0
    call POINT_MESSAGE_IN16BIT

    mov dx, 11
    call SET_CUR_IN_REAL

    jmp LOAD_KERNEL

; 加载内核
LOAD_KERNEL:
    mov ax, 0x1000
    mov si, KERNEL_START_SECTOR
    mov cx, 128
    call LOAD_FILE

    mov ax, 0x2000
    mov si, KERNEL_START_SECTOR + 128
    mov cx, 128
    call LOAD_FILE

    mov ax, 0x3000
    mov si, KERNEL_START_SECTOR + 256
    mov cx, 128
    call LOAD_FILE

    mov ax, 0x5000
    mov si, 400
    mov cx, 25
    call LOAD_FILE

    jmp GET_MEMORY

; 0xe820读取内存
GET_MEMORY:
    xor ebx, ebx
    mov edx, 0x534d4150
    mov di, ARDS_BUF
    mov ax, ARDS_SAVE_ADDR
    mov es, ax
    mov word [es:ARDS_N], 0

LOOP_GET_MEMORY_0XE820:
    mov eax, 0xe820
    mov ecx, 20
    int 0x15

    jc GET_MEMORY_FAIL                      ; Cf标志，0为成功，1为失败
    add di, cx                              ; 跳过一个ARDS结构体的大小（20 = 5 * sizeof(uint32_t)）
    inc word [es:ARDS_N]                    ; 记录地址增加ARDS数量

    cmp ebx, 0                              ; Ebx不为0则还需要继续读
    jnz LOOP_GET_MEMORY_0XE820

GET_MEMORY_COMPLETE:
    mov si, MEMORY_OK_MSG
    mov cx, 18
    mov di, LINE_WIDE
    call POINT_MESSAGE_IN16BIT

    mov dx, (LINE_WIDE >> 1) + 17
    call SET_CUR_IN_REAL

    jmp CHECK_VBE_EXISTS

GET_MEMORY_FAIL:
    mov si, MEMORY_ERROR_MSG
    mov cx, 19
    mov di, LINE_WIDE
    call POINT_MESSAGE_IN16BIT

    mov dx, (LINE_WIDE >> 1) + 18
    call SET_CUR_IN_REAL

    jmp $
    hlt

CHECK_VBE_EXISTS:
    mov ax, 0x9000                          ; 检查VBE模式是否支持, 缓冲区地址在0x90000
    mov es, ax
    mov di, 0
    mov ax, 0x4f00                          ; 检查VBE模式支持中断参数 ax=0x4f00
    int 0x10

    cmp ax, 0x004f                          ; Ax=0x004f说明支持
    jne VBE_SET_FAIL

CHECK_VBE_VERSION:
    mov ax, [es:di + 4]                     ; 检查VBE版本，至少为2.0
    cmp ax, 0x200
    jb VBE_SET_FAIL

GET_VBE_INFORMATION:
    mov cx, VBEMODE                         ; 检查目前设置的分辨率是否支持
    mov ax, 0x4f01                          ; 检查VBE模式支持中断参数 ax=0x4f01
    int 0x10

    cmp ax, 0x004f                          ; Ax=0x004f说明支持
    jne VBE_SET_FAIL

SET_RESOLUTION:
    mov bx, VBEMODE + 0x4000
    mov ax, 0x4f02
    int 0x10                                ; 设定VBE模式

    mov byte [VCOLOR], 8                    ; 保存颜色数信息
    mov ax, [es:di + 0x12]                  ; 保存分辨率X信息
    mov [SCREENX], ax
    mov ax, [es:di + 0x14]                  ; 保存分辨率Y信息
    mov [SCREENY], ax
    mov eax, [es:di + 0x28]                 ; 保存显存地址信息
    mov [VRAM], eax

    jmp LOAD_GDT_AND_IDT

VBE_SET_FAIL:
    mov si, VGA_MODE_FAIL_MSG
    mov cx, 19
    mov di, LINE_WIDE * 2 + 60
    call POINT_MESSAGE_IN16BIT

POINT_MESSAGE_IN16BIT:
    mov ax, VIDEO_MEMORY
    mov es, ax

.POINT_STRING_IN16BIT:
    mov al, [si]                            ; 字符串地址
    mov byte [es:di], al                    ; 打印当前字符
    inc si                                  ; 下一个字符
    inc di                                  ; 光标下一个位置
    mov byte [es:di], 0x0c                  ; 设置字符前景和背景
    inc di                                  ; 光标下一个位置
    loop .POINT_STRING_IN16BIT

    ret

SET_CUR_IN_REAL:
    mov ah, 0x2                             ; 设置光标位置
    mov bh, 0
    int 0x10

    ret

LOAD_GDT_AND_IDT:
    lgdt[GDT_DESCRIPTOR]
    lidt[IDT_DESCRIPTOR]

ENABLE_A20GATE:
    in al, 0x92
    or al, 2
    out 0x92, al

    cli                                     ; 关闭中断

SET_BIT_OF_PE:
    mov eax, cr0
    or eax, 1
    mov cr0, eax

ENTER_PROTECTED_MODE:
    jmp dword CODE_ADDRESS:FLUSH            ; 进入保护模式

; 1kb对齐
times 1024 - ($ - $$) db 0

GDT_START:

; NULL
GDT_NULL:
    dd 0
    dd 0

; 代码段
GDT_CODE:
    dw 0xffff                               ; 段限制：0-15位
    dw 0                                    ; 段基地址：0-15位
    db 0                                    ; 段基地址：16-23位
    db 10011010b                            ; 段描述符的第6字节属性（代码段可读写）
    db 11001111b                            ; 段描述符的第7字节属性：16-19位
    db 0                                    ; 段描述符的最后一个字节是段基地址的第二部分：24-31位

; 数据段
GDT_DATA:
    dw 0xffff                               ; 段限制：0-15位
    dw 0                                    ; 段基地址：0-15位
    db 0                                    ; 段基地址：16-23位
    db 10010010b                            ; 段描述符的第6字节属性（数据段可读写）
    db 11001111b                            ; 段描述符的第7字节属性：16-19位
    db 0                                    ; 段描述符的最后一个字节是段基地址的第二部分：24-31位

GDT_END:

GDT_DESCRIPTOR:
    dw GDT_END-GDT_START-1                  ; GDT 大小
    dd GDT_START                            ; GDT 地址

IDT_DESCRIPTOR:
    dw 0
    dw 0
    dw 0

CODE_ADDRESS equ GDT_CODE-GDT_START
DATA_ADDRESS equ GDT_DATA-GDT_START

; 读取软盘，拷贝数据
READ_FLOPPY_SECTOR:
    push ax
    push cx
    push dx
    push bx

    mov ax, si
    xor dx, dx
    mov bx, 18

    div bx
    inc dx
    mov cl, dl
    xor dx, dx
    mov bx, 2
    div bx

    mov dh, dl
    xor dl, dl
    mov ch, al
    pop bx

.REPEATED_PASTING:
    mov al, 0x01
    mov ah, 0x02
    int 0x13

    jc .REPEATED_PASTING
    pop dx
    pop cx
    pop ax

    ret

LOAD_FILE:
    mov es, ax
    xor bx, bx

.LOOP:
    call READ_FLOPPY_SECTOR
    add bx, 512
    inc si
    loop .LOOP

    ret

[bits 32]

FLUSH:
    mov cx, 0x10                            ; 数据段
    mov ds, cx
    mov es, cx
    mov gs, cx
    mov cx, 0x10                            ; 栈段
    mov ss, cx
    mov esp, 0x70000                        ; 设置栈指针

    mov esi, PROTECTED_MODE_OK_MSG
    mov ecx, 22
    mov edi, 0

.POINT_PROTECT_OK:
    mov eax, [esi]
    mov dword [VIDEO_MEMORY_PROTECT + LINE_WIDE * 3 + edi], eax
    inc esi
    inc edi
    mov dword [VIDEO_MEMORY_PROTECT + LINE_WIDE * 3 + edi], FONT_BACKGROUND_COLOR
    inc edi
    loop .POINT_PROTECT_OK

    mov bx, 3 * (LINE_WIDE >> 1) + 21
    call SET_CUR_IN_32BITS

START_TO_PAGING_MODE:
    mov ecx, 1024
    mov ebx, PAGE_DIR_ADDR
    xor esi, esi

.CLEAN_PDT:
    mov byte [ebx + esi], 0
    inc esi
    loop .CLEAN_PDT

    mov eax, 0                              ; 映射虚拟地址 0（此处为1：1映射）
    shr eax, 22                             ; Eax >>= 22
    shl eax, 2                              ; Eax *= 4（地址字节宽度4字节）
    mov dword [PAGE_DIR_ADDR + eax], PA_PAGE_ADDR | 0x7

    mov edi, PA_PAGE_ADDR
    mov esi, 0
    or esi, 0x07
    mov ecx, 1024                           ; 映射内核物理地址空间
    call .MAPPING_ADDR

    mov eax, 0x80000000                     ; 映射虚拟地址 0x80000000
    shr eax, 22
    shl eax, 2
    mov dword [PAGE_DIR_ADDR + eax], VA_PAGE_ADDR | 0x7

    mov edi, VA_PAGE_ADDR
    mov esi, 0
    or esi, 0x07
    mov ecx, 1024                           ; 映射内核虚拟地址空间
    call .MAPPING_ADDR

    mov eax, [0x70000 + VRAM]               ; 映射显存虚拟地址
    shr eax, 22
    shl eax, 2
    mov dword [PAGE_DIR_ADDR + eax], VBE_PAGE_ADDR | 0x07

    mov edi, VBE_PAGE_ADDR
    mov esi, [0x70000 + VRAM]
    or esi, 0x07
    mov ecx, 1024                           ; 映射4M显存
    call .MAPPING_ADDR

    jmp SAVE_THE_ADDRESS_TO_CR3

; 页表映射
.MAPPING_ADDR:
    mov dword [edi], esi
    add edi, 4
    add esi, 0x1000
    loop .MAPPING_ADDR
    ret

SAVE_THE_ADDRESS_TO_CR3:
    mov eax, PAGE_DIR_ADDR
    mov cr3, eax

SET_BIT_OF_PG:
    mov eax, cr0
    or eax, 1 << 31                         ; PG = 0x80000000
    mov cr0, eax

ENTER_PAGING_MODE_COMPLETE:
    mov esi, PAGING_OK_MSG
    mov ecx, 19
    mov edi, 4 * LINE_WIDE
    call POINT_STRING_IN32BIT_PAGE

    mov bx, 4 * (LINE_WIDE >> 1) + 18
    call SET_CUR_IN_32BITS

; Contains: Start to into the kernel
CALL_KERNEL:
    mov esi, LOADING_SYSTEM_MSG
    mov ecx, 18
    mov edi, 5 * LINE_WIDE
    call POINT_STRING_IN32BIT_PAGE

    mov bx, 5 * (LINE_WIDE >> 1) + 17
    call SET_CUR_IN_32BITS

    call INIT_KERNEL

    call CLEAR_SCREEN

    mov bx, 0
    call SET_CUR_IN_32BITS

    jmp VIR_KERNEL_ENTRY                    ; 进入内核

INIT_KERNEL:
   xor eax, eax
   xor ebx, ebx                             ; 记录每一个Program Header Table地址
   xor ecx, ecx                             ; 记录每一个Program Header Table数量
   xor edx, edx                             ; 记录每一个Program Header Table的大小：e_phentsize

   mov dx, [KERNEL_BIN_BASE_ADDR + 42]      ; 偏移42字节：e_phentsize
   mov ebx, [KERNEL_BIN_BASE_ADDR + 28]     ; 偏移28字节：e_phoff，第一个program header偏移量
   add ebx, KERNEL_BIN_BASE_ADDR
   mov cx, [KERNEL_BIN_BASE_ADDR + 44]      ; 偏移44字节： e_phnum

.EACH_SEGMENT:
   cmp byte [ebx + 0], 0                    ; PT_NULL
   je .PTNULL

   push dword [ebx + 16]                    ; 程序头中16字节的偏移量是p_filesz，函数memcpy的第三个参数：size
   mov eax, [ebx + 4]                       ; 从程序头偏移 4 个字节的位置为 p_offset
   add eax, KERNEL_BIN_BASE_ADDR            ; 加上kernel.bin加载到的物理地址，eax就是段的物理地址
   push eax                                 ; Push函数memcpy的第二个参数：源地址
   push dword [ebx + 8]                     ; Push函数memcpy的第一个参数：目的地址，偏移程序的前8个字节的位置是p_vaddr，也就是目的地址
   call MEMORY_COPY                         ; 调用mem_cpy完成段拷贝
   add esp, 12                              ; 调用mem_cpy完成段拷贝

.PTNULL:
   add ebx, edx                             ; Edx为程序头大小，即e_phentsize，其中ebx指向下一个程序头
   loop .EACH_SEGMENT

   ret

MEMORY_COPY:
   cld
   push ebp
   mov ebp, esp
   push ecx                                 ; Ecx保存
   mov edi, [ebp + 8]                       ; Dst参数
   mov esi, [ebp + 12]                      ; Src参数
   mov ecx, [ebp + 16]                      ; Size参数
   rep movsb                                ; 逐字节拷贝

   pop ecx                                  ; Ecx恢复
   pop ebp

   ret

; 分页模式下清屏
CLEAR_SCREEN:
    mov eax, VIDEO_MEMORY_PAGE
    mov edi, eax
    mov ecx, 2000

.CLEAR_POINT:
    mov dword [edi + 0], 0
    add edi, 1
    mov dword [edi + 0], 0xf
    add edi, 1
    loop .CLEAR_POINT

    ret

; 分页模式下打印字符串
POINT_STRING_IN32BIT_PAGE:
    mov eax, [esi]
    mov dword [VIDEO_MEMORY_PAGE + edi], eax
    inc esi
    inc edi
    mov dword [VIDEO_MEMORY_PAGE + edi], FONT_BACKGROUND_COLOR
    inc edi
    loop POINT_STRING_IN32BIT_PAGE

    ret

; 保护模式后设置光标位置
SET_CUR_IN_32BITS:
    mov dx, 0x3d4
    mov al, 0x0e
    out dx, al

    mov dx, 0x3d5
    mov al, bh                              ; Bh = 0 (屏幕Y)
    out dx, al

    mov dx, 0x3d4
    mov al, 0x0f
    out dx, al

    mov dx, 0x3d5
    mov al, bl                              ; Bl = 0 (屏幕X)
    out dx, al

    ret

; 要打印的字符串信息
BOOT_OK_MSG             db  '[ OK ] Boot '              ; 字符串长度： 12
MEMORY_OK_MSG           db  '[ OK ] Get Memory '        ; 字符串长度： 18
MEMORY_ERROR_MSG        db  'Get Memory Failed! '       ; 字符串长度： 19
VGA_MODE_FAIL_MSG       db  'Set VGA MODE Fail! '       ; 字符串长度： 19
PROTECTED_MODE_OK_MSG   db  '[ OK ] Protected Mode '    ; 字符串长度： 22
PAGING_OK_MSG           db  '[ OK ] Paging Mode '       ; 字符串长度： 19
LOADING_SYSTEM_MSG      db  'Loading System... '        ; 字符串长度： 18

times 4096 - ($ - $$) db 0
