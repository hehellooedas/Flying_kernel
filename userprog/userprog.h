#ifndef __USERPROG_USERPROG_H
#define __USERPROG_USERPROG_H



/*
  在4GB虚拟地址模型中
  (0~0xc0000000-1)是用户空间,(0xc0000000~0xfffffff)是内核空间
  用户空间的最顶端是栈和环境变量&命令行参数
  我们选取(0xc00000000 - 0x1000)~(0xc0000000)
  这一页作为栈顶(确定栈的虚拟地址)
*/
#define USER_STACK3_VADDR    (0xc0000000 - 0x1000)

#endif // !__USERPROG_USERPROG_H
