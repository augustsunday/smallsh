/* Colin Cummins - smallsh */

/* required for getline*/
#define _GNU_SOURCE
#define  _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>

/* print statement for debug use */
#ifdef DEBUG
#define dprintf(...) fprintf(stderr, __VA_ARGS__)
#else
#define dprintf(...) ((void)0)
#endif

const int WORD_LIMIT = 512;

struct shell_env {
  char *self_pid;
  char *last_fg;
  char *last_bg;
  char *home_dir;
};


int split_input(char **words, char *line_ptr) {
    /* Splits string 'line_ptr' into an array of substrings pointed at by 'words'
     * Delimiter is $IFS, or default delimiter otherwise
     * Returns number of substrings, or -1 if the WORD_LIMIT is exceeded */
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

char *str_gsub(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub, bool const sub_once)
{
  /* Globaly substitute sub for needle in string haystack, from left to right, non-recursive
   * Set 'sub_once' to 0 to substitute only the first occurence */
  
  char *str = *haystack;
  size_t haystack_len = strlen(str);
  size_t const needle_len = strlen(needle),
         sub_len = strlen(sub);

  ptrdiff_t off = str - *haystack;
  for (; (str = strstr(str, needle));) {
      off = str - *haystack;
      if (sub_len > needle_len) {
        str = realloc(*haystack, sizeof **haystack * (haystack_len + sub_len - needle_len + 1));
        if (!str) {
          perror("Unable to reallocate haystack");
          goto exit; 
         }
        *haystack = str;
        str = off + *haystack;
      }
      memmove(str + sub_len, str + needle_len, haystack_len + 1 - off - needle_len);
      memcpy(str, sub, sub_len);
      haystack_len = haystack_len + sub_len - needle_len;
      str += sub_len;
      if (sub_once) break;
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

int expand(char **args, struct shell_env *environment) {
  /* initialize home dir for possible ~/ substitution and append trailing '/' */
  char *tmp_home_dir = getenv("HOME");
  int len = strlen(tmp_home_dir);
  char *home_dir = NULL;
  home_dir = calloc(2 + len, sizeof (*tmp_home_dir));
  strcpy(home_dir, tmp_home_dir);
  strcat(home_dir, "/");



  for (int i=0; args[i] != NULL && i < WORD_LIMIT; i++) {
    dprintf("Expanding %s", args[i]);

    /* Expand ~/ if it occurs at start of word */
    if (strncmp(args[i], "~/", 2) == 0) {
      str_gsub(&args[i],"~/",home_dir, true);
    }

    /* Smallsh PID  */
    if (str_gsub(&args[i],"$$", environment->self_pid, false) == NULL) perror("Sub $$");

    /* Last fg process exit status  */
    if (str_gsub(&args[i],"$?", environment->last_bg, false) == NULL) perror("Sub $?");

    /* Last bg process exit status  */
    if (str_gsub(&args[i],"$!", environment->last_fg, false) == NULL) perror("Sub $!");

  }

free(home_dir);

return 0;
}
    

int execute(char **args) {
  int childStatus;

  /* initial check for valid command word*/
  if (*args == NULL) {
    dprintf("No command word detected. Return to prompt.\n");
    return 0;
  }

  /*fork and execute*/ 

  pid_t spawnPid = fork();

  switch(spawnPid){
	case -1:
		perror("fork()\n");
		return(1);
		break;
	case 0:
    /* Child process */
    dprintf("Executing child process with command word %s",args[0]);
    execvp(*args, args);
		// exec only returns if there is an error
		perror("execute");
		return(2);
		break;
	default:
		// In the parent process
		// Wait for child's termination
    dprintf("In parent process");
    spawnPid = waitpid(spawnPid, &childStatus, 0);
    dprintf("Child process completed");
    return(0);
		break;
	} 
}

int main(void) {
  char* line_ptr = NULL;
  size_t line_len = 0;
  ssize_t read;
  char** words = NULL;
  char* prompt = NULL;
  int words_read = 0;

  words = malloc(sizeof *words * WORD_LIMIT); 

  struct shell_env environment = {
  .self_pid = NULL,
  .last_bg = "0",
  .last_fg = ""
  };

  /* Get PID of smallsh for later expansion */
  int smallsh_pid = 0;
  smallsh_pid = getpid();
  dprintf("SmallshPID = %d\n",smallsh_pid);
  char *fmt = "%d";
  int len = snprintf(NULL, 0, fmt, smallsh_pid);
  environment.self_pid = malloc(sizeof environment.self_pid * (len + 2));
  snprintf(environment.self_pid, len+1, fmt, smallsh_pid);
  dprintf("SmallshPID = %s\n",environment.self_pid);


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
      goto cleanup;
    } else {
    dprintf("Line input: %s\n", line_ptr);
    };

    /* Split input */
    words_read = split_input(words, line_ptr);
    if (words_read == -1) {
      perror("Exceeded max arguments");
      goto cleanup;
    } else if (words_read == 0) {
      goto cleanup;
    }

    /* Expand */
    expand(words, &environment);

    /* Parse */

    /* Execute */
    if (words[0] == NULL) continue;
    execute(words);
   
    

    /* Waiting */

    /* Cleanup */
cleanup:;
    for (int i=0; i < WORD_LIMIT; i++) {
      if (words[i] != NULL) {
        free(words[i]);
        words[i] = NULL;
      }

    }


  }


return 0;
}
