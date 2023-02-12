/* Colin Cummins - smallsh */

/* required for getline*/
#define _GNU_SOURCE
#define  _POSIX_C_SOURCE 200809L


#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

/* Maximum # of arguments allowed in a line */
#define WORD_LIMIT 512

/* print statement for debug use */
#ifdef DEBUG
#define dprintf(...) fprintf(stderr, __VA_ARGS__)
#else
#define dprintf(...) ((void)0)
#endif


struct shell_env {
  pid_t self_pid;
  int last_fg_exit_status;
  pid_t last_bg_pid;
  char *home_dir;
  char *infile_path;
  char *outfile_path;
  bool run_in_background;
  sigset_t sh_signals;
};

char *pid_to_str(pid_t process_id) {
  /*Converts a pid to a string and returns a pointer to that string */
  /*Uses dynamic memory allocation - be sure to free after use! */
  char *str = NULL;
  char *fmt = "%d";
  int len = snprintf(NULL, 0, fmt, process_id);
  str = malloc(sizeof *str * (len + 2));
  snprintf(str, len+1, fmt, process_id);
  return str;
}

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

  /* set substitution strings based on environment values or defaults if not available */
  char *self_pid_sub = pid_to_str(environment->self_pid);
  char *last_fg_exit_status_sub = pid_to_str(environment->last_fg_exit_status);
  char *last_bg_pid_sub = NULL;
  if (environment->last_bg_pid == 0) {
    last_bg_pid_sub = "";
  } else {
    last_bg_pid_sub = pid_to_str(environment->last_bg_pid);
  }
  
  for (int i=0; args[i] != NULL && i < WORD_LIMIT; i++) {

    /* Expand ~/ if it occurs at start of word */
    if (strncmp(args[i], "~/", 2) == 0) {
      str_gsub(&args[i],"~/",home_dir, true);
    }

    /* Smallsh PID  */
    if (str_gsub(&args[i],"$$", self_pid_sub, false) == NULL) perror("Sub $$");

    /* Last fg process exit status  */
    if (str_gsub(&args[i],"$?", last_fg_exit_status_sub, false) == NULL) perror("Sub $?");

    /* Last bg process pid  */
    if (str_gsub(&args[i],"$!", last_bg_pid_sub, false) == NULL) perror("Sub $!");

  }

/* pid_to_str uses dynamic memory allocation so we have to clean up*/
free(home_dir);
free(self_pid_sub);
free(last_fg_exit_status_sub);
if (strlen(last_bg_pid_sub) > 0) free(last_bg_pid_sub);

return 0;
}


int cd(char **args, struct shell_env *environment) {
  int ret = 0;
  dprintf("cd built-in\n");
  dprintf("args[1]: %s\n",args[1]);
  dprintf("args[2]: %s\n",args[2]);
  if (args[1] != NULL && args[2] != NULL) {
    perror("too many arguments");
    return 1;
  }

  else if (args[1] == NULL) {
    dprintf("No argument provided. cd to HOME\n");
    ret = chdir(getenv("HOME"));
    if (ret == 0) return 0;

    perror("Unable to cd to HOME");
    return(errno);
  }
  else {
    dprintf("Attempting to change to dir %s",args[1]);
    ret = chdir(args[1]);
    if (ret == 0) return 0;

    perror("Unable to cd to directory provided");
    return(errno);
  }
  return(errno);
}

int exit_sh(char **args, struct shell_env *environment) {
  /* set exit status*/
  int exit_status = environment->last_fg_exit_status;
  dprintf("built-in exit function 'exit_sh'\n");
  dprintf("args[1]: %s\n",args[1]);
  dprintf("args[2]: %s\n",args[2]);

  if (args[1] != NULL && args[2] != NULL) {
    perror("too many arguments");
    return 1;
  }
  if (args[1]) exit_status = atoi(args[1]);

  /* SIGINT all smallsh processes */
  kill(0, SIGINT); 

  fprintf(stderr, "\nexit\n");
  exit(exit_status);
}
    

int execute(char **args, struct shell_env *environment) {
  int childStatus;

  /* initial check for existence of valid command word*/
  if (*args == NULL) {
    dprintf("No command word detected. Return to prompt.\n");
    return 0;
  }

  /* Route requests for built-ins to custom functions */
  
  /* exit */
  if (strcmp(*args, "exit") == 0) {
    dprintf("exit built-in\n");
    int success = exit_sh(args, environment);
    return success;
  }

  /* cd */

  if (strcmp(*args, "cd") == 0) {
    dprintf("cd built-in\n");
    int success = cd(args, environment);
    return success;
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

    /* TODO Reset all signals to dispositions when smallsh was invoked */
    sigfillset(&environment->sh_signals);
    signal(SIGINT, SIG_DFL);

    /* Redirect input and output, if requested */
    if (environment->outfile_path != NULL) {
      if(fclose(stdout) != 0) {
        perror("Unable to close stdout for redirection\n");
        exit(errno);
      };
      if (open(environment->outfile_path, O_WRONLY | O_CREAT | O_TRUNC, 0777) == -1) {
        perror("Unable to open file for output redirection\n");
        exit(errno);
      }
    }

    if (environment->infile_path != NULL) {
      if(fclose(stdin) != 0) {
        perror("Unable to close stdin for redirection\n");
        exit(errno);
      };
      if (open(environment->infile_path,O_RDONLY) == -1) {
        perror("Unable to open file for input redirection\n");
        exit(errno);
      }

    }

    execvp(*args, args);
		// exec only returns if there is an error
		perror("execute");
		return(2);
		break;

	default:
		// In the parent process

    dprintf("In parent process\n");

    
    /* Wait for foreground process */
    if (!environment->run_in_background) {
      waitpid(spawnPid, &childStatus, 0);
      if (WIFEXITED(childStatus)) environment->last_fg_exit_status = WEXITSTATUS(childStatus);
      if (WIFSIGNALED(childStatus)) environment->last_fg_exit_status = 128 + WTERMSIG(childStatus);
      if (WIFSTOPPED(childStatus)) {
        kill(spawnPid, SIGCONT);
        fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t) spawnPid);
      };

      dprintf("Child process completed\n");
      return(0);
    }

    /* Record current bg process pid */
    environment->last_bg_pid = spawnPid;

    break;
	} 
  return 0;
}

