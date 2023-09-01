[bits 32]
section .text


; c语言调用该函数 switch_to(cur, next);
; 保护cur线程的寄存器映像,将下一个线程next的映像装载到处理器
global switch_to
switch_to:
    ;在栈中保存cur的寄存器映像
    push esi
    push edi
    push ebx
    push ebp

    mov eax,[ebp + 20] ;得到栈中的参数cur,绕过前面4个push,直接获取到cur这个参数,即eax=cur
    mov [eax],esp ;保存当前栈顶指针esp,到PCB的self_kstart字段,即cur->self_kstack=esp
                  ;self_ksatrt在task_stack中的偏移量为0
                  ;所以直接往thread开头存4 Bytes就好  


    ;上面和下面的代码作用不同
    ;以上是备份当前线程的环境,下面是恢复下一个线程的环境



    mov eax,[esp + 24] ;得到栈中的参数next,也就是next线程的pcb

    mov esp,[eax] ;下一个线程的栈地址写进esp寄存器里
                  ;这里对eax进行寻址,相当于esp = [eax] = next->self_kstack

    ;由于esp已经改变了,所以这里不是恢复到上面保存的环境
    ;而是恢复到next中存储的寄存器映像
    pop ebp
    pop ebx
    pop edi
    pop esi
    ret ;最后用ret跳到下一个任务,已经不会回到原来的函数了
        ;未由中断进入,第一次执行时会返回到kernel_thread