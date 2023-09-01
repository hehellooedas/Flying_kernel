#include <keyboard.h>
#include <print.h>
#include <interrupt.h>
#include <io.h>
#include <global.h>


ioqueue kbd_buf; //定义键盘缓冲区
#define KDB_BUF_PORT 0x60 //键盘buffer寄存器的端口号

uint8_t color = default_color;

/*用转义字符定义部分控制字符*/
#define esc       '\033'      //ESC键
#define backspace '\b'        //退格键
#define tab       '\t'        //制表符
#define enter     '\r'        //回车键
#define delete    '\177'      //删除键


/*不可见字符定义为0*/
#define char_invisiable 0
#define ctrl_l_char char_invisiable
#define ctrl_r_char char_invisiable
#define shift_l_char char_invisiable
#define shift_r_char char_invisiable
#define alt_l_char char_invisiable
#define alt_r_char char_invisiable
#define caps_lock_char char_invisiable


/*定义控制字符的通码和断码*/
#define shift_l_make 0x2a
#define shift_r_make 0x36
#define alt_l_make 0x38
#define alt_r_make 0xe038
#define alt_r_break 0xe0b8
#define ctrl_l_make 0x1d
#define ctrl_r_make 0xe01d
#define ctrl_r_break 0xe09d
#define caps_lock_make 0x3a


/*  定义一下变量记录相应按键是否处于按下的状态  */
static bool ctrl_status,alt_status,shift_status,caps_lock_status,ext_scancode;


/*以通码make_code为索引的二维数组*/
static char keymap[][2] = {
/*  0x00  */    {0, 0},
/*  0x01  */    {esc,   esc},
/*  0x02  */    {'1',   '!'},
/*  0x03  */    {'2',   '@'},
/*  0x04  */    {'3',   '#'},
/*  0x05  */    {'4',   '$'},
/*  0x06  */    {'5',   '%'},
/*  0x07  */    {'6',   '^'},
/*  0x08  */    {'7',   '&'},
/*  0x09  */    {'8',   '*'},
/*  0x0a  */    {'9',   '('},
/*  0x0b  */    {'0',   ')'},
/*  0x0c  */    {'-',   '_'},
/*  0x0d  */    {'+', '+'},
/*  0x0e  */    {backspace, backspace},
/*  0x0f  */    {tab,   tab},
/*  0x10  */    {'q',   'Q'},
/*  0x11  */    {'w',   'W'},
/*  0x12  */    {'e',   'E'},
/*  0x13  */    {'r',   'R'},
/*  0x14  */    {'t',   'T'},
/*  0x15  */    {'y',   'Y'},
/*  0x16  */    {'u',   'U'},
/*  0x17  */    {'i',   'I'},
/*  0x18  */    {'o',   'O'},
/*  0x19  */    {'p',   'P'},
/*  0x1a  */    {'[',   '{'},
/*  0x1b  */    {']',   '}'},
/*  0x1c  */    {enter, enter},
/*  0x1d  */    {ctrl_l_char,   ctrl_l_char},
/*  0x1e  */    {'a',   'A'},
/*  0x1f  */    {'s',   'S'},
/*  0x20  */    {'d',   'D'},
/*  0x21  */    {'f',   'F'},
/*  0x22  */    {'g',   'G'},
/*  0x23  */    {'h',   'H'},
/*  0x24  */    {'j',   'J'},
/*  0x25  */    {'k',   'K'},
/*  0x26  */    {'l',   'L'},
/*  0x27  */    {';',   ':'},
/*  0x28  */    {'\'',  '\"'},
/*  0x29  */    {'`',   '~'},
/*  0x2a  */    {shift_l_char,  shift_l_char},
/*  0x2b  */    {'\\',  '|'},
/*  0x2c  */    {'z',   'Z'},
/*  0x2d  */    {'x',   'X'},
/*  0x2e  */    {'c',   'C'},
/*  0x2f  */    {'v',   'V'},
/*  0x30  */    {'b',   'B'},
/*  0x31  */    {'n',   'N'},
/*  0x32  */    {'m',   'M'},
/*  0x33  */    {',',   '<'},
/*  0x34  */    {'.',   '>'},
/*  0x35  */    {'/',   '?'},
/*  0x36  */    {shift_r_char,  shift_r_char},
/*  0x37  */    {'*',   '*'},
/*  0x38  */    {alt_r_char,    alt_r_char},
/*  0x39  */    {' ',   ' '},
/*  0x3a  */    {caps_lock_char,    caps_lock_char}

/*  其他按键暂不处理  */
};



