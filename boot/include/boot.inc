LOADER_BASE_ADDR      equ    0x900        ;loader在内存中的位置
LOADER_START_SECTOR   equ    0x2          ;loader在硬盘中的位置
PAGE_DIR_TABLE_POS    equ    0x100000     ;页目录表的位置,位置已经超过了1MB
KERNEL_START_SECTOR   equ    0x9          ;kernel.bin在硬盘中的位置(第9块扇区)
KERNEL_BIN_BASE_ADDR  equ    0x70000      ;kernel.bin将要被放置到内存的位置
KERNEL_ENTRY_POINT    equ    0xc0001500   ;kernel.bin入口点地址


;gdt描述符属性
DESC_G_4K   equ    1_00000000000000000000000b     ;段的粒度为4KB
DESC_D_32   equ    1_0000000000000000000000b      ;使用eip
DESC_L      equ    0_000000000000000000000b       ;32位代码段

DESC_AVL          equ   0_00000000000000000000b   ;为操作系统保留(0)
DESC_LIMIT_CODE2  equ   1111_0000000000000000b    ;段界限(拉满了)
DESC_LIMIT_DATA2  equ   DESC_LIMIT_CODE2          ;总共4GB
DESC_LIMIT_VIDEO2 equ   0000_000000000000000b     ;显示段不需要这么大的空间
DESC_P            equ   1_000000000000000b        ;在内存中

;0～3特权级
DESC_DPL_0        equ   00_0000000000000b
DESC_DPL_1        equ   01_0000000000000b
DESC_DPL_2        equ   10_0000000000000b
DESC_DPL_3        equ   11_0000000000000b

DESC_S_CODE       equ   1_000000000000b          ;代码段或数据段(非系统段)
DESC_S_DATA       equ   DESC_S_CODE              
DESC_S_sys        equ   0_000000000000b          ;系统段,是为各类门准备的
DESC_TYPE_CODE    equ   1000_00000000b           ;单纯的可执行代码段
DESC_TYPE_DATA    equ   0010_00000000b           ;可读写数据段


;代码段的高4字节
;两部分段基址(各1B)都是0
DESC_CODE_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 +\
DESC_L + DESC_AVL + DESC_LIMIT_CODE2 +\
DESC_P + DESC_DPL_0 + DESC_S_CODE +\
DESC_TYPE_CODE + 0x00

;数据段的高4字节
DESC_DATA_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 +\
DESC_L + DESC_AVL +DESC_LIMIT_DATA2 +\
DESC_P + DESC_DPL_0 + DESC_S_DATA +\
DESC_TYPE_DATA + 0x00

;显示段的高4字节
DESC_VIDEO_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 +\
DESC_L + DESC_AVL + DESC_LIMIT_VIDEO2 + DESC_P +\
DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x0b ;与后面的段基址拼接成0xb8000



;选择子属性
RPL0    equ   00b     ;0特权级
RPL1    equ   01b     ;1特权级(用不到)
RPL2    equ   10b     ;2特权级(用不到)
RPL3    equ   11b     ;3特权级
TI_GDT  equ   000b    ;全局描述符
TI_LDT  equ   100b    ;局部描述符

;页表相关属性 
PG_P     equ   1b     ;存在位
PG_RW_R  equ   00b    ;只读
PG_RW_W  equ   10b    ;可读可写
PG_US_S  equ   000b   ;超级用户位(3特权级不可访问)
PG_US_U  equ   100b   ;普通用户位


;判断是否为\0
PT_NULL  equ   0      ;为0意味着该程序段不存在