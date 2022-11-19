
#include "utils.h"

/**
 * remember to free() the returned value
 * @example
 * mergeStrings(2, "abc", "def");
 * mergeStrings(3, "abc", "def", "and now a third");
 *
 * @param {int} argc - number of strings to pass
 * @param {*char[]} ... - variadic array of strings to merge together
 * @return the combination of strings passed in, with space allocated using calloc
 */
char *mergeStrings(int argc, ...) {
  int totalLength = 0;
  char **args = (char**)calloc(argc, sizeof(char*));

  va_list ap;
  va_start(ap, argc);

  for(int i = 0; i < argc; i++ ) {
    char *partial = va_arg(ap, char*);
    args[i] = partial;

    totalLength += (int)strlen(args[i]);
  }

  va_end(ap);

  char *fullMessage = (char*)malloc(sizeof(char) * totalLength + 1);
  int charsCopied = 0;

  for(int i = 0; i < argc; i++ ) {
    strcpy(fullMessage + charsCopied, args[i]);
    charsCopied += (int)strlen(args[i]);
  }

  fullMessage[totalLength] = '\0';
  free(args);

  return fullMessage;
}

