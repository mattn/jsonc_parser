#ifndef _TRIE_TREE_H_
#define _TRIE_TREE_H_
#include <stdlib.h>

typedef struct _trie {
  char c;
  unsigned int n;
  struct _trie* parent;
  struct _trie** next;
  void* value;
} trie;

typedef int (* trie_foreach_func)
  (const char* key, const void* value, void* data);

trie*
trie_new() {
  trie* p = (trie*) malloc(sizeof(trie));
  p->c = 0;
  p->n = 0;
  p->next = NULL;
  p->parent = NULL;
  return p;
}

void
trie_free(trie* p) {
  unsigned int i;
  for (i = 0; i < p->n; i++)
    trie_free(p->next[i]);
  if (p->n)
    free(p->next);
  free(p);
}

trie*
trie_put(trie* p, const char* key, const void* value) {
  int i;
  while (p) {
    trie* next = NULL;
    if (*key == 0) {
      p->value = (void*) value;
      return p;
    }
    for (i = 0; i < p->n; i++)
      if (p->next[i]->c == *key) {
        next = p->next[i];
        break;
      }
    if (!next) {
      trie** children;
      next = trie_new();
      if (!next) return NULL;
      children = (trie**) realloc(p->next, (p->n+1) * sizeof(trie*));
      if (!children) return NULL;
      p->next = children;
      next->c = *key;
      next->parent = p;
      p->next[p->n] = next;
      p->n++;
    }
    key++;
    p = next;
  }
  return NULL;
}

trie*
trie_get(trie* p, const char* key) {
  int i;
  while (p) {
    trie* next = NULL;
    for (i = 0; i < p->n; i++) {
      if (p->next[i]->c == *key) {
        next = p->next[i];
        break;
      }
    }
    if (!next) return NULL;
    if (*(key+1) == 0) return next;
    key++;
    p = next;
  }
  return NULL;
}

void
trie_delete(trie* p, const char* key) {
  p = trie_get(p, key);
  while (p) {
    if (p->n == 0) {
      trie* pp = p;
      p = p->parent;
      if (!p) return;
      if (p->n == 1) {
        free(p->next);
        p->next = NULL;
      } else {
        trie** children = (trie**) malloc((p->n-1) * sizeof(trie*));
        int i, j = 0;
        for (i = 0; i < p->n; i++)
          if (p->next[i] != pp)
            children[j++] = p->next[i];
        p->next = children;
      }
      p->n--;
    } else
      p = p->parent;
  }
}

int
trie_foreach(trie* p, trie_foreach_func f, void* data) {
  unsigned int i;
  int ret, step;
  char* key;
  trie* pp;
  for (i = 0; i < p->n; i++) {
    int ret = trie_foreach(p->next[i], f, data);
    if (ret != 0) return ret;
  }
  if (p->n == 0) {
    for (pp = p, step = 0; pp->parent; pp = pp->parent, ++step);
    key = (char*) malloc(step + 1);
    key[step] = 0;
    for (pp = p; pp->parent; pp = pp->parent)
      key[--step] = pp->c;
    return f(key, p->value, data);
  }
  return 0;
}

#endif /* _TRIE_TREE_H_ */
