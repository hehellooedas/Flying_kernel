%include "boot.inc"
SECTION loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR

;jmp loader_start 由mbr跳转


[bits 16]  ;编译成16位的机器码(默认)
;把描述符放在loader.S的开头,方便后期找到位置并增改数据
GDT_BASE: ;空描述符
    dd 0x00000000
    dd 0x00000000

CODE_DESC: ;代码段描述符(4GB)
    dd 0x0000FFFF
    dd DESC_CODE_HIGH4

DATA_STACK_DESC: ;数据，栈段描述符(4GB)
    dd 0x0000FFFF
    dd DESC_DATA_HIGH4

VIDEO_DESC: ;显示描述符(8*4KB=32KB)
    dd 0x80000007      ;基地址为0xb8000
    dd DESC_VIDEO_HIGH4


;定义GDT的尺寸(GDT_LIMIT)
GDT_SIZE    equ    $-GDT_BASE
GDT_LIMIT   equ    GDT_SIZE-1


times 60 dq 0 ;预留60个描述符的空位
    ;在tss.c中会定义以下新的描述符:
        ;TSS描述符
        ;RPL3的代码段描述符
        ;RPL3的数据段描述符


;以下是RPL0选择子/TI=0则选择GDT/第0个描述符不可使用
SELECTOR_CODE    equ    (0x0001 << 3) + TI_GDT  + RPL0   ;代码段选择子
SELECTOR_DATA    equ    (0x0002 << 3) + TI_GDT  + RPL0   ;数据段选择子
SELECTOR_VIDEO   equ    (0x0003 << 3) + TI_GDT  + RPL0   ;显示段选择子



;进入保护模式之前用中断统计内存容量
;保存内存容量,位于物理内存0xb00处.
;便于后面从这个内存地址获取该值(xp 0xb00)
;0xb00=0x900+0x200    (60+4)*8=0x200
total_mem_bytes dd 0 


;gdt指针,后面被修改
gdt_ptr:
    dw GDT_LIMIT ;2字节的偏移量
    dd GDT_BASE  ;4字节的基址


;预留ARDS的位置(获取内存容量)
ards_buf times 244 db 0     ;手动对齐
ards_nr dw 0                ;记录数量

loader_start:
    ;call show_bootloader    ;输出bootloader提示信息
    
    xor ebx,ebx             ;第一次调用时ebx要设置为0
    mov edx,0x534d4150      ;固定值
    mov di,ards_buf         ;ARDS结构要保存的位置


;第一种方法获取内存的循环
.e820_mem_get_loop:
    mov eax,0xE820
    mov ecx,20              ;ADRS结构的大小(固定值)
    int 0x15
    jc .e820_failed_so_try_e801      ;CF=1说明第一种方式行不通
    add di,cx               ;下一个位置
    inc word [ards_nr]      ;数量+1
    cmp ebx,0               ;EBX=0是代表着是最后一个了
    jnz .e820_mem_get_loop  ;不是最后一个那么就要循环

    ;已经是最后一个了，遍历所有ARDS结构找最大值
    mov cx,[ards_nr]        ;设置循环次数，有几个结构就循环几次
    mov ebx,ards_buf        ;指定结构们的起始位置
    xor edx,edx             ;edx记录的是最大值
.find_max_mem_area:         ;无需判断type字段,最大的内存块一定是可使用的
    mov eax,[ebx]           ;内存块的起始地址
                            ;由于内存是连续的,所以这个地址之前的地址都是存在的
    add eax,[ebx+8]         ;基地址+内存块的大小 (该ARDS获取到的内存大小)
    add ebx,20              ;轮到下一个结构
    cmp edx,eax             ;edx为到目前为止找到的最大的内存大小
;冒泡排序，找出最大并放到edx
    jge .next_ards          ;如果没有比edx更大，那么就去下一个
    mov edx,eax             ;如果比edx更大了，那就把edx设置成当前找到的最大值
.next_ards:
    loop .find_max_mem_area ;循环次数是固定的,完成后跳转即可
    jmp .mem_get_ok         ;找完收工



;第二种方法去找(最大只能识别4GB内存)
.e820_failed_so_try_e801:
    mov ax,0xe801
    int 0x15
    jc .e801_failed_so_try88   ;CF=1说明第二种方法行不通

;第二种方法成功，开始计算
    ;1、先算出低15MB的内存容量
    ;AX/CX中为1KB为单位的内存数量
    mov cx,0x400
    mul cx
    shl edx,16
    and eax,0x0000ffff
    or edx,eax
    add edx,0x100000
    mov esi,edx

    ;2、再算出16MB以上的内存容量
    ;BX/DX中为64KB为单位的内存数量
    xor eax,eax
    mov ax,bx
    mov ecx,0x10000      ;0x1000为64KB
    mul ecx
    add esi,eax
    mov edx,esi          ;把结果放到edx
    jmp .mem_get_ok      ;完事收工



