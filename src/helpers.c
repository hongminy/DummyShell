// Your helper functions need to be here.
#include "shell_util.h"
#include <stdio.h>
#include <string.h>
int timeComparator(void* time1, void* time2){
	return difftime((time_t)time1, (time_t)time2);
}

char* findCommandByPid(List_t* list, pid_t pid)
{
	int length = list->length;
	node_t* head = list->head;
	int i = 0;
	for(; i < length; i++){
		ProcessEntry_t* p = head->value;
		if (p->pid == pid){
			return p->cmd;
		}
		head = head->next;
	}
	return '\0';

}

char *strcatarray(char *strings[], size_t number)
{

		 size_t i = 0;
		 size_t total = 1;
		 for (; i< number; i++){
			 total += strlen(strings[i]);
		 }
		 char* dest = malloc(sizeof(char)*total);
		 dest[0] = '\0';
     for (; i < number; i++) {
         strcat(dest, strings[i]);
     }
     return dest;
}
