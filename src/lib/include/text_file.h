#ifndef __TEXT_LIST_H__
#define __TEXT_LIST_H__

typedef struct text_list {
	char *text;
	struct text_list *next;
} text_list;

text_list *text_read(FILE *file);
void 	   text_free(text_list *head);
text_list  text_dump(text_list *head);
char *	   text_strstr(text_list *head, const char *needle);


#endif