;第三种方法(最大只能识别64MB内存)
;返回AX,以1KB(0x400)为单位大小
.e801_failed_so_try88:
    mov ah,0x88
    int 0x15
    jc .error_hlt        ;CF=1说明第三种方法行不通
    and eax,0x0000ffff

    mov cx,0x400
    mul cx
    shl edx,16           ;把dx移动到eax中的高16位
    or edx,eax           ;再与低16位拼接
    add edx,0x100000     ;最后再加上1MB就是结果
    jmp .mem_get_ok


.error_hlt:    ;出错就悬停
    mov ax,0
    mov ds,ax
    mov si,.error_message
    mov ax,0x0b800
    mov es,ax
    mov di,0
    mov cx,26   ;一共26个字符
    cld
    .movsb_loop:
        movsb   ;移动一次
        inc di
        loop .movsb_loop
    cli
    hlt
.error_message db 'Error! The loader is halt.'   

.mem_get_ok:
    mov [total_mem_bytes],edx   ;把最终的结果给记录下来


    ;打开A20,关闭地址回绕
    in al,0x92
    or al,0000_0010B
    out 0x92,al

    ;加载GDT
    lgdt [gdt_ptr]

    ;设置cr0
    ;cr0的第一位是PE位
    ;PE=0实模式/PE=1保护模式
    mov eax,cr0
    or eax,0x00000001
    mov cr0,eax

    jmp dword SELECTOR_CODE:p_mode_start  ;跳入保护模式
    ;使用远跳转刷新流水线,更新段描述符缓冲寄存器



[bits 32]  ;编译成32位的机器码
p_mode_start: ;protect mode开始
    mov ax,SELECTOR_DATA ;各个段寄存器都是数据段
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov esp,LOADER_STACK_TOP
    mov ax,SELECTOR_VIDEO
    mov gs,ax
    mov byte [gs:160],'P'
 


;加载kernel.bin
    mov eax,KERNEL_START_SECTOR     ;在硬盘中的位置
    mov ebx,KERNEL_BIN_BASE_ADDR    ;将要被放置到内存中的位置
    mov ecx,200                     ;读取200个扇区，一次性200个以后就不要改了
    call rd_disk_m_32               ;读取硬盘扇区的数据


;开始开启分页模式
    call setup_page
    sgdt [gdt_ptr]         ;把gdt里的内容放回到内存

    mov ebx,[gdt_ptr + 2]  ;获取全局描述符的基地址
    or dword [ebx + 0x18 + 4],0xc0000000   ;视频段映射到内核地址
    add dword [gdt_ptr + 2],0xc0000000     ;段描述符的基地址映射到内核中
    add esp,0xc0000000     ;栈指针也映射到内核地址

;页目录地址给cr3
    mov eax,PAGE_DIR_TABLE_POS
    mov cr3,eax

;通知cr0已打开分页模式
    mov eax,cr0
    or eax,0x80000000 ;打开cr0的pg位(第32位)
    mov cr0,eax

    lgdt [gdt_ptr]    ;再次设置gdt
    mov byte [gs:160],'V'


    jmp SELECTOR_CODE:enter_kernel ;强制刷新流水线，以防万一
                                   ;代码段选择子:地址标号

enter_kernel:
    call kernel_init        ;初始化内核
    mov esp,0xc009f000      ;设置栈顶位置,将来主线程的PCB地址,因为线程顶端就是栈
    jmp KERNEL_ENTRY_POINT  ;跳入内核，启动引导的任务结束




;以下是函数定义

;将kernel.bin中的segment拷贝到编译的地址
;通过ELF_header找到程序头表的地址,然后遍历所有程序头,一个程序头代表一个段(程序头的目的就是描述这些段)
;
kernel_init:
    xor eax,eax
    xor ebx,ebx   ;ebx记录程序头表地址
    xor ecx,ecx   ;cx记录程序头表的program header数量
    xor edx,edx   ;dx记录program header尺寸,每一项的大小，即e_phentsize

    mov dx,[KERNEL_BIN_BASE_ADDR + 42]  ;e_phentsize:头程序表每个条目的字节大小
    mov ebx,[KERNEL_BIN_BASE_ADDR + 28] ;e_phoff:程序头表在文件内的字节偏移量

    add ebx,KERNEL_BIN_BASE_ADDR        ;ELF_header+e_phoff=程序头表的位置
    mov cx,[KERNEL_BIN_BASE_ADDR + 44]  ;e_phnum:程序头表中条目的数量(段的个数)也就是循环次数
    
