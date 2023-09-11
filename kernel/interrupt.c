#include <interrupt.h>
#include <global.h>
#include <io.h>
#include <print.h>
#include <stdint.h>



#define EFLAGS_IF    0x00000200  //IF位的位置

/*  获取eflags  */
#define GET_EFLAGS(EFLAG_VAR)  asm volatile ("pushfl;popl %0":"=g"(EFLAG_VAR))  


#define PIC_M_CTRL      0x20    //主片的控制端口(master)
#define PIC_M_DATA      0x21    //主片的数据端口
#define PIC_S_CTRL      0xA0    //从片的控制端口(slave)
#define PIC_S_DATA      0xA1    //从片的数据端口


#define IDT_DESC_CNT    0x81    //中断描述符(IDT)的个数
#define lastindex       0x80    //最后一个IDE的索引

extern uint32_t syscall_handler(void); //在kernel.S里


/*  中断门描述符结构体  */
typedef struct {
    /*低32位*/
    uint16_t func_offset_low_word; //低偏移量
    uint16_t selector;             //选择子

    /*高32位*/
    uint8_t dcount;     //是固定值(0)，表示这是中断门描述符
    uint8_t attribute;  //属性
    uint16_t func_offset_high_word; //高偏移量
}gate_desc;

static void make_idt_desc(gate_desc *p_gdesc,uint8_t attribute,intr_handler function); //制作中断描述符的函数

static gate_desc idt[IDT_DESC_CNT];    //中断描述门数组
extern intr_handler intr_entry_table[IDT_DESC_CNT]; //中断处理函数的入口地址表,已经在kernel.S中定义

char *intr_name[IDT_DESC_CNT];        //中断的名称
intr_handler idt_table[IDT_DESC_CNT]; //指向中断函数



/*初始化中断控制器8259A*/
static void pic_init(void){
    /*初始化主片*/
    outb(PIC_M_CTRL,0x11); //ICW1
    outb(PIC_M_DATA,0x20); //ICW2
    outb(PIC_M_DATA,0x04); //ICW3
    outb(PIC_M_DATA,0x01); //ICW4

    /*初始化从片*/
    outb(PIC_S_CTRL,0x11); //ICW1
    outb(PIC_S_DATA,0x28); //ICW2
    outb(PIC_S_DATA,0x02); //ICW3
    outb(PIC_S_DATA,0x01); //ICW4

    /*ICW初始化完成*/

    /*  打开IRQ0 IRQ1 IRQ2*/
    outb(PIC_M_DATA,0xf8);

    /*打开从片上的IRQ14,此引脚接受硬盘控制器的中断*/
    outb(PIC_S_DATA,0xbf);

    put_str("   pic_init done\n");
}




/*  
    制作中断门描述符
    p_gdesc是中断描述符表
    attribute是中断门描述符DPL
    function是中断处理函数的地址
*/
static void make_idt_desc(gate_desc *p_gdesc,uint8_t attribute,intr_handler function){
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF; //取出后16位
    p_gdesc->selector = SELECTOR_K_CODE;  //设置目标代码段描述符的选择子,就是全局代码段
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attribute;
    p_gdesc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16; //取出前16位的值
}


/*  初始化中断描述符  */
static void idt_desc_init(void){
    for(int i=0;i<IDT_DESC_CNT-1;i++){
        make_idt_desc(&idt[i],IDT_DESC_ATTR_DPL0,intr_entry_table[i]);
    }

    /*
      单独处理系统调用,对应的中断门为dpl3
      中断处理程序为单独的syscall_handler
    */
    make_idt_desc(&idt[lastindex], IDT_DESC_ATTR_DPL3, syscall_handler);
    put_str("   idt_desc_init done\n");
}



