#include <stdio.h>
#include <stdlib.h>
#include "list.h"

SLIST_HEAD(Head, ListData) head;

typedef struct ListData{
	SLIST_ENTRY(ListData) next;
	int data;
}LISTDATA, *PLIST;

int main(){
	PLIST PIndex = NULL;
	LISTDATA data1;
	data1.data = 1;
	LISTDATA data2 = {
		{
			NULL
		},
		2
	};
	LISTDATA data3 = {
		{
			NULL
		},
		3
	};

	LISTDATA *Data1 = &data1;
	LISTDATA *Data2 = &data2;
	LISTDATA *Data3 = &data3;
	SLIST_INIT(&head);
	SLIST_INSERT_HEAD(&head, Data1, next);
	SLIST_INSERT_AFTER(&data1, Data2, next);
	SLIST_INSERT_AFTER(&data2, Data3, next);

	SLIST_REMOVE_HEAD(&head, next);
	LISTDATA *tmp = SLIST_FIRST(&head);
	printf("%d\n", tmp->data);
	SLIST_FOREACH(PIndex, &head, next)
		if(PIndex != NULL)
			printf("PIndex->data = %d\n", PIndex->data);

	return 0;
}