.each_segment:   ;判断每一个段
    cmp byte [ebx + 0],PT_NULL   ;若相等则该程序头表没有使用,程序头表第一项是p_type;0代表PT_NULL
    je .PTNULL                   ;如果是空段则跳过去不处理

    push dword [ebx + 16]        ;参数3：本段在文件中的大小,p_filesz

    mov eax,[ebx + 4]            ;本段在文件内的起始偏移字节,p_offset
    add eax,KERNEL_BIN_BASE_ADDR ;ELF_header+p_offset=节头表的位置

    push eax                     ;参数2：源地址 
    push dword [ebx + 8]         ;参数1：目的地址,p_vaddr(在链接的时候确定)
    
    call mem_cpy
    add esp,12             ;清理之前push的三个参数
.PTNULL:
    add ebx,edx            ;edx为程序头表中每个项目的大小(e_phentsize),加上去ebx就知道下一个项目了
    loop .each_segment     ;遍历所有程序头
    ret

mem_cpy:
;逐字节拷贝，参数在栈里(dst,src,size)
;copy from src to dst.
    cld
    push ebp
    mov ebp,esp
    push ecx
    mov edi,[ebp + 8]    ;dst
    mov esi,[ebp + 12]   ;src
    mov ecx,[ebp + 16]   ;size:尺寸有多大就移动几次,每次移动一个字节
    ;正向移动：[esi] -> [edi]
    rep movsb ;逐字节拷贝

    ;还原寄存器
    pop ecx
    pop ebp
    ret




setup_page:        ;设置页目录表和页表
    mov ecx,4096   ;为循环作准备
    mov esi,0
.clear_page_dir:   ;页目录表数据清零
    mov byte [PAGE_DIR_TABLE_POS + esi],0
    inc esi
    loop .clear_page_dir
.create_pde:       ;开始创建页目录项(PDE)
    mov eax,PAGE_DIR_TABLE_POS
    add eax,0x1000
    mov ebx,eax

    or eax,PG_US_U | PG_RW_W | PG_P        ;高位是地址，低位是属性
    mov [PAGE_DIR_TABLE_POS + 0x0],eax     ;页目录表第一项指向页表的第一项
                                           ;页表第一项指向物理地址最低的4MB
    mov [PAGE_DIR_TABLE_POS + 0xc00],eax   ;页目录表的第768项指向页表的第一项

    sub eax,0x1000
    mov [PAGE_DIR_TABLE_POS + 4092],eax    ;页目录表最后一项指向页目录第一项
                                           ;方便后期动态操作页表
;为创建PTD作准备
    mov ecx,256           ;1MB/4KB=256.循环256次则刚好把低端1MB放到页表里
    mov esi,0
    mov edx,PG_US_U | PG_RW_W | PG_P

.create_pte:              ;开始创建页表项(PTE)
    mov [ebx+esi*4],edx   ;ebx为页表的物理地址
    add edx,0x1000        ;跳到下一个物理地址
    inc esi               ;下一个页表项
    loop .create_pte

;创建内核其它页表的PTE
    mov eax,PAGE_DIR_TABLE_POS
    add eax,0x2000
    or eax,PG_US_U | PG_RW_W | PG_P
    mov ebx,PAGE_DIR_TABLE_POS
    mov ecx,254           ;768+254=1022 / 第1023项已经设置好了
    mov esi,769           ;768项已经设置好了,从第769项开始
.create_kernel_pde:       ;768～1022页目录项都指向页表(从页表的最低端开始)
    mov [ebx+esi*4],eax
    inc esi
    add eax,0x1000
    loop .create_kernel_pde
    ret



;读取硬盘第n个扇区 input:eax,ebx,ecx
;eax:源地址（硬盘），ebx目的地址（内存），ecx读取几个扇区(当前为200)
rd_disk_m_32:
    mov esi,eax;备份eax
    mov edi,ecx;备份ecx

;读写硬盘
;第一步：设置要读取的扇区数
    mov dx,0x1f2 ;这也是一个8位的端口,0~255
    mov eax,ecx
    out dx,al
    mov eax,esi;还原eax

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
    and al,0x88
    cmp al,0x08
    jnz .not_ready

;第五步：从0x1f0端口读数据
    mov eax,edi ;edi为要读取的扇区数
    mov edx,256
    mul edx
    mov ecx,eax ;ecx规定了循环的次数，扇区数x256次(每次读取两个字节)

    mov dx,0x1f0

.go_on_read:
    in ax,dx
    mov [ebx],ax
    add ebx,2
    loop .go_on_read
    ret


show_bootloader:
    mov ax,0
    mov ds,ax
    mov si,.bootloader_message

    mov ax,0x0b800
    mov es,ax
    mov di,0
    
    mov cx,24
    cld
    .print_loop:
        movsb
        inc di
        loop .print_loop
    mov si,0
    mov di,0
    ret
.bootloader_message db 'bootloader is loading...'
