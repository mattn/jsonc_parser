#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include "jsonc.h"

void json_serialize(JSON_VALUE s, JSON_VALUE v);
void json_value_finalize(JSON_VALUE v);

JSON_ERROR _json_parse_array(char** i, JSON_VALUE* v);
JSON_ERROR _json_parse_null(char** i, JSON_VALUE* v);
JSON_ERROR _json_parse_number(char** i, JSON_VALUE* v);
JSON_ERROR _json_parse_string(char** i, JSON_VALUE* v);
JSON_ERROR _json_parse_boolean(char** i, JSON_VALUE* v);
JSON_ERROR _json_parse_object(char** i, JSON_VALUE* v);
JSON_ERROR _json_parse_any(char **i, JSON_VALUE* v);

int
json_value_ref(JSON_VALUE v) {
  return ++v->ref;
}

int
json_value_unref(JSON_VALUE v) {
  if (v->ref == 1)
    json_value_finalize(v);
  return --v->ref;
}

JSON_VALUE
json_null() {
  static struct _JSON_VALUE n = { 0, JSON_TYPE_NULL, 0 };
  return &n;
}

JSON_VALUE
json_array_new() {
  JSON_VALUE v;
  JSON_ARRAY a = (JSON_ARRAY) malloc(sizeof(JSON_ARRAY));
  a->n = 0;
  a->e = NULL;
  JSON_VALUE_MAKE(v, JSON_TYPE_ARRAY, array, a);
  return v;
}

int
json_array_length(JSON_VALUE v) {
  assert(v->type != JSON_TYPE_ARRAY);
  return v->array->n;
}

JSON_VALUE
json_array_nth(JSON_VALUE v, int i) {
  assert(v->type != JSON_TYPE_ARRAY);
  return (JSON_VALUE) v->array->e[i];
}

void
json_array_append(JSON_VALUE v, JSON_VALUE value) {
  assert(v->type == JSON_TYPE_ARRAY);
  v->array->e = realloc(v->array->e, v->array->n + 1);
  v->array->e[v->array->n++] = value;
}

JSON_VALUE
json_boolean_new(JSON_BOOLEAN b) {
  JSON_VALUE v;
  JSON_VALUE_MAKE(v, JSON_TYPE_BOOLEAN, boolean, b);
  return v;
}

JSON_VALUE
json_number_new(JSON_NUMBER n) {
  JSON_VALUE v;
  JSON_VALUE_MAKE(v, JSON_TYPE_NUMBER, number, n);
  return v;
}

JSON_VALUE
json_string_new(const JSON_STRING str) {
  JSON_VALUE v;
  JSON_VALUE_MAKE(v, JSON_TYPE_STRING, string, strdup(str));
  return v;
}

void
json_string_append(JSON_VALUE v, const JSON_STRING str) {
  assert(v->type == JSON_TYPE_STRING);
  v->string = realloc(v->string, strlen(v->string) + strlen((char*) str) + 1);
  strcat(v->string, str);
}

JSON_VALUE
json_object_new() {
  JSON_VALUE v;
  JSON_VALUE_MAKE(v, JSON_TYPE_OBJECT, object, (JSON_OBJECT) trie_new());
  return v;
}

void
json_object_put(JSON_VALUE v, const char* key, JSON_VALUE value) {
  assert(v->type == JSON_TYPE_OBJECT);
  trie_put((trie*) v->object, key, (void*) value);
}

JSON_VALUE
json_object_get(JSON_VALUE v, const char* key) {
  assert(v->type == JSON_TYPE_OBJECT);
  trie* value = trie_get((trie*) v->object, key);
  assert(value != NULL);
  return value->value;
}

int
_json_finalize_object(const char* key, const void* value, void* data) {
  json_value_unref((JSON_VALUE) value);
  return 0;
}

void
json_value_finalize(JSON_VALUE v) {
  if (v->type == JSON_TYPE_OBJECT) {
    trie_foreach(v->object, _json_finalize_object, NULL);
    trie_free((trie*) v->object);
  }
  if (v->type == JSON_TYPE_ARRAY) {
    int i;
    for (i = 0; i < v->array->n; i++)
      json_value_unref((JSON_VALUE) v->array->e[i]);
    free(v->array);
  }
  if (v->type == JSON_TYPE_STRING)
    free(v->string);
  v->type = JSON_TYPE_UNDEFINED;
}

int
_json_serialize_object(const char* key, const void* value, void* data) {
  JSON_VALUE s = (JSON_VALUE) data;
  assert(s->type == JSON_TYPE_STRING);
  if (*s->string != 0)
    json_string_append(s, ",");
  json_string_append(s, "\"");
  json_string_append(s, (const JSON_STRING) key);
  json_string_append(s, "\":");
  json_serialize(s, (JSON_VALUE) value);
  return 0;
}

