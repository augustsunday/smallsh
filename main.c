/* Colin Cummins - smallsh */

/* required for getline*/
#define _GNU_SOURCE
#define  _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

/* print statement for debug use */
#ifdef DEBUG
#define dprintf(...) fprintf(stderr, __VA_ARGS__)
#else
#define dprintf(...) ((void)0)
#endif

const int WORD_LIMIT = 512;

char *str_gsub(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub)
{
  char *str = *haystack;
  size_t haystack_len = strlen(str);
  size_t const needle_len = strlen(needle),
         sub_len = strlen(sub);

  ptrdiff_t off = str - *haystack;
  for (; (str = strstr(str, needle));) {
      if (sub_len > needle_len) {
        str = realloc(*haystack, sizeof **haystack * (haystack_len + sub_len - needle_len + 1));
        if (!str) goto exit; 
        *haystack = str;
        str = off + *haystack;
      }
      memmove(str + sub_len, str + needle_len, haystack_len + 1 - off - needle_len);
      memcpy(str, sub, sub_len);
      haystack_len = haystack_len + sub_len - needle_len;
      str += sub_len;
      }
  str = *haystack;
  if (sub_len < needle_len) {
    str = realloc(*haystack, sizeof **haystack * (haystack_len + 1));
  }
  if (!str) goto exit;

  *haystack = str;

exit:;
  return str;
 }
 

int split_input(char **words, char *line_ptr) {
    char* restrict word;
    char* restrict delim = getenv("IFS");

    if (!delim) delim = " \t\n";
    word = strtok(line_ptr, delim);
    int i = 0;
    while ((word != NULL) & (i < WORD_LIMIT)) {
      words[i] = strdup(word);
      ++i;
      word = strtok(NULL, delim);
    };

    if (i == WORD_LIMIT && word != NULL) {
      return -1;
    }

    return i;
}

int execute(char  *restrict *restrict args) {
  if (*args == NULL) {
    dprintf("No command word detected. Return to prompt.\n");
    return 0;
  }
  dprintf("Command word: %s\n",*args);
  return 1;

}

int main(void) {
  char* line_ptr = NULL;
  size_t line_len = 0;
  ssize_t read;
  char** words = NULL;
  char* prompt = NULL;
  int words_read = 0;

  words = malloc(sizeof *words * WORD_LIMIT); 

  for (;;) {
    /* TODO: Realloc everything */

    /* Input */
    /* Manage background processes */

    /* Prompt */
    prompt = getenv("PS1");
    dprintf("PS1 Prompt: %s\n",prompt);
    if (prompt == NULL || *prompt == '\0') {
      prompt = "$\0";
    }
    fprintf(stderr,"%s", prompt);

    /* Read a line of input */
    read = getline(&line_ptr, &line_len, stdin);
    dprintf("Read %ld chars from line\n", read);
    if (read == -1) {
      perror("error: getline");
      exit(EXIT_FAILURE);     
    } else if (read == 0) {
      continue;
    } else {
    dprintf("Line input: %s\n", line_ptr);
    };

    /* Split input */
    words_read = split_input(words, line_ptr);
    if (words_read == -1) {
      fprintf(stderr, "smallsh: Exceeded max arguments");
      continue;
    } else if (words_read == 0) {
      continue;
    }

    /* Expand */

    /* Parse */

    /* Execute */
    if (words[0] == NULL) continue;
    execute(words);


   
    

    /* Waiting */

  }

  /* Cleanup */

return 0;
}
