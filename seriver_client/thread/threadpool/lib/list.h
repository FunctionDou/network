// 单链表的头部
#define SLIST_HEAD(name, type) \
struct name {				\
	struct type *slh_first;	\
}

// 接下来就是需要一个链表的入口可以理解为链条，用来连接指向下一个节点的。
#define SLIST_ENTRY(type) \
struct {				\
	struct type *sle_next;	\
}

// 定义list的访问方法
#define SLIST_FIRST(head)		((head)->slh_first)
#define SLIST_END(head)			NULL
#define SLIST_EMPTY(head)		(SLIST_FIRST(head) == SLIST_END(head))
#define SLIST_NEXT(elm, field)	((elm)->field.sle_next)
#define SLIST_FOREACH(var, head, field) \
	for((var) = SLIST_FIRST(head); \
		(var) != SLIST_END(head); \
		(var) = SLIST_NEXT(var, field))

#define SLIST_INIT(head){ \
	SLIST_FIRST(head) = SLIST_END(head); \
}

#define SLIST_INSERT_AFTER(slistelm, elm, field) do{	\
	(elm)->field.sle_next = (slistelm)->field.sle_next;	\
	(slistelm)->field.sle_next = (elm);		\
}while(0)

#define SLIST_INSERT_HEAD(head, elm, field) do{ \
	(elm)->field.sle_next = (head)->slh_first;	\
	(head)->slh_first = (elm);\
}while(0)

#define SLIST_REMOVE_HEAD(head, field) do{	\
	(head)->slh_first = (head)->slh_first->field.sle_next;\
}while(0)
