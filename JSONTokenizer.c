#include "JSONTokenizer.h"
extern int errorLoc;
struct JSONToken *parseJSON(char **str) {
  errorLoc = -1;
  int position = 0;
  struct JSONToken *root = createEmptyToken();
  if (!tokenize(str, &position, root)) {
    freeJSON(root);
    return 0;
  }
  return root;
}

int tokenize(char **str, int *position, struct JSONToken *token) {
  skipWhitespace(str, position);
  char first = **str;
  token->start = *position;
  ++(*str);
  ++(*position);
  skipWhitespace(str, position);
  if (first == '{') {
    token->type = TYPE_object;
    struct JSONToken *last = NULL;
    while (**str != '}') {
      if (**str != '"') {
        setErrorLoc(*position);
        return 0;
      }
      struct JSONToken *curPair = createEmptyToken();
      ++(*str);
      ++(*position);
      curPair->keyStart = *position;
      int ret = skipKey(str, position);
      if (!ret) {
        // malformed key
        freeJSON(curPair);
        setErrorLoc(*position);
        return 0;
      }
      curPair->keyEnd = ret;
      if (!tokenizeValue(curPair, str, position)) {
        freeJSON(curPair);
        setErrorLoc(*position);
        return 0;
      }
      skipWhitespace(str, position);
      char cur = **str;
      ++(*str);
      ++(*position);
      if (cur != ',') {
        if (cur == '}') {
          if (last) {
            last->next = curPair;
          } else {
            token->child = curPair;
          }
          break;
        }
        freeJSON(curPair);
        setErrorLoc(*position);
        return 0;
      }
      skipWhitespace(str, position);
      if (last) {
        last->next = curPair;
      } else {
        token->child = curPair;
      }
      last = curPair;
    }
    if (**str == '}') {
      ++(*str);
      ++(*position);
    }
  } else if (first == '[') {
    token->type = TYPE_array;
    struct JSONToken *last = NULL;
    while (**str != ']') {
      struct JSONToken *curVal = createEmptyToken();
      if (!tokenizeValue(curVal, str, position)) {
        freeJSON(curVal);
        setErrorLoc(*position);
        return 0;
      }
      skipWhitespace(str, position);
      char cur = **str;
      ++(*str);
      ++(*position);
      if (cur != ',') {
        if (cur == ']') {
          if (last) {
            last->next = curVal;
          } else {
            token->child = curVal;
          }
          break;
        }
        freeJSON(curVal);
        setErrorLoc(*position);
        return 0;
      }
      skipWhitespace(str, position);
      if (last) {
        last->next = curVal;
      } else {
        token->child = curVal;
      }
      last = curVal;
    }
    if (**str == ']') {
      ++(*str);
      ++(*position);
    }
  } else {
    freeJSON(token);
    setErrorLoc(*position);
    return 0;
  }
  return 1;
}

struct JSONToken *createEmptyToken() {
  struct JSONToken *token = calloc(1, sizeof(struct JSONToken));
  if (token) {
    token->keyEnd = token->keyStart = -1;
    return token;
  } else {
    // ERROR ALLOCATING MEMORY
    freeJSON(token);
    return 0;
  }
}

