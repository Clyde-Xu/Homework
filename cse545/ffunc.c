#define _GNU_SOURCE
#include <dlfcn.h>
#include <limits.h>
#include <search.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct _Node {
	void *ptr;
	size_t len;
	struct _Node *next;
}Node;

// dummy header, Header-->Ptr1-->Ptr2-->Ptr3-->NULL
static Node head_node;
static Node *head;
static void* (*malloc_real)(size_t) = NULL;
static void (*free_real)(void *ptr) = NULL;


// Find the size of available space.
// Return Value: A positive number for size or 0 for Not Found.
int find(void *ptr) {
	Node *node = head->next;
	while (node) {
		if (ptr >= node->ptr && ptr < node->ptr + node->len)
			return node->ptr + node->len - ptr;
		node = node->next;
	}
	return 0;
}


// update node with new ptr and new size
void update(void *ptr, void *new_ptr, size_t size) {
	Node *node = head->next;
	while (node) {
		if(node->ptr == ptr) {
			node->ptr = new_ptr;
			node->len = size;
			break;
		}
		node = node->next;
	}
}


void *malloc(size_t size) {
	if (!malloc_real) {
		head = &head_node;
		head->next = NULL;
		malloc_real = dlsym(RTLD_NEXT, "malloc");
		free_real = dlsym(RTLD_NEXT, "free");
	}
	void *ptr = malloc_real(size);
	if (ptr) {
		Node *node = (Node *)malloc_real(sizeof(Node));
		if (node) {
			node->ptr = ptr;
			node->len = size;
			node->next = head->next;
			head->next = node;
		} else {
			free_real(ptr);
			return NULL;
		}
	}
	return ptr;
}


void *realloc(void *ptr, size_t size) {
	static void *(*realloc_real)(void *, size_t size) = NULL;
	if (!realloc_real) {
		realloc_real = dlsym(RTLD_NEXT, "realloc");
	}
	if (!ptr)
		return malloc(size);
	else if(!size) {
		free(ptr);
		return NULL;
	}
	void *new_ptr = realloc_real(ptr, size);
	if (new_ptr)
		update(ptr, new_ptr, size);	
	return new_ptr;
}


char *strcpy(char *dest, const char *src) {
	static char* (*strcpy_real)(char *, const char *) = NULL;
	if (!strcpy_real)
		strcpy_real = dlsym(RTLD_NEXT, "strcpy");
	int left = find(dest);
	if (left) {
		// Make sure available space longer than strlen(src) will not be overwrittern by NULL!!
		int len = left > strlen(src) + 1 ? strlen(src) + 1 : left;
		char *result = strncpy(dest, src, len);
		dest[len - 1] = '\0';
		return dest;
	} else {
		return strcpy_real(dest, src);
	}
}


char *strcat(char *dest, const char *src) {
	strcpy(dest + strlen(dest), src);
	return dest;
}


int sprintf(char *str, const char *format, ...) {
	va_list arg;
	va_start(arg, format);
	int result;
	int left = find(str);
	if (left) {
		result = vsnprintf(str, left, format, arg);
	} else {
		result = vsprintf(str, format, arg);
	}
	va_end(arg);
	return result;
}	


char *getwd(char *wd) {
	static char* (*getwd_real)(char *) = NULL;
	if (!getwd_real)
		getwd_real = dlsym(RTLD_NEXT, "getwd");
	int left = find(wd) - 1;
	if (left >= 0) {
		return getcwd(wd, left);
	} else
		return getwd_real(wd);
}


char *gets(char *s) {
	static char* (*gets_real)(char *) = NULL;
	if (!gets_real)
		gets_real = dlsym(RTLD_NEXT, "gets");
	int left = find(s);
	if (left) {
		return fgets(s, left, stdin);
	} else
		return gets_real(s);
}


void free(void *ptr) {
	Node *node = head;
	while (node->next) {
		if (node->next->ptr == ptr) {
			Node *target = node->next;
			node->next = target->next;
			free_real(ptr);
			free_real(target);
			break;
		}
		node = node->next;
	}
}