int parse(char** args, struct shell_env *environment) {

  int i = 0;

  /* Look for comment mark from left to right */
  while(i < WORD_LIMIT && args[i] != NULL && (strcmp(args[i], "#") != 0)) {
    i++;
  }
  if (args[i] != NULL && (strcmp(args[i], "#") == 0)) {
    free(args[i]);
    args[i] = NULL;
  }

  /* Look for bg token '&' */
  i -= 1;

  if ((i >= 0) && (args[i] != NULL) && (strcmp(args[i], "&") == 0)) {
    dprintf("Background token detected\n");
    environment->run_in_background = true;
    free(args[i]);
    args[i] = NULL;
    i -= 1;
  }

  /* Check for input and output redirects */
  for (int x = 0; x < 2; x++) {
    if (i >= 1) {
      if ((strcmp(args[i - 1], ">") == 0) && (environment->outfile_path == NULL)) {
        environment->outfile_path = args[i];
        dprintf("Redirect output path: %s\n", environment->outfile_path);
        free(args[i-1]);
        args[i-1] = NULL;
        i -= 2;
      }
      else if ((strcmp(args[i - 1], "<") == 0) && (environment->infile_path == NULL)) {
        environment->infile_path = args[i];
        dprintf("Redirect input path: %s\n", environment->infile_path);
        free(args[i-1]);
        args[i-1] = NULL;
        i -= 2;
      }
    }
  }
  return 0;
}

int manage_bg() {
  int current_pid = 0;
  int status= 0;
  int exit_status,
      exit_signal; 

  for (;;) { 
    current_pid = waitpid(0, &status, WUNTRACED | WNOHANG);
    if (current_pid < 1) break;

    if (WIFEXITED(status)) {
        exit_status = WEXITSTATUS(status);
        fprintf(stderr, "Child process %jd done. Exit status %d.\n", (intmax_t) current_pid, exit_status);
      } 

    else if (WIFSIGNALED(status)) {
        exit_signal = WTERMSIG(status);
        fprintf(stderr, "Child process %jd done. Signaled %d.\n", (intmax_t) current_pid, exit_signal);
    }

    else if (WIFSTOPPED(status)) {
        fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t) current_pid);
        kill(current_pid, SIGCONT);
    }
  }
return current_pid;
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
  .self_pid = 0,
  .last_bg_pid = 0,
  .last_fg_exit_status = 0,
  .run_in_background = false,
  };


  /* Initialize signal handling */
  sigfillset(&environment.sh_signals);
  sigdelset(&environment.sh_signals, SIGSTOP);
  signal(SIGINT, SIG_IGN);



  /* Get PID of smallsh for later expansion */
  environment.self_pid = getpid();

  for (;;) {

    /* Manage background processes */
    manage_bg();

    /* Prompt */
    prompt = getenv("PS1");
    dprintf("PS1 Prompt: %s\n",prompt);
    if (prompt == NULL || *prompt == '\0') {
      prompt = "$\0";
    }
    fprintf(stderr,"%s", prompt);

    /* Line reading listens to sigint */
    signal(SIGINT, SIG_DFL);

    /* Read a line of input */
    read = getline(&line_ptr, &line_len, stdin);
    dprintf("Read %ld chars from line\n", read);
    if (read == -1) {
      if (feof(stdin)) exit_sh(words, &environment);
      perror("getline");
      exit(errno);     
    } else if (read == 0) {
      goto cleanup;
    } else {
    dprintf("Line input: %s\n", line_ptr);
    };

    /* Back to ignoring signals */
    signal(SIGINT, SIG_IGN);

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
    parse(words, &environment);


    /* Execute */
    if (words[0] == NULL) continue;
    execute(words, &environment);

    /* Cleanup */
cleanup:;
    for (int i=0; i < WORD_LIMIT; i++) {
      if (words[i] != NULL && *words[i] != 0) {
        free(words[i]);
        words[i] = NULL;
      }

    }
    environment.run_in_background = false;
    environment.outfile_path = NULL;
    environment.infile_path = NULL;
  }


return 0;
}