int tokenizeValue(struct JSONToken *token, char **str, int *position) {
  switch (**str) {
  // String
  case '"':
    token->type = TYPE_string;
    token->start = *position;
    ++(*str);
    ++(*position);
    bool escaped = false;
    while (true) {
      if (**str == '"')
        if (escaped)
          escaped = false;
        else
          break;
      else
        escaped = (**str == '\\');
      ++(*str);
      ++(*position);
      if (!**str)
        return 0;
    }
    ++(*str);
    ++(*position);
    token->end = *position;
    break;
  // True/False
  case 't':
    token->type = TYPE_boolean;
    token->start = *position;
    if (strncmp(*str, "true", 4) == 0) {
      (*str) += 4;
      (*position) += 4;
      token->end = *position;
    } else
      return 0;
    break;
  case 'f':
    token->type = TYPE_boolean;
    token->start = *position;
    if (strncmp(*str, "false", 5) == 0) {
      (*str) += 5;
      (*position) += 5;
      token->end = *position;
    } else
      return 0;
    break;
  // Null
  case 'n':
    token->type = TYPE_nil;
    token->start = *position;
    if (strncmp(*str, "null", 4) == 0) {
      (*str) += 4;
      (*position) += 4;
      token->end = *position;
    } else
      return 0;
    break;
  case '[':
    token->type = TYPE_array;
    token->start = *position;
    if (!tokenize(str, position, token))
      return 0;
    token->end = *position;
    break;
  case '{':
    token->type = TYPE_object;
    token->start = *position;
    if (!tokenize(str, position, token))
      return 0;
    token->end = *position;
    break;
  // Number, or malformed.
  default:
    token->type = TYPE_number;
    token->start = *position;
    bool leadingDigits = false;
    char first = **str;
    if (!strchr("+-0123456789.", first))
      return 0;
    if (strchr("-+0123456789", first)) {
      if (strchr("0123456789", **str)) {
        leadingDigits = true;
      }
      ++(*str);
      ++(*position);
      while (strchr("0123456789", **str)) {
        leadingDigits = true;
        ++(*str);
        ++(*position);
      }
    }
    if (**str == '.') {
      ++(*str);
      ++(*position);
      while (strchr("0123456789", **str)) {
        leadingDigits = true;
        ++(*str);
        ++(*position);
      }
    }
    if (strchr("eE", **str) && leadingDigits) {
      ++(*str);
      ++(*position);
      if (!strchr("+-", **str))
        return 0;
      ++(*str);
      ++(*position);
      while (strchr("0123456789", **str)) {
        ++(*str);
        ++(*position);
      }
    }
    token->end = *position;
    break;
  }
  return 1;
}

void freeJSON(struct JSONToken *object) {
  if (object->child) {
    freeJSON(object->child);
  }
  if (object->next) {
    freeJSON(object->next);
  }
  free(object);
}

void printJSON(struct JSONToken *object, char **str, FILE *fp) {
  printHelper(object, str, 0, fp);
}
void printHelper(struct JSONToken *object, char **str, int indent, FILE *fp) {
  printIndent(indent, fp);
  if (object->keyEnd != -1) {
    fprintf(fp, "\"%.*s\": ", object->keyEnd - object->keyStart,
            (*str) + object->keyStart);
  }
  switch (object->type) {
  case TYPE_object:
    if (object->child) {
      fprintf(fp, "{\n");
      printHelper(object->child, str, indent + 1, fp);
      printIndent(indent, fp);
      fprintf(fp, "}");
    } else {
      fprintf(fp, "{}");
    }
    break;
  case TYPE_array:
    if (object->child) {
      fprintf(fp, "[\n");
      printHelper(object->child, str, indent + 1, fp);
      printIndent(indent, fp);
      fprintf(fp, "]");
    } else {
      fprintf(fp, "[]");
    }
    break;
  default:
    fprintf(fp, "%.*s", object->end - object->start, (*str) + object->start);
    break;
  }
  if (object->next) {
    fprintf(fp, ",\n");
    printHelper(object->next, str, indent, fp);
  } else {
    fprintf(fp, "\n");
  }
}

void printIndent(int indent, FILE *fp) {
  for (int i = indent; i > 0; --i)
    fprintf(fp, "\t");
}

char *getFormattedString(struct JSONToken *object, char **str) {
  char *buf;
  size_t buf_size;
  FILE *bp = open_memstream(&buf, &buf_size);
  if (!bp) {
    return NULL;
  }
  printJSON(object, str, bp);
  fclose(bp);
  return buf;
}
void setErrorLoc(int loc) {
  if (errorLoc == -1) {
    errorLoc = loc;
  }
}
