#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 4096
#define INIT_LEN 8 // arbitrary initial length of array or object composite

enum json_token_type {
  object_start,
  object_end,
  array_start,
  array_end,
  name_separator,
  value_separator,
  string_token,
  number_token,
  null_token,
  true_token,
  false_token
};

typedef struct json_token {
  enum json_token_type type;
  union {
    char *string;
    long double number;
  } value;
} Json_token;

enum json_value_type { array, object, string, number, null, true, false };

typedef struct json_value {
  enum json_value_type type;
  int is_valid;
  Json_token *token;
  union {
    struct {
      Json_token *key;
      struct json_value *value;
    } *object;
    struct json_value *array;
  } composite;
  size_t composite_i;
  size_t composite_len;
  struct json_value *parent;
} Json_value;

typedef struct {
  Json_token *key;
  struct json_value *value;
} Json_object_entry;

typedef struct json {
  Json_value *value;
} Json;

Json_value *new_json_value(enum json_value_type value_type, Json_value *parent);
void print_tokens(Json_token *ts, size_t ts_len);
Json_value *parse_obj(Json_value *parent_val, Json_token *ts, size_t *ts_i);
void add_obj_entry(Json_token *key, Json_value *value, Json_value *obj);

int main(int argc, char **argv) {
  char *fname = NULL;
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file") == 0) {
      fname = argv[i + 1];
    }
  }

  if (fname == NULL) {
    printf("File name has to be specified.\n");
    exit(1);
  }

  FILE *f;
  if ((f = fopen(fname, "r")) == NULL) {
    char msg[256];
    sprintf(msg, "Couldn't open the file '%s'", fname);
    perror(msg);
    exit(1);
  }

  char buf[BUF_SIZE];
  size_t read_n;
  int eof = 0;
  while (!(eof = feof(f))) {
    read_n = fread(buf, sizeof(*buf), BUF_SIZE, f);
  }
  buf[read_n] = 0;

  char *c = buf;
  Json_token ts[BUF_SIZE / 2];
  size_t ts_i = 0;
  while (*c != 0 && c < buf + BUF_SIZE) {
    // structural tokens
    if (*c == '{') {
      ts[ts_i++] = (Json_token){.type = object_start};
    } else if (*c == '}') {
      ts[ts_i++] = (Json_token){.type = object_end};
    } else if (*c == '[') {
      ts[ts_i++] = (Json_token){.type = array_start};
    } else if (*c == ']') {
      ts[ts_i++] = (Json_token){.type = array_end};
    } else if (*c == ':') {
      ts[ts_i++] = (Json_token){.type = name_separator};
    } else if (*c == ',') {
      ts[ts_i++] = (Json_token){.type = value_separator};
    } else if (*c == '"') { // string token
                            // TODO
                            // escaping and utf-8
      char *str = malloc(sizeof(char) * 256);
      ts[ts_i++] = (Json_token){.type = string_token, .value.string = str};
      do {
        *str++ = *c++;
      } while (*c != 0 && c < buf + BUF_SIZE && *c != '"');
      if (*c == '"') {
        *str++ = *c;
      }
      *str = 0;
    } else if (isdigit(*c)) { // number token
                              // TODO
                              // parsing decimals and scientific notation
      char *str = malloc(sizeof(char) * 256);
      char *strp = str;
      do {
        *strp++ = *c++;
      } while (*c != 0 && c < buf + BUF_SIZE && isdigit(*c));
      *strp = 0;
      ts[ts_i++] =
          (Json_token){.type = number_token, .value.number = atof(str)};
      free(str);
      // literal value tokens
    } else if (strncmp(c, "null", 4) == 0) {
      ts[ts_i++] = (Json_token){.type = null_token};
      c += 3;
    } else if (strncmp(c, "true", 4) == 0) {
      ts[ts_i++] = (Json_token){.type = true_token};
      c += 3;
    } else if (strncmp(c, "false", 5) == 0) {
      ts[ts_i++] = (Json_token){.type = false_token};
      c += 4;
    } else if (*c != ' ' && *c != '\t' && *c != '\n' && *c != '\r') {
      printf("Unexpected token: %c\n", *c);
      exit(1);
    }
    c++;
  }
  printf("\n");

  size_t ts_len = ts_i;
  print_tokens(ts, ts_len);

  Json j;
  Json_value *curr_val = new_json_value(object, NULL);
  j.value = curr_val;
  ts_i = 1;
  Json_token first_token = ts[0];
  // in case we have a literal value in the root of document
  // it has to be the only thing in that document
  // TODO
  // handle token stream array bounds
  switch (first_token.type) {
  case object_start:
    parse_obj(curr_val, ts, &ts_i);
    break;
  case object_end:
    printf("Unexpected <object_end>.\n");
    break;
  case array_start:
    printf("<array_start> ");
    break;
  case array_end:
    printf("Unexpected <array_end>.\n");
    break;
    // we should never encounter name separator on it's own
    // they are always analyzed as part of parsing key-value pair
  case name_separator:
    printf("Unexpected ','.\n");
    break;
  case value_separator:
    printf("Unexpected <value_separator>.\n");
    break;
  case string_token:
    if (curr_val == j.value) {
      curr_val->type = string;
      curr_val->token = &first_token;
      curr_val->parent = NULL;
    }
    break;
  case number_token:
    if (curr_val == j.value) {
      curr_val->type = number;
      curr_val->token = &first_token;
      curr_val->parent = NULL;
    }
    break;
  case null_token:
    if (curr_val == j.value) {
      curr_val->type = null;
      curr_val->token = &first_token;
      curr_val->parent = NULL;
    }
    break;
  case true_token:
    if (curr_val == j.value) {
      curr_val->type = true;
      curr_val->token = &first_token;
      curr_val->parent = NULL;
    }
    break;
  case false_token:
    if (curr_val == j.value) {
      curr_val->type = false;
      curr_val->token = &first_token;
      curr_val->parent = NULL;
    }
    break;
  }
}

