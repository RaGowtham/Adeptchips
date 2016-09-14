#include "include.h"

void add_lable(int lineNo, char *lable_name)
{
    struct lable_list_t *new= calloc(1,sizeof(struct lable_list_t));

    lable_name[strlen(lable_name) - 1] = 0;
    new->name = strdup(&lable_name[1]);
    new->no = lineNo;

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
