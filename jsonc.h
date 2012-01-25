#ifndef _JSONC_H_
#define _JSONC_H_

#include "trie_tree.h"

typedef enum {
  JSON_ERROR_SUCCEEDED,
  JSON_ERROR_UNDEFINED,
  JSON_ERROR_INVALID_TOKEN,
  JSON_ERROR_UNKNOWN_TYPE,
  JSON_ERROR_BADALLOC
} JSON_ERROR;

typedef enum {
  JSON_TYPE_UNDEFINED,
  JSON_TYPE_NULL,
  JSON_TYPE_BOOLEAN,
  JSON_TYPE_NUMBER,
  JSON_TYPE_STRING,
  JSON_TYPE_ARRAY,
  JSON_TYPE_OBJECT
} JSON_TYPE;

typedef int JSON_BOOLEAN;
typedef double JSON_NUMBER;
typedef char* JSON_STRING;
typedef trie* JSON_OBJECT;

typedef struct _JSON_ARRAY {
  int n;
  void** e;
} *JSON_ARRAY;

typedef struct _JSON_VALUE {
  union {
    JSON_BOOLEAN boolean;
    JSON_NUMBER number;
    JSON_STRING string;
    JSON_ARRAY array;
    JSON_OBJECT object;
  };
  JSON_TYPE type;
  int ref;
} *JSON_VALUE;

#define JSON_VALUE_MAKE(r, t, i, v) \
  r = (JSON_VALUE) malloc(sizeof(struct _JSON_VALUE)); \
  r->type = t; \
  r->ref = 1; \
  r->i = v;

#endif /* _JSONC_H_ */
