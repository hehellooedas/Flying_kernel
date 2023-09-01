%include "boot.inc"

org 0x7c00
[bits 16]
align 16
    mov ax,cs    ;从寄存器传递数据到寄存器的效率更高
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov fs,ax
    mov sp,0x7c00;设置栈的地址
    mov ax,0xb800
    mov gs,ax    ;显示段

    ;调用BIOS清屏中断
    mov ax,0x600
    mov bx,0x700
    mov cx,0
    mov dx,0x184f
    int 0x10

    ;输出MBR提示信息
    mov byte [gs:0x00],'M'
    mov byte [gs:0x01],0x07

    mov byte [gs:0x02],'B'
    mov byte [gs:0x03],0x07

    mov byte [gs:0x04],'R'
    mov byte [gs:0x05],0x07

    mov byte [gs:0x06],' '
    mov byte [gs:0x07],0x07

    mov byte [gs:0x08],'i'
    mov byte [gs:0x09],0x07

    mov byte [gs:0x0a],'s'
    mov byte [gs:0x0b],0x07

    mov byte [gs:0x0c],' '
    mov byte [gs:0x0d],0x07

    mov byte [gs:0x0e],'l'
    mov byte [gs:0x0f],0x07

    mov byte [gs:0x10],'o'
    mov byte [gs:0x11],0x07

    mov byte [gs:0x12],'a'
    mov byte [gs:0x13],0x07

    mov byte [gs:0x14],'d'
    mov byte [gs:0x15],0x07

    mov byte [gs:0x16],'i'
    mov byte [gs:0x17],0x07

    mov byte [gs:0x18],'n'
    mov byte [gs:0x19],0x07

    mov byte [gs:0x1a],'g'
    mov byte [gs:0x1b],0x07

    mov byte [gs:0x1c],'.'
    mov byte [gs:0x1d],0x07

    ;jmp $ 悬停MBR以供调试

    ;把bootloader加载到内存中
    mov eax,LOADER_START_SECTOR ;起始扇区lba地址
    mov bx,LOADER_BASE_ADDR     ;写入的地址
    mov cx,4; 待读入的扇区数
    call rd_disk_m_16
    ;跳转到kernel loader
    jmp LOADER_BASE_ADDR + 0x300 ;0xc00是loader代码执行的入口



;读取硬盘第n个扇区
;eax:起始扇区
;bx:从硬盘里读取到的数据放到内存的地址位置
;cx:待读入的扇区数
rd_disk_m_16:
    mov esi,eax   ;备份eax
    mov di,cx     ;备份cx

;读写硬盘
;第一步：设置要读取的扇区数
    mov dx,0x1f2
    mov al,cl
    out dx,al
    mov eax,esi   ;还原eax

;第二步：将LBA地址存入0x1f3 ~ 0x1f6
    ;LBA地址7～0位写入端口0x1f3
    mov dx,0x1f3
    out dx,al

    ;LBA地址15～8位写入端口0x1f4
    mov cl,8
    shr eax,cl
    mov dx,0x1f4
    out dx,al

    ;LBA地址23~16位写入端口0x1f5
    shr eax,cl
    mov dx,0x1f5
    out dx,al

    shr eax,cl
    and al,0x0f
    or al,0xe0;设置7~4位为1110,表示LBA模式
    mov dx,0x1f6
    out dx,al

;第三步：向0x1f7端口写入读命令,0x20
    mov dx,0x1f7
    mov al,0x20
    out dx,al

;第四步：检测硬盘状态
.not_ready:
    nop
    in al,dx
    and al,0x88 ;(10001000)
    cmp al,0x08 ;(00001000)
    jnz .not_ready ;没准备好就继续检测

;第五步：从0x1f0端口读数据
    mov ax,di
    mov dx,256
    mul dx      ;(要读取的扇区数x每个扇区的大小)/每次读写的字节数=读取的次数
    mov cx,ax   ;总共要读取的次数(mul的结果保存在ax)

    mov dx,0x1f0

.go_on_read:
    in ax,dx   ;一次读取两个字节
    mov [bx],ax
    add bx,2
    loop .go_on_read
    
ret  ;读磁盘函数结束


;伪指令
times 510-($-$$) db 0
db 0x55,0xaa