void
json_serialize(JSON_VALUE s, JSON_VALUE v) {
  if (v->type == JSON_TYPE_UNDEFINED) {
    json_string_append(s, "undefined");
  } else
  if (v->type == JSON_TYPE_NULL) {
    json_string_append(s, "null");
  } else
  if (v->type == JSON_TYPE_BOOLEAN) {
    json_string_append(s, v->boolean ? "true" : "false");
  } else
  if (v->type == JSON_TYPE_NUMBER) {
    char buf[256];
    double tmp;
    snprintf(buf, sizeof(buf),
      modf(v->number, &tmp) == 0 ? "%.f" : "%f", v->number);
    json_string_append(s, buf);
  } else
  if (v->type == JSON_TYPE_STRING) {
    char* ptr = v->string;
    json_string_append(s, "\"");
    while (*ptr) {
      switch (*ptr) {
      case '"':  json_string_append(s, "\\\""); break;
      case '\\': json_string_append(s, "\\\\"); break;
      case '/':  json_string_append(s, "\\/"); break;
      case '\b':  json_string_append(s, "\\b"); break;
      case '\f':  json_string_append(s, "\\f"); break;
      case '\n':  json_string_append(s, "\\n"); break;
      case '\r':  json_string_append(s, "\\r"); break;
      case '\t':  json_string_append(s, "\\t"); break;
      default:
        if ((unsigned char)*ptr < 0x20 || *ptr == 0x7f) {
          char buf[7];
          snprintf(buf, sizeof(buf), "\\u%04x", *ptr & 0xff);
          json_string_append(s, buf);
        } else {
          char buf[2] = {*ptr, 0};
          json_string_append(s, buf);
        }
      }
      ptr++;
    }
    json_string_append(s, "\"");
  } else
  if (v->type == JSON_TYPE_ARRAY) {
    int i;
    json_string_append(s, "[");
    for (i = 0; i < v->array->n; i++) {
      if (i != 0) json_string_append(s, ",");
      JSON_VALUE ss = json_string_new("");
      json_serialize(ss, (JSON_VALUE) v->array->e[i]);
      json_string_append(s, ss->string);
      json_value_finalize(ss);
    }
    json_string_append(s, "]");
  } else
  if (v->type == JSON_TYPE_OBJECT) {
    json_string_append(s, "{");
    JSON_VALUE ss = json_string_new("");
    trie_foreach(v->object, _json_serialize_object, ss);
    json_string_append(s, ss->string);
    json_value_finalize(ss);
    json_string_append(s, "}");
  }
}

#define _JSONC_SKIP(i) while (strchr("\r\n \t", **i)) { (*i)++; }

JSON_ERROR
_json_parse_array(char** i, JSON_VALUE* v) {
  JSON_VALUE a = json_array_new();
  (*i)++;
  _JSONC_SKIP(i);
  if (**i != ']') {
    while (**i) {
      JSON_VALUE va;
      JSON_ERROR e = _json_parse_any(i, &va);
      if (e != JSON_ERROR_SUCCEEDED) return e;
      json_array_append(a, va);
      _JSONC_SKIP(i);
      if (**i == ']') break;
      if (**i != ',') return JSON_ERROR_INVALID_TOKEN;
      (*i)++;
      _JSONC_SKIP(i);
#ifdef __MINIJSON_LIBERAL
      if (**i == '\x7d') break;
#endif
    }
  }
  *v = a;
  (*i)++;
  return JSON_ERROR_SUCCEEDED;
}

JSON_ERROR
_json_parse_null(char** i, JSON_VALUE* v) {
  char* p = *i;
  if (**i == 'n' && *(*i+1) == 'u' && *(*i+2) == 'l' && *(*i+3) == 'l') {
    *i += 4;
    *v = json_null();
  }
  if (**i && NULL == strchr(":,\x7d]\r\n ", **i)) {
    *i = p;
    return JSON_ERROR_UNDEFINED;
  }
  return JSON_ERROR_SUCCEEDED;
}

JSON_ERROR
_json_parse_boolean(char** i, JSON_VALUE* v) {
  char* p = *i;
  if (**i == 't' && *(*i+1) == 'r' && *(*i+2) == 'u' && *(*i+3) == 'e') {
    *i += 4;
    *v = json_boolean_new(1);
  } else if (**i == 'f' && *(*i+1) == 'a' && *(*i+2) == 'l'
      && *(*i+3) == 's' && *(*i+4) == 'e') {
    *i += 5;
    *v = json_boolean_new(0);
  }
  if (**i && NULL == strchr(":,\x7d]\r\n ", **i)) {
    *i = p;
    return JSON_ERROR_INVALID_TOKEN;
  }
  return JSON_ERROR_SUCCEEDED;
}

