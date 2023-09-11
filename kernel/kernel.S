;该文件中定义了interrupt和syscall调用的函数

[bits 32]
;保持格式一致
%define  ERROR_CODE  nop      ;已经压入了一个错误码，占个位置
%define  ZERO        push 0   ;没有压入错误码，补一个上去

extern idt_table  ;中断处理函数


section .data
global intr_entry_table ;
intr_entry_table:       ;入口表 


%macro VECTOR 2 ;有两个参数(%1是第一个参数/%2是第二个参数)
section .text   ;以下代码放在代码段
intr%1entry:    ;第%1个中断的入口
    %2          ;压入错误码或者0,把第二个输入的参数搬到这里来

    ;保存环境,越先push越在栈顶
    push ds
    push es
    push fs
    push gs
    pushad      ;eax，ecx，edx，ebx，esp、ebp，esi，edi
    

    mov al,0x20  ;发送中断结束命令
    out 0xa0,al  ;发送到从片
    out 0x20,al  ;发送到主片

    push %1      ;中断程序要压入中断向量号

    call [idt_table + %1*4] ;调用对应的中断处理函数
    jmp intr_exit

section .data ;由于它们位于同一个节，所有入口地址都会保存在数组中
    dd intr%1entry ;存储第%1个中断程序的入口地址，形成一个数组
%endmacro ;多行宏定义的结束符



section .text
global intr_exit
intr_exit:
    add esp,4  ;跳过中断向量号
    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp,4  ;跳过错误码
    iretd      ;这是退出中断的最后一条指令


; 内部中断
VECTOR 0x00,ZERO
VECTOR 0x01,ZERO
VECTOR 0x02,ZERO
VECTOR 0x03,ZERO 
VECTOR 0x04,ZERO
VECTOR 0x05,ZERO
VECTOR 0x06,ZERO
VECTOR 0x07,ZERO 
VECTOR 0x08,ERROR_CODE  ;#DF
VECTOR 0x09,ZERO
VECTOR 0x0a,ERROR_CODE  ;#TS
VECTOR 0x0b,ERROR_CODE  ;#NP
VECTOR 0x0c,ERROR_CODE  ;#SS
VECTOR 0x0d,ERROR_CODE  ;#GP
VECTOR 0x0e,ERROR_CODE  ;#PF
VECTOR 0x0f,ZERO 
VECTOR 0x10,ZERO
VECTOR 0x11,ERROR_CODE  ;#AC
VECTOR 0x12,ZERO
VECTOR 0x13,ZERO 
VECTOR 0x14,ZERO
VECTOR 0x15,ZERO
VECTOR 0x16,ZERO
VECTOR 0x17,ZERO 
VECTOR 0x18,ERROR_CODE
VECTOR 0x19,ZERO
VECTOR 0x1a,ERROR_CODE
VECTOR 0x1b,ERROR_CODE 
VECTOR 0x1c,ZERO
VECTOR 0x1d,ERROR_CODE
VECTOR 0x1e,ERROR_CODE
VECTOR 0x1f,ZERO 

; 外部中断
VECTOR 0x20,ZERO  ;时钟中断
VECTOR 0x21,ZERO  ;键盘中断
VECTOR 0x22,ZERO  ;级联用的
VECTOR 0x23,ZERO  ;串口2对应的入口
VECTOR 0x24,ZERO  ;串口1对应的入口
VECTOR 0x25,ZERO  ;并口2对应的入口
VECTOR 0x26,ZERO  ;软盘对应的入口
VECTOR 0x27,ZERO  ;并口1对应的入口
VECTOR 0x28,ZERO  ;实时时钟对应的入口
VECTOR 0x29,ZERO  ;重定向
VECTOR 0x2a,ZERO  ;保留
VECTOR 0x2b,ZERO  ;保留
VECTOR 0x2c,ZERO  ;ps/2鼠标
VECTOR 0x2d,ZERO  ;fpu浮点单元异常
VECTOR 0x2e,ZERO  ;硬盘中断(主盘)
VECTOR 0x2f,ZERO  ;硬盘中断(从盘)






;                      0x80号中断                      ;
;遇到syscall就来执行这段函数
;这是一段通用的函数,无论出发了0x80的哪个子功能都得经过这里
[bits 32]
extern syscall_table ;数组声明在syscall-init.c

section .text
global syscall_handler ;让外部可调用
syscall_handler:
    ;1、保存上下文环境
    push 0
    push ds
    push es
    push fs
    push gs
    pushad
    push 0x80 ;保持格式统一
    ;因为intr_exit有处理错误码和中断向量号


    ;2、为系统调用子功能传入参数
    ;统一压入三个参数也不会有问题
    push edx ;第三个参数
    push ecx ;第二个参数
    push ebx ;第一个参数

    ;3、调用子功能处理函数
    call [syscall_table + eax * 4]
    add esp,12 ;跨过上面的三个参数,因为上面的call函数执行完,这三个参数就没有用了

    ;4、将call调用后的返回值存入当前内核栈中eax的位置
    mov [esp + 8 * 4],eax   ;[esp + 8 * 4]是保存在栈里的eax
    jmp intr_exit  ;使用intr_exit返回,恢复上下文