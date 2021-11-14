LOADER_ADDR         equ 0x7000      ; Loader程序地址
READ_SECTOR         equ 9           ; 要读取的扇区数量
BOOT_START          db  0           ; 启动扇区号
BOOT_MESSAGE        db  'Booting'   ; 启动信息

[bits 16]

org 0x7c00

; 初始化段寄存器
START:
    mov ax,cs
    mov ds,ax
    mov es,ax
    xor ax,ax
    mov ss,ax
    mov sp,0x7c00

    jmp POINT_BOOT_TITLE

POINT_BOOT_TITLE:
    mov ax,0x2              ; 清屏
    int 0x10

    mov ax,BOOT_MESSAGE
    mov bp,ax
    mov cx,7
    mov ax,0x1300
    mov bx,0xc
    mov dx,0
    int 0x10

    mov ah,0x2              ; 重新设置光标坐标
    mov bh,0
    mov dh,0
    mov dl,7
    int 0x10

    jmp READ_FLOPPY

READ_FLOPPY:
    mov dx,0x0              ; Dh为头部号，dl为驱动器号
    mov cx,0x2              ; 读取第二个扇区（扇区号起点为1，ch为柱面数，cl为扇区号）
    mov ax,LOADER_ADDR
    mov es,ax
    xor bx,bx
    mov ah,0x2
    mov al,READ_SECTOR
    int 0x13

    jnc ENTER_LOADER        ; Cf标志, 0 为成功, 1 为失败
    jmp READ_FLOPPY

; 跳转到Loader程序
ENTER_LOADER:
    jmp LOADER_ADDR:0

times 510-($-$$) db 0
dw 0xaa55
