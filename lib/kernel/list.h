#ifndef __LIB_KERNEL_LIST_H
#define __LIB_KERNEL_LIST_H
#include "global.h"


/* 宏定义函数，通过PCB中的某个成员获取该PCB  */
#define offset(struct_type,member) (int)(&((struct_type*)0)->member)
#define elem2entry(struct_type,struct_member_name,elem_ptr) \
(struct_type*)((int)elem_ptr - offset(struct_type,struct_member_name))

/*
  定义双向链表的前驱和后继节点
  不需要定义数据成员,主要起连接作用
*/
typedef struct list_elem{
    struct list_elem* prev; //前驱节点
    struct list_elem* next; //后继节点
} list_elem;

/*  双向链表队列  */
typedef struct{
    /*
      head为队首,位置固定不变,第一个元素为head.next;
      tail为队尾,位置固定不变,最后一个元素为tail.prev;
    */
    list_elem head,tail;
} list;


/*自定义函数类型function,用于在list_traversal中做回调函数*/
typedef bool (function)(list_elem*,int arg) ;

/*  函数声明  */
void list_init(list* list);
void list_insert_before(list_elem* before,list_elem* elem);
void list_push(list* plist,list_elem* elem);
void list_iterable(list* plist);
void list_append(list* plist,list_elem* elem);
void list_remove(list_elem* pelem);
list_elem* list_pop(list* plist);
bool elem_find(list* plist,list_elem* obj_elem);
list_elem* list_traversal(list* plist,function func,int arg);
uint32_t list_len(list* plist);
bool list_empty(list* plist);


#endif // !__LIB_KERNEL_LIST_H
