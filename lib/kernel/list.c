#include <list.h>
#include <interrupt.h>

/*  初始化双向链表  */
void list_init(list* list){
    list->head.prev = NULL;  //不使用该属性
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;  //不是用该属性
}


/*  把链表元素elem插入到before之前  */
void list_insert_before(list_elem* before,list_elem* elem){
    intr_status old_status = intr_disable(); //关闭中断以获得原子操作
    before->prev->next = elem;
    elem->prev = before->prev;
    elem->next = before;
    before->prev = elem;
    intr_set_status(old_status); //设置回去
}


/*  添加元素到列表队首  */
void list_push(list* plist,list_elem* elem){
    list_insert_before(plist->head.next, elem);
}


/*  添加元素到列表队尾  */
void list_append(list* plist,list_elem* elem){
    list_insert_before(&plist->tail, elem);
}


/*  使元素pelem脱离链表  */
void list_remove(list_elem* pelem){
    intr_status old_status = intr_disable();
    pelem->prev->next = pelem->next;
    pelem->next->prev = pelem->prev;
    intr_set_status(old_status);
}


/*  将链表的第一项弹出  */
list_elem* list_pop(list* plist){
    list_elem* elem = plist->head.next;
    list_remove(elem);
    return elem;
}


/*  从链表中查找obj_elem,成功返回true,失败返回flase  */
bool elem_find(list* plist,list_elem* obj_elem){
    list_elem* elem = plist->head.next;
    while (elem != &plist->tail) {
        if(elem == obj_elem) return true;
        elem = elem->next;
    }
    return false;
}


/*
  把列表plist的每个元素elem和arg传给回调函数func
  arg给func用来判断elem是否符合条件 
  遍历列表内所有元素,判断是否有符合条件的元素
  找到符合条件的返回元素指针，否则返回NULL 
*/
list_elem* list_traversal(list* plist,function func,int arg){
    list_elem* elem = plist->head.next;
    if(list_empty(plist)) return NULL; //队列为空
    while(elem != &plist->tail){ //遍历
        if(func(elem,arg)) return elem; //符合条件就返回
        elem = elem->next; //不符合条件就查下一个
    }
    return NULL;
}



/*  返回链表长度  */
uint32_t list_len(list* plist){
    list_elem* elem = plist->head.next;
    uint32_t length = 0;
    while(elem != &plist->tail){
        length++;
        elem = elem->next;
    }
    return length;
}


/*  判断链表是否为空,空返回true,否则返回false  */
bool list_empty(list* plist){
    return (plist->head.next == &plist->tail);
}