static void intr_keyboard_handler(void){

    /*这次中断发生前的上一次中断需要考虑进去*/
    bool ctrl_down_last = ctrl_status;   //前面是否按了ctrl
    bool shift_down_last = shift_status; //前面是否按了shift
    bool caps_lock_last = caps_lock_status; //前面是否按了caps
    bool break_code; //断码，按键弹起时的扫描码


    /*必须读取输出缓冲区寄存器，否则8042不再继续响应键盘中断*/
    uint16_t scancode = inb(KDB_BUF_PORT); //从该端口中读取
    /*如果是0xe0,直接读取下一个扫描码,进行下一次判断*/
    if(scancode == 0xe0){
        ext_scancode = true; //标记
        return;
    }

    /*如果上一次的扫描码是0xe0,将上一次的扫描码与这一次的合并*/
    if(ext_scancode){
        scancode = ((0xe000) | scancode); //把0xe0放到前面
        ext_scancode = false; //结束标记
    }

    break_code = ((scancode & 0x0080) != 0); //判断是否有断码的存在
    if(break_code){ //如果是断码(按键弹起的时候响应)
        uint16_t make_code = (scancode &= 0xff7f); //0xff7f刚好第8位是0
        /*出现断码说明按键已经结束了，所以把状态设置成false*/
        if(make_code == ctrl_l_make || make_code == ctrl_r_make) ctrl_status = false;
        else if(make_code == shift_l_make || make_code == shift_r_make) shift_status = false;
        else if(make_code == alt_l_make || make_code == alt_r_make) alt_status = false;
        /*caps_lock不是弹起的时候关闭，所以单独处理*/
        return;
    }
    else if((scancode > 0x00 && scancode < 0x3b) || \
            (scancode == ctrl_r_make) || (scancode == alt_r_make)

    ){
        bool shift = false; //判断是否与shift组合
        if((scancode < 0x0e) || (scancode == 0x29)|| \
           (scancode == 0x1a) || (scancode == 0x1b) || \
           (scancode == 0x2b) || (scancode == 0x27) || \
           (scancode == 0x28) || (scancode == 0x33) || \
           (scancode == 0x34) || (scancode == 0x35)
        ){
            if(shift_down_last) shift = true;
        }else{
        if(shift_down_last && caps_lock_last) shift = false; //如果shift和caps同时按下，又会变成小写
        else if(shift_down_last || caps_lock_last) shift = true; //二者只按下一个就是大写
        else shift = false;
        }

        uint8_t index = (scancode &= 0x00ff);
        char cur_char = keymap[index][shift]; //shift为true则为第二项
        if(cur_char){ //如果是可打印的字符
            if(!ioq_full(&kbd_buf)){
                put_char_color(cur_char,color);
                //if(color == 0b00001111) color = 0b00000001;
                ioq_putchar(&kbd_buf,cur_char);
            }
            return;
        }
        /*  如果是不可打印的字符  */
        /*记录本次是否按下了下列积累控制键，为下一次处理做铺垫*/
        if(scancode == ctrl_l_make || scancode == ctrl_r_make) ctrl_status = true;
        else if(scancode == alt_l_make|| scancode == alt_r_make) alt_status = true;
        else if(scancode == shift_l_make || scancode == shift_r_make) shift_status = true;
        else if(scancode == caps_lock_make) caps_lock_status = !caps_lock_status;
    }else{
        put_str("unknow key\n");
    }
}



/*  键盘初始化  */
void keyboard_init(void){
    put_str("keyboard_init start\n");
    ioqueue_init(&kbd_buf);
    register_handler(0x21, intr_keyboard_handler);
    put_str("keyboard_init done\n");
}