void print_tokens(Json_token *ts, size_t ts_len) {
  for (int i = 0; i < ts_len; i++) {
    switch (ts[i].type) {
    case object_start:
      printf("<object_start> ");
      break;
    case object_end:
      printf("<object_end> ");
      break;
    case array_start:
      printf("<array_start> ");
      break;
    case array_end:
      printf("<array_end> ");
      break;
    case name_separator:
      printf("<name_separator> ");
      break;
    case value_separator:
      printf("<value_separator> ");
      break;
    case string_token:
      printf("<string = %s> ", ts[i].value.string);
      break;
    case number_token:
      printf("<number = %Lf> ", ts[i].value.number);
      break;
    case null_token:
      printf("<null> ");
      break;
    case true_token:
      printf("<true> ");
      break;
    case false_token:
      printf("<false> ");
      break;
    }
  }
  printf("\n");
}

// returns pointer to the object
// ts_i is an index to the first token after <object_start>
// which is mutated to be the index of first unparsed token
Json_value *parse_obj(Json_value *parent_val, Json_token *ts, size_t *ts_i) {
  size_t i = *ts_i;
  Json_value *curr_val = new_json_value(object, parent_val);

  while (ts[i].type == string_token) {
    Json_token *key = &ts[i];
    if (ts[++i].type != name_separator) {
      printf("Expected <name_separator>.\n");
      exit(1);
    }

    Json_value *val;
    *ts_i = i + 1;
    switch (ts[*ts_i].type) {
    case object_start:
      *ts_i = *ts_i + 1;
      val = parse_obj(curr_val, ts, ts_i);
      break;
    case array_start:
      // TODO
      *ts_i = *ts_i + 1;
      val = parse_obj(curr_val, ts, ts_i);
      break;
    case string_token:
      val = new_json_value(string, curr_val);
      val->token = &ts[*ts_i];
      *ts_i = *ts_i + 1;
      break;
    case number_token:
      val = new_json_value(number, curr_val);
      val->token = &ts[*ts_i];
      *ts_i = *ts_i + 1;
      break;
    case null_token:
      val = new_json_value(null, curr_val);
      val->token = &ts[*ts_i];
      *ts_i = *ts_i + 1;
      break;
    case false_token:
      val = new_json_value(false, curr_val);
      val->token = &ts[*ts_i];
      *ts_i = *ts_i + 1;
      break;
    case true_token:
      val = new_json_value(true, curr_val);
      val->token = &ts[*ts_i];
      *ts_i = *ts_i + 1;
      break;
    }
    i = *ts_i;

    add_obj_entry(key, val, curr_val);

    if (ts[i++].type != value_separator) {
      break;
    }
  }

  if (ts[i].type != object_end) {
    // TODO
    // add token pointers
    printf("Token type: %d\n", ts[i].type);
    printf("Unexpected token while parsing object.\n");
    exit(1);
  }
  *ts_i = i + 1;
  return curr_val;
}

void add_obj_entry(Json_token *key, Json_value *value, Json_value *json_val) {
  if (json_val->composite_i == json_val->composite_len) {
    json_val->composite_len *= 2;
    // fail hard if unable to allocate more memory
    if ((json_val->composite.object = realloc(
             json_val->composite.object, json_val->composite_len)) == NULL) {
      perror("Failed to allocate more memory.");
      exit(1);
    }
  }
  json_val->composite.object[json_val->composite_i].key = key;
  json_val->composite.object[json_val->composite_i].value = value;
  json_val->composite_i++;
}

Json_value *new_json_value(enum json_value_type value_type,
                           Json_value *parent) {
  Json_value *val = malloc(sizeof(Json_value));
  val->parent = parent;
  val->type = object;
  val->token = NULL;
  val->parent = NULL;
  val->composite_i = 0;
  val->composite_len = INIT_LEN;
  val->composite.object =
      malloc(sizeof(Json_object_entry) * val->composite_len);
  return val;
}
