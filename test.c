#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

/**
 * After many years, got the bug to write some C again. 
 * This was mainly inspired by a guy writing their own bootloader to play tetris
 *
 * Being used to modern languages,
 * I'm missing Garbage Collection, Classes syntax, Exceptions, maybe namespaces
 */


/**
 * All great achievements in software were a transformation on the way we think
 * A file system with seemingly random create/write/delete files on a flat disk
 * An infinitely scrolling console, while on a simple 80x25 terminal display
 * Windows, overlapping, on a flat GUI
 * Message queues
 * TCP/IP over a very unreliable network hardware infrastucture
 * A multi user, multi process OS with inter-process communication
 *
 * What could the lowly me create in a few lines of C ?
 * C++ is already taken. Cloud functions?
 *
 */





/**
 * Sample class definition -- too c++ for my taste
 * Maybe the original file io struct is better design for C
 *
 * Usage:
 *
 *   Class *c = newClass(10);
 *   int j = c->method1(c, 1, 2);
 *   printf("j is now %i\n", j);
 */
typedef struct Class_struct Class;
struct Class_struct {
	int base;
	int (*method1)(Class *, int, int);
};
int Class_method1(Class *this, int arg1, int arg2) {
	return this->base + arg1 + arg2;
}
Class *newClass(int base) {
	Class *p = malloc(sizeof(Class));
	p->base = base;
	p->method1 = Class_method1;
	return p;
}
void test_class_construct() {
	Class *c = newClass(10);
	int j = c->method1(c, 1, 2);
	printf("j is now %i\n", j);
	free(c);
}



/**
 * Trying out creating a doubly linked list functionality
 * Mind you, this is a O(n) search operation
 */