JSON_ERROR
_json_parse_number(char** i, JSON_VALUE* v) {
  char* p = *i;

#define _IS_NUM(x)  ('0' <= x && x <= '9')
#define _IS_ALNUM(x)  (('0' <= x && x <= '9') || ('a' <= x && x <= 'f') || ('A' <= x && x <= 'F'))
  if (**i == '0' && *(*i + 1) == 'x' && _IS_ALNUM(*(*i+2))) {
    *i += 3;
    while (_IS_ALNUM(**i)) (*i)++;
    *v = json_number_new(strtod(p, NULL));
  } else {
    while (_IS_NUM(**i)) (*i)++;
    if (**i == '.') {
      (*i)++;
      if (!_IS_NUM(**i)) {
        *i = p;
        return JSON_ERROR_INVALID_TOKEN;
      }
      while (_IS_NUM(**i)) (*i)++;
    }
    if (**i == 'e') {
      (*i)++;
      if (!_IS_NUM(**i)) {
        *i = p;
        return JSON_ERROR_INVALID_TOKEN;
      }
      while (_IS_NUM(**i)) (*i)++;
    }
    *v = json_number_new(strtod(p, NULL));
  }
  if (**i && NULL == strchr(":,\x7d]\r\n ", **i)) {
    *i = p;
    return JSON_ERROR_INVALID_TOKEN;
  }
#undef _IS_NUM
#undef _IS_ALNUM
  return JSON_ERROR_SUCCEEDED;
}

JSON_ERROR
_json_parse_string(char** i, JSON_VALUE* v) {
  char t = **i;
  char* p;
  char* tmp;
  
  p = ++(*i);
  JSON_VALUE s = json_string_new("");
  while (**i && **i != t) {
    if (**i == '\\' && *(*i+1)) {
      (*i)++;
      if (**i == 'n') json_string_append(s, "\n");
      else if (**i == 'r') json_string_append(s, "\r");
      else if (**i == 't') json_string_append(s, "\t");
      else {
        char buf[2] = {**i, 0};
        json_string_append(s, buf);
      }
    } else {
      char buf[2] = {**i, 0};
      json_string_append(s, buf);
    }
    (*i)++;
  }
  if (!**i) return JSON_ERROR_INVALID_TOKEN;

  tmp = malloc(*i - p + 1);
  *tmp = 0;
  strncat(tmp, p, *i - p);
  *v = json_string_new(tmp);
  (*i)++;
  if (**i && NULL == strchr(":,\x7d]\r\n ", **i)) {
    *i = p;
    return JSON_ERROR_INVALID_TOKEN;
  }
  return JSON_ERROR_SUCCEEDED;
}

JSON_ERROR
_json_parse_object(char** i, JSON_VALUE* v) {
  JSON_VALUE o = json_object_new();
  (*i)++;
  _JSONC_SKIP(i);
  if (**i != '\x7d') {
    while (**i) {
      JSON_VALUE vk, vv;
      JSON_ERROR e = _json_parse_string(i, &vk);
      if (e != JSON_ERROR_SUCCEEDED) return e;
      _JSONC_SKIP(i);
      if (**i != ':') return JSON_ERROR_INVALID_TOKEN;
      (*i)++;
      e = _json_parse_any(i, &vv);
      if (e != JSON_ERROR_SUCCEEDED) return e;
      json_object_put(o, vk->string, vv);
      _JSONC_SKIP(i);
      if (**i == '\x7d') break;
      if (**i != ',') return JSON_ERROR_INVALID_TOKEN;
      (*i)++;
      _JSONC_SKIP(i);
      //if (**i == '\x7d') break;
    }
  }
  *v = o;
  (*i)++;
  return JSON_ERROR_SUCCEEDED;
}

JSON_ERROR
_json_parse_any(char **i, JSON_VALUE* v) {
  _JSONC_SKIP(i);
  if (**i == '\x7b') return _json_parse_object(i, v);
  if (**i == '[') return _json_parse_array(i, v);
  if (**i == 't' || **i == 'f') return _json_parse_boolean(i, v);
  if (**i == 'n') return _json_parse_null(i, v);
  if ('0' <= **i && **i <= '9') return _json_parse_number(i, v);
  if (**i == '\'' || **i == '"') return _json_parse_string(i, v);
  return JSON_ERROR_INVALID_TOKEN;
}

JSON_ERROR
json_parse(char **i, JSON_VALUE* v) {
  return _json_parse_any(i, v);
}

#undef _JSONC_SKIP

int
main() {
  JSON_VALUE v;
  JSON_VALUE ss = json_string_new("");

  char* foo = "{'foo': 'bar', 'bar': null, 'baz':[false,'bbb']}";
  JSON_ERROR e = json_parse(&foo, &v);
  if (e == JSON_ERROR_SUCCEEDED) {
    json_serialize(ss, v);
    printf("%s\n", ss->string);
  } else {
    printf("parse error: %s\n", foo);
  }
/*
  JSON_VALUE ss = json_string_new("");
  JSON_VALUE o = json_object_new();
  JSON_VALUE o2 = json_object_new();
  JSON_VALUE a = json_array_new();

  json_object_put(o2, "foo", json_string_new("hello world"));
  json_object_put(o2, "bar", json_string_new("こにゃにゃちわ"));
  json_object_put(o2, "baz", json_number_new(23.4));
  json_object_put(o2, "boo", json_boolean_new(1));

  json_array_append(a, json_boolean_new(1));
  json_array_append(a, json_string_new("fuga"));

  json_object_put(o, "foo", json_string_new("hello world"));
  json_object_put(o, "bar", o2);
  json_object_put(o, "baz", a);

  json_serialize(ss, o);
  printf("%s\n", ss->string);

  json_value_unref(o);
  json_value_unref(o2);
  json_value_unref(ss);
*/
}
