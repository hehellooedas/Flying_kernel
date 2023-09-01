;宏定义
TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0 ;视频段选择子，和前面一样 

[bits 32]
section .data ;数据区
put_int_buffer dq 0 ;定义8字节的字符缓冲区，用于把数字转换成字符


section .text ;代码段

;打印字符串函数
global put_str
put_str:
    push ebx
    push ecx
    xor ecx,ecx
    mov ebx,[esp + 12] ;c语言进入函数前把头指针放到了栈里
.goon:
    mov cl,[ebx]       ;此时，ebx是头指针
    cmp cl,0           ;如果要打印的字符是'\0',说明已经到头了
    jz .str_over
    push ecx           ;把要打印的字符以栈的形式传递到函数
    call put_char      ;打印一个字符
    add esp,4          ;回收push ecx产生的空间
    inc ebx            ;寻找下一个字符
    jmp .goon

.str_over:
    pop ecx
    pop ebx
    ret



;打印单个字符，是其它打印函数的基础
global put_char ;声明为全局函数方便c调用
put_char:
    pushad      ;备份32环境下的寄存器
    mov ax,SELECTOR_VIDEO
    mov gs,ax   ;把显示段放入gs段寄存器

;获取当前光标的位置 0x03d4里放索引;0x03d5用来读写
    ;先获取高8位 索引：0x0e
    mov dx,0x03d4
    mov al,0x0e
    out dx,al
    mov dx,0x03d5
    in al,dx
    mov ah,al  ;高8位所以放在ah

    ;再获取低8位 索引：0x0f
    mov dx,0x03d4
    mov al,0x0f
    out dx,al
    mov dx,0x03d5
    in al,dx
    mov bx,ax   ;把刚获取到的当前光标的位置放到bx里

    ;去栈中找出待打印的字符
    mov ecx,[esp + 36] ;8x4+4=36
                       ;跳过pushad保存的8个寄存器和call保存的eip(返回地址)
    ;由于输入的是uint8_t类型的数据
    ;所以只需要判断ecx中的cl即可(1B)


    ;不可见字符的处理
    cmp cl,0xd     ;CR换行
    jz .is_carriage_return 

    cmp cl,0xa     ;LF回车 换行和回车最终都跳到同一个地方
    jz .is_line_feed

    cmp cl,0x8     ;退格键
    jz .is_backspace

    jmp .put_other ;可见字符

.is_backspace:
    dec bx         ;当前光标的位置-1
    shl bx,1       ;左移1位=乘以2(字符的位置都是偶数)
    mov byte [gs:bx],0x20 ;空格
    inc bx
    mov byte [gs:bx],0x07 ;设置默认颜色
    shr bx,1       ;再变回去
    jmp .set_cursor

.put_other:
    shl bx,1
    mov [gs:bx],cl
    inc bx
    mov byte [gs:bx],0x07  
    shr bx,1       ;变回去
    inc bx         ;光标移动到下一个位置
    cmp bx,2000    ;判断当前光标是否在屏幕范围之内
    jl .set_cursor ;小于2000,没有到屏幕外面，直接设置


