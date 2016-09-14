#include "include.h"

void predefined_symbols(void)
{
    struct pre
    {
	char *name;
	int value;
    }list[] = {{"R0",0}, {"R1",1}, {"R2",2}, {"R3",3}, {"R4",4}, {"R5",5}, {"R6",6}, {"R7",7}, {"R8",8}, {"R9",9}, {"R10",10}, {"R11",11}, {"R12",12}, {"R13",13}, {"R0",14}, {"R0",15}, {"SCREEN",16384},{"KBD",24576}, {"SP",0}, {"LCL",1}, {"ARG",2}, {"THIS",3},{"THAT",4}};
    int i;
    struct lable_list_t *new;
    for(i = 0; i < sizeof(list)/sizeof(struct pre);i++)
    {
	new = calloc(1,sizeof(struct lable_list_t));
	new->name = list[i].name;
	new->no = list[i].value;
	if(lable_list == NULL)
	{   
	    lable_list = new;
	    last = new;
	}   
	else
	{   
	    last->next = new;
	    last = new;
	} 
    }
}