/*通用的中断处理函数,一般用于异常*/
static void general_intr_handler(uint8_t vec_nr){
    /*伪中断无需处理,IRQ7和IRQ15会产生伪中断*/
    if(vec_nr==0x27 || vec_nr==0x2f){
        return;
    }

    /*在屏幕的上方空出来一段空间展示异常信息*/
    set_cursor(0);
    int cursor_pos = 0;  //光标当前的位置
    while(LIKELY(cursor_pos < 320)){
        put_char(' ');
        cursor_pos++;
    }
    set_cursor(0);  //把光标位置放到最左上角
    put_str("!!!!!!!!   exception message begin    !!!!!!!!\n");
    set_cursor(88);
    put_str(intr_name[vec_nr]);

    /*page-fault缺页中断，将缺失的地址打印出来*/
    if(vec_nr == 14){
        int page_fault_vaddr = 0;
        asm("movl %%cr2,%0":"=r"(page_fault_vaddr)); //cr2寄存器含有导致页错误的线性地址
        put_str("\n     page fault addr is ");
        put_int(page_fault_vaddr);
    }
    
    put_str("\n!!!!!!!!    exception message end     !!!!!!!!");
    while(1); //出现重大错误的时候直接停止即可,不需要跳转回去了
}



/*
  提供注册中断的接口，以后在设备自己的文件里注册即可
  注册中断处理程序数组的第vector_no个元素 
*/
void register_handler(uint8_t vector_no,intr_handler function){
    idt_table[vector_no] = function;
}



/*  完成一般中断处理函数注册及异常名称注册  */
static void exception_init(void){
    for(int i=0;i<IDT_DESC_CNT;i++){
        intr_name[i] = "unknown";   //先统一命名
        idt_table[i] = general_intr_handler; //注册为通用的中断处理函数
    }

    
    /*  0～19的中断是固定的  */
    intr_name[0] = "#DE Divide Error";
    intr_name[1] = "#DB Debug Exxception";
    intr_name[2] = "NMI Interrupt";
    intr_name[3] = "#BP Breakpoint Exception"; //int3断点异常
    intr_name[4] = "#OF Overflow Exception";   //into溢出中断指令
    intr_name[5] = "#BR BOUND Range Exceeded Exception";
    intr_name[6] = "#UD Invalid Opcode Exception"; //UD异常可以主动触发
    intr_name[7] = "NM Device Not Avaliable Exception";
    intr_name[8] = "#DF Double Fault Exception";
    intr_name[9] = "Coprocessor Segment Overrun";
    intr_name[10] = "#TS Invalid TSS Exception";
    intr_name[11] = "#NP Segment NOt Present";
    intr_name[12] = "#SS Stack Fault Exception";
    intr_name[13] = "#GP General Protection Exception";
    intr_name[14] = "#PF Page-Fault Exception"; //缺页异常
    // intr_name[15]为intel保留
    intr_name[16] = "#MF x87 FPU Floating-Point Error";
    intr_name[17] = "#AC Alignment Check Exception";
    intr_name[18] = "#MC Machine-Check Exception";
    intr_name[19] = "#XF SIMD Floating-Point Exception";

    /*  以下是自定义中断  */
    intr_name[32] = "timer interrupt";    //时钟中断
    intr_name[33] = "keyboard interrupt"; //键盘中断 
}



void idt_init(void){
    put_str("idt_init start\n");
    idt_desc_init();                    //初始化中断描述符表
    exception_init();                   //初始化通用中断处理函数
    pic_init();                         //初始化8259A
    put_str("idt_init done\n");

    /*  
      加载IDT
      前16位是界限。后32位是基地址
    */
    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)(uint32_t)idt << 16));
    asm volatile ("lidt %0"::"m"(idt_operand));
}


/*----------------------------------------------------------*/


/*  打开中断并返回打开中断前的状态  */
intr_status intr_enable(void){
    intr_status old_status;
    if (INTR_ON == intr_get_status()) //如果已经打开了
    {
        old_status = INTR_ON; 
    }else{
        old_status = INTR_OFF;
        asm volatile ("sti");
    }
    return old_status;
}


/*  关闭中断并返回关闭中断前的状态  */
intr_status intr_disable(void){
    intr_status old_status;
    if(INTR_ON == intr_get_status()){ //如果之前是打开状态
        old_status = INTR_ON;
        asm volatile ("cli":::"memory"); //把中断关了
    }else{
        old_status = INTR_OFF;
    }
    return old_status;
}

/*  将中断设置为status  */ 
intr_status intr_set_status(intr_status status){
    return ((status == INTR_ON) ? intr_enable():intr_disable());
}


/*  获取当前中断状态  */
intr_status intr_get_status(void){
    uint32_t eflags;
    GET_EFLAGS(eflags);
    return (EFLAGS_IF & eflags) ? INTR_ON:INTR_OFF; 
}

