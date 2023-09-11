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
                  ;self_ksatrt在task_stack中的偏移量为0,esp寄存器也恰好是4个字节
                  ;等到下一次被调度到的时候在把self_kstart写回到esp,于是任务的栈信息也被还原了


    ;以上是备份当前线程(旧)的环境,下面是恢复下一个线程(新)的环境



    mov eax,[esp + 24] ;得到栈中的参数next,也就是next线程的pcb

    mov esp,[eax] ;下一个线程的栈地址写进esp寄存器里
                  ;这里对eax进行寻址,相当于esp = [eax] = next->self_kstack
                  ;self_kstack指向线程栈thread_stack

    ;由于esp已经改变了,所以这里不是恢复到上面保存的环境
    ;而是恢复到next中存储的寄存器映像
    pop ebp
    pop ebx
    pop edi
    pop esi

    ;thread_stack开头恰好是这四个寄存器
    ;当我们绕过这四个寄存器,接下来的就是eip

    ret ;最后用ret跳到下一个任务,已经不会回到原来的函数了
        ;因为ret是从esp中找到返回到函数的路,而esp刚刚进行修改
        ;未由中断进入,第一次执行时会返回到kernel_thread
        ;因为如果一个任务是第一次调度到的话,mov [eax],esp是没有这一步的
        ;如果一个任务不是第一次被调度到,则在进入switch_to.S的时候会自然地把eip压入栈中
        ;通过这里的eip返回到schedule()再返回到时钟中断再返回到中断处理函数里intr_exit退出时钟中断
        ;此时任务就返回到了触发时钟中断之前的地址,继续执行