;`linux中\r与\n都是\n的意思
.is_line_feed:       ;换行(\n)
.is_carriage_return: ;回车(\r) 
    xor dx,dx        ;被除数的高16位，清零
    mov ax,bx        ;被除数的低16位
    mov si,80        ;一行80个字符
                     ;AX:在第几行
    div si           ;DX:在该行的第几个位置
    sub bx,dx        ;让bx位置在行首


.is_carriage_return_end:
    add bx,80       ;跳到下一行
    cmp bx,2000     ;判断是否需要滚屏
.is_line_feed_end:
    jl .set_cursor  ;bx的位置小于2000则不需要滚屏
    ;需要滚屏则继续执行下面的滚屏函数




;滚屏函数,把整个屏幕往上推(bx>2000)
;屏幕行的范围:0~24
;滚屏的原理：把1～24移动到0~23
.roll_screen:
    cld                  ;设置方向位DF=0,顺序移动
    mov ecx,960          ;(2000-80)*2/4=960
    ;一共要移动1920个字符.每个字符两个字节,movsd一次移动4个字节
    mov esi,0xc00b80a0   ;第1行
    mov edi,0xc00b8000   ;第0行 差值为1行(160)
    rep movsd            ;开始移动

    ;最后一行用空格填充
    mov ebx,3840         ;最后一行行首的位置
    mov ecx,80           ;每行80个字符

.cls:
    mov word [gs:ebx],0x0720 ;0x0720是黑底白字的空格
    add ebx,2
    loop .cls
    mov bx,1920  ;光标设置在最后一行第一个字符

;设置光标位置的函数
;需要一个参数:bx光标位置
.set_cursor:
    ;先设置高8位
    mov dx,0x03d4
    mov al,0x0e
    out dx,al
    mov dx,0x3d5
    mov al,bh
    out dx,al

    ;再设置低8位
    mov dx,0x03d4
    mov al,0x0f
    out dx,al
    mov dx,0x03d5
    mov al,bl
    out dx,al

;put_char函数最终结束了
    .put_char_done:
    popad  ;恢复环境
    ret




global put_int
put_int: ;栈中存储要打印的数字
    pushad
    mov ebp,esp
    mov eax,[ebp + 4*9]
    mov edx,eax  ;edx和eax中存储要打印的数字的头指针
    mov edi,7    ;指定在put_int_buffer的初始偏移量(0~7)
    mov ecx,8    ;要处理8次(固定的处理次数,去0由后面来解决)
    mov ebx,put_int_buffer   ;ebx指向缓冲区

.16based_4bits:
    and edx,0x0000000F ;处理最后4位二进制数（一个字节）
    cmp edx,9          ;数字和字符需要分开处理
    jg .is_A2F         ;处理字符(ASCII)
    add edx,'0'        ;如果是数字则只要加上'0'即可
    jmp .store         ;存储到缓冲区中(由于直接跳转了则edi不变)

.is_A2F:
    sub edx,10 
    add edx,'A'        ;减去偏移量再加上头字母的acsii

.store: ;低位放在高地址
    mov [ebx+edi],dl   ;dl中存储的就是该数字16进制字符对应的ASCII
    dec edi            ;从右往左记录
    shr eax,4          ;处理下一个4位
    mov edx,eax        ;放到edx来处理，eax保存这个数剩下要处理的部分
    loop .16based_4bits ;进行下一次处理

.ready_to_print:
    inc edi      ;edi多减了一次
.skip_prefix_0:  ;跳过前面的0
    cmp edi,8    ;如果edi是8，则出现了8个0
                 ;edi=出现0的个数
    je .full0    ;如果全是0的情况

.go_on_skip:
    mov cl,[put_int_buffer + edi]
    inc edi
    cmp cl,'0'
    je .skip_prefix_0  ;如果该字符是'0',则不用执行下面的dec edi，意味着edi加一
    dec edi            ;如果该字符不是'0'，则减回去
    jmp .put_each_num


.full0:
    mov cl,'0'

.put_each_num:   ;把数字转成的字符打印出来,调用put_char
    push ecx
    call put_char
    add esp,4
    inc edi           ;下一个要打印的字符的索引
    mov cl,[put_int_buffer+edi]
    cmp edi,8         ;等于8的时候意味着打印结束了，不要再跳回去继续打印了
    jl .put_each_num  ;edi小于8时，得继续打印
    popad ;恢复环境
    ret



global set_cursor
set_cursor:
    pushad
    mov bx,[esp+36]   ;获取待设置的光标的位置

    ;先设置高8位
    mov dx,0x03d4
    mov al,0x0e
    out dx,al
    mov dx,0x03d5
    mov al,bh
    out dx,al

    ;再设置低8位
    mov dx,0x03d4
    mov al,0x0f
    out dx,al
    mov dx,0x03d5
    mov al,bl
    out dx,al

    ;函数结束
    popad
    ret







;打印带颜色的字符串
global put_str_color
put_str_color:
    push ebx
    push ecx
    
    mov ecx,[esp + 16] ;获取颜色信息
    mov ebx,[esp + 12] ;c语言进入函数前把头指针放到了栈里
    push ecx
    xor ecx,ecx
.goon:
    mov cl,[ebx] ;此时，ebx是头指针
    cmp cl,0 ;如果要打印的字符是'\0',说明已经到头了
    jz .str_over
    
    push ecx ;把要打印的字符以栈的形式传递到函数
    call put_char_color ;打印一个字符
    add esp,4 ;回收push ecx产生的空间
    inc ebx ;寻找下一个字符
    jmp .goon

.str_over:
    add esp,4 ;彩色打印函数执行完成后,颜色就信息无效了
    pop ecx
    pop ebx
    ret



;打印带颜色的字符
global put_char_color
put_char_color:
    pushad
    mov ax,SELECTOR_VIDEO
    mov gs,ax
    mov dx,0x03d4
    mov al,0x0e
    out dx,al
    mov dx,0x03d5
    in al,dx
    mov ah,al
    mov dx,0x03d4
    mov al,0x0f
    out dx,al
    mov dx,0x03d5
    in al,dx
    mov bx,ax
    mov ecx,[esp + 36] ;8x4+4=36
    mov edx,[esp + 40]
    mov ch,dl
    cmp cl,0xd
    jz .is_carriage_return 
    cmp cl,0xa 
    jz .is_line_feed
    cmp cl,0x8
    jz .is_backspace
    jmp .put_other
.is_backspace:
    dec bx
    shl bx,1
    mov byte [gs:bx],0x20
    inc bx
    mov byte [gs:bx],0x07
    shr bx,1
    jmp .set_cursor
.put_other:
    shl bx,1
    mov [gs:bx],cl
    inc bx
    mov byte [gs:bx],dl
    shr bx,1
    inc bx
    cmp bx,2000
    jl .set_cursor
.is_line_feed:
.is_carriage_return:
    xor dx,dx
    mov ax,bx
    mov si,80
    div si
    sub bx,dx
.is_carriage_return_end:
    add bx,80
    cmp bx,2000
.is_line_feed_end:
    jl .set_cursor
.roll_screen:
    cld
    mov ecx,960 ;(2000-80)*2/4=960
    mov esi,0xc00b80a0
    mov edi,0xc00b8000
    rep movsd
    mov ebx,3840
    mov ecx,80
.cls:
    mov word [gs:ebx],0x0720
    add ebx,2
    loop .cls
    mov bx,1920
.set_cursor:
    mov dx,0x03d4
    mov al,0x0e
    out dx,al
    mov dx,0x3d5
    mov al,bh
    out dx,al
    mov dx,0x03d4
    mov al,0x0f
    out dx,al
    mov dx,0x03d5
    mov al,bl
    out dx,al
    .put_char_done:
    popad
    ret