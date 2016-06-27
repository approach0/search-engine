#include <stdint.h>
#include <stdbool.h>

typedef bool (*heap_lt_fun)(void*, void*);
typedef void (*heap_pr_fun)(void*, uint32_t, uint32_t);

struct heap {
	void       **array;
	uint32_t     volume;
	uint32_t     end;
	heap_lt_fun  ltf;
};

struct heap heap_create(uint32_t);
void        heap_destory(struct heap*);
void        heap_set_callbk(struct heap*, heap_lt_fun);
bool        heap_full(struct heap*);
uint32_t    heap_size(struct heap*);
void        heap_push(struct heap*, void*);
void        heap_print_tr(struct heap*, heap_pr_fun);
void        heap_print_arr(struct heap*, heap_pr_fun);
void       *heap_top(struct heap*);
void        minheap_heapify(struct heap*);
void        minheap_insert(struct heap*, void*);
void        minheap_delete(struct heap*, uint32_t);
void        minheap_replace(struct heap*, uint32_t, void*);
void        minheap_sort(struct heap*);
void        heap_sort_desc(struct heap*);
