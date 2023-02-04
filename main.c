/* Colin Cummins - smallsh */

/* required for getline*/
#define _GNU_SOURCE
#define  _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* print statement for debug use */
#ifdef DEBUG
#define dprintf(...) fprintf(stderr, __VA_ARGS__)
#else
#define dprintf(...) ((void)0)
#endif




int main(void) {

  char* line_ptr = NULL;
  size_t line_len = 0;
  ssize_t read;
  char** restrict words = calloc(513, sizeof (*words));
  char* restrict word = malloc(sizeof word);
  char* restrict delim = NULL;
  char* const restrict env_ifs = "IFS";

  for (;;) {
    /* TODO: Realloc everything */

    /* Input */
    /* Manage background processes */

    /* Prompt */
    char *prompt = getenv("PS1");
    dprintf("PS1 Prompt: %s",prompt);
    if (prompt == 0) {
      perror("Error: prompt");
      goto cleanup;
    }
    printf("%s", prompt);

    /* Read a line of input */
    read = getline(&line_ptr, &line_len, stdin);
    dprintf("Read %ld chars from line\n", read);
    if (read == -1) {
      perror("error: getline");
      goto cleanup;
    } else if (read == 0) {
      continue;
    }
    dprintf("Line input: %s\n", line_ptr);

    /* Split input */
    delim = getenv(env_ifs);
    if (!delim) delim = " \t\n";
    dprintf("Current IFS delimiter: %s", delim);
    word = strtok(line_ptr, delim);
    while (word != NULL) {
      *words = strdup(word);
      /* TODO null handling */
      dprintf("Copied %s to args",*words);
      words += 1;
      word = strtok(NULL, delim);
    }

    /* Expand */

    /* Parse */

    /* Execute */

   
    

    /* Waiting */

  }

  /* Cleanup */
cleanup:;
        return(EXIT_FAILURE);

return 0;
}
