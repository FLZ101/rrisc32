#include <assert.h>
#include <rrisc32.h>
#include <stdlib.h>
#include <string.h>

typedef struct block_t {
  struct block_t *prev, *next;
  unsigned size;
  char data[0];
} block;

#define BLOCK_META_SIZE sizeof(block)

#define MIN_ALLOC_SIZE 16

static block free_list_head = {NULL, NULL, 0};

static block *get_new_block(block *prev, unsigned size) {
  if (size < MIN_ALLOC_SIZE)
    size = MIN_ALLOC_SIZE;

  block *res = sbrk(BLOCK_META_SIZE + size);
  if (res == (void *)-1)
    return NULL;

  prev->next = res;

  res->prev = prev;
  res->next = NULL;
  res->size = size;
  return res;
}

static block *get_free_block(unsigned size) {
  if (!size)
    return NULL;

  block *p = &free_list_head;
  while (p->next) {
    p = p->next;
    if (p->size >= size) {
      p->prev->next = p->next;
      if (p->next)
        p->next->prev = p->prev;
      return p;
    }
  }
  return get_new_block(p, size);
}

void merge_free_blocks(block *p) {
  if (!p)
    return;

  assert(p != &free_list_head);

  while (p->data + p->size == p->next) {
    block *q = p->next;
    p->size += BLOCK_META_SIZE + q->size;

    p->next = q->next;
    if (q->next)
      q->next->prev = p;
  }

  if (!p->next) {
    assert(sbrk(0) == p->data + p->size);

    sbrk(-(BLOCK_META_SIZE + p->size));
    p->prev->next = NULL;
  }
}

void put_free_block(block *b) {
  block *p = &free_list_head;
  while (p < b && p->next)
    p = p->next;
  assert(p != b);
  if (p > b)
    p = p->prev;

  b->prev = p;
  b->next = p->next;
  p->next->prev = b;
  p->next = b;

  if (p->data + p->size != b || p == &free_list_head)
    p = b;
  merge_free_blocks(p);
}

static void zero_block(block *p) { memset(p->data, 0, p->size); }

void *malloc(unsigned size) {
  block *p = get_free_block(size);
  if (!p)
    return NULL;
  return p->data;
}

void *calloc(unsigned num, unsigned size) {
  block *p = get_free_block(num * size);
  if (!p)
    return NULL;
  zero_block(p);
  return p->data;
}

void *realloc(void *ptr, unsigned new_size) {
  block *b = (block *)(ptr - BLOCK_META_SIZE);
  if (b->size >= new_size)
    return b;

  block *p = get_free_block(new_size);
  if (!p)
    return NULL;
  memcpy(p->data, b->data, b->size);

  put_free_block(b);

  return p;
}

void free(void *ptr) {
  if (!ptr)
    return;
  block *b = (block *)(ptr - BLOCK_META_SIZE);
  put_free_block(b);
}
