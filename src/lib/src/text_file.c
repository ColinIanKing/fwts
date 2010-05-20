#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "text_file.h"

void text_free(text_list *head)
{
	while (head) {
		text_list *next = head->next;
		if (head->text)
			free(head->text);
		free(head);
		head = next;
	}
}

text_list *text_read(FILE *file)
{
	char buffer[4096];
	text_list *head = NULL;
	text_list *prev;

	while (fgets(buffer, sizeof(buffer)-1, file) != NULL) {		
		text_list *item;

		if ((item = calloc(sizeof(text_list),1)) == NULL) {
			text_free(head);
			return NULL;
		}
		if ((item->text = malloc(strlen(buffer)+1)) == NULL) {
			text_free(head);
			return NULL;
		}
		strcpy(item->text, buffer);
		if (head == NULL) {
			head = item;
			prev = item;
		}
		else {
			prev->next = item;
			prev = item;
		}
	}
	return head;
}

text_list text_dump(text_list *head)
{
	while (head) {
		printf("%s", head->text);
		head = head->next;
	}
}

char *text_strstr(text_list *head, const char *needle)
{
	while (head) {
		char *match;
		if ((match = strstr(head->text, needle)) != NULL) {
			return match;
		}
		head = head->next;
	}
}