typedef struct LLNode_struct LLNode;
typedef struct LL_struct LL;
struct LLNode_struct {
	LLNode *next;
	LLNode *prev;
	void *data;
};
struct LL_struct {
	LLNode *head;
	LLNode *tail;
	int length;
};
LL *ll_create() {
	LL *p = malloc(sizeof(LL));
	p->head = NULL;
	p->tail = NULL;
	p->length = 0;
	return p;
}
bool ll_empty(LL *ll) {
	return ll->length == 0;
}
int ll_length(LL *ll) {
	return ll->length;
}
void ll_add(LL *ll, void *something) {
	LLNode *n = malloc(sizeof(LLNode));
	n->data = something;
	n->next = NULL;
	n->prev = NULL;

	if (ll->head == NULL)
		ll->head = n;

	if (ll->tail != NULL)
		ll->tail->next = n;
	n->prev = ll->tail;
	ll->tail = n;

	ll->length++;
}
bool ll_contains(LL *ll, void *something) {
	LLNode *n = ll->head;
	while (n != NULL) {
		if (n->data == something)
			return true;
		n = n->next;
	}
	return false;
}
bool ll_remove(LL *ll, void *something) {
	LLNode *n = ll->head;
	while (n != NULL) {
		if (n->data != something) {
			n = n->next;
			continue;
		}

		if (n->prev != NULL)
			n->prev->next = n->next;
		if (n->next != NULL)
			n->next->prev = n->prev;
		if (ll->head == n)
			ll->head = n->next;
		if (ll->tail == n)
			ll->tail = n->prev;
		free(n);
		ll->length--;
		break;
	}
}
void ll_foreach(LL *ll, void pf(void *something)) {
	LLNode *n = ll->head;
	while (n != NULL) {
		pf(n->data);
		n = n->next;
	}
}
void ll_map(LL *ll, void *pf(void *something)) {
	LLNode *n = ll->head;
	while (n != NULL) {
		n->data = pf(n->data);
		n = n->next;
	}
}
LL *ll_filter(LL *ll, bool pf(void *something)) {
	LL *filtered = ll_create();
	LLNode *n = ll->head;
	while (n != NULL) {
		if (pf(n->data))
			ll_add(filtered, n->data);
		n = n->next;
	}
	return filtered;
}
void ll_free(LL *ll) {
	LLNode *n = ll->head;
	while (n != NULL) {
		LLNode *next = n->next;
		free(n);
		n = next;
	}
	free(ll);
}
void ll_unit_tests() {
	LL *ll;

	// test creation
	ll = ll_create();
	assert(ll != NULL);
	assert(ll_length(ll) == 0);
	assert(ll_empty(ll));
	assert(ll->head == NULL);
	assert(ll->tail == NULL);
	ll_free(ll);

	void *payload1 = (void *)"Data 1";
	void *payload2 = (void *)"Data 2";
	
	// test first addition
	ll = ll_create();
	ll_add(ll, payload1);
	assert(!ll_empty(ll));
	assert(ll_length(ll) == 1);
	assert(ll_contains(ll, payload1));
	assert(ll->head != NULL);
	assert(ll->tail != NULL);
	assert(ll->head == ll->tail);
	assert(ll->head->prev == NULL);
	assert(ll->head->next == NULL);
	assert(ll->tail->prev == NULL);
	assert(ll->tail->next == NULL);
	assert(ll->head->data == payload1);
	ll_free(ll);

	// test addition at the end
	ll = ll_create();
	ll_add(ll, payload1);
	ll_add(ll, payload2);
	assert(ll_contains(ll, payload2));
	assert(!ll_empty(ll));
	assert(ll_length(ll) == 2);
	assert(ll->head != NULL);
	assert(ll->tail != NULL);
	assert(ll->head != ll->tail);
	assert(ll->head->data == payload1);
	assert(ll->tail->data == payload2);
	assert(ll->head->next = ll->tail);
	assert(ll->tail->prev == ll->head);
	assert(ll->head->prev == NULL);
	assert(ll->tail->next == NULL);
	ll_free(ll);

	// test deletion of head
	ll = ll_create();
	ll_add(ll, payload1);
	ll_add(ll, payload2);
	ll_remove(ll, payload1);
	assert(!ll_empty(ll));
	assert(ll_length(ll) == 1);
	assert(!ll_contains(ll, payload1));
	assert(ll_contains(ll, payload2));
	assert(ll->head == ll->tail);
	assert(ll->head->data == payload2);
	assert(ll->head->next == NULL);
	assert(ll->head->prev == NULL);
	ll_free(ll);

	// test deletion of tail
	ll = ll_create();
	ll_add(ll, payload1);
	ll_add(ll, payload2);
	ll_remove(ll, payload2);
	assert(!ll_empty(ll));
	assert(ll_length(ll) == 1);
	assert(ll_contains(ll, payload1));
	assert(!ll_contains(ll, payload2));
	assert(ll->head == ll->tail);
	assert(ll->head->data == payload1);
	assert(ll->head->next == NULL);
	assert(ll->head->prev == NULL);
	ll_free(ll);

	// test final deletion
	ll = ll_create();
	ll_add(ll, payload1);
	ll_remove(ll, payload1);
	assert(ll_empty(ll));
	assert(ll_length(ll) == 0);
	assert(!ll_contains(ll, payload1));
	assert(ll->head == NULL);
	assert(ll->tail == NULL);
	ll_free(ll);
}





int main(int argc, char *argv[]) {
	if (argc == 2 && strcmp(argv[1], "tests") == 0) {
		printf("Running tests\n");
		ll_unit_tests();
		printf("Tests finished\n");
		exit(0);
	}

	printf("Hello, mits!\n");
	test_class_construct();

	char *p = malloc(1000);
	printf("Gotten a memory chunk!\n");
	free(p);

	int i;
	printf("sizeof(char) is %lu\n", sizeof(char));
	printf("sizeof(short) is %lu\n", sizeof(short));
	printf("sizeof(int) is %ld\n", sizeof(int));
	printf("sizeof(long) is %lu\n", sizeof(long));
}


