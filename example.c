#include <stdio.h>
#include "jsonc.h"

void
test_serialize() {
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
}

void
test_parse() {
  JSON_VALUE v1;
  char* foo = "{'foo': 'bar', 'bar': null, 'baz':[false,'bbb']}";
  JSON_ERROR e = json_parse(&foo, &v1);
  if (e == JSON_ERROR_SUCCEEDED) {
    JSON_VALUE v2 = json_object_get(v1, "baz");
    JSON_VALUE v3 = json_array_nth(v2, 1);
	printf("%s\n", json_string_get(v3));
    json_value_unref(v1);
  } else {
    printf("parse error: %s\n", foo);
  }

}

int
main() {
  test_serialize();
  test_parse();
}
