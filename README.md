# SmallSH - A Minimalistic Shell

SmallSH is a simple and minimalistic Unix-like shell implementation in C. It provides basic shell functionality, including executing commands, managing processes, and handling input/output redirection. I wrote it to teach myself about process management and basic shell operation.

## Features

- Command execution.
- Input and output redirection.
- Background and foreground processes.
- Built-in shell commands (cd, status, exit).
- Signal handling (SIGINT and SIGTSTP).

## Getting Started

To get started with SmallSH, you can clone this repository:

```bash
git clone https://github.com/augustsunday/smallsh.git
```

### Prerequisites

- C Compiler (gcc recommended)
- Linux-based operating system (tested on Ubuntu)

### Compilation

There is a makefile included with the following options:
* all - Compile both debug and release
* clean - Delete contents of debug and release directories, leaving folders intact
* prep - Create debug and release directories, but does not compile
* debug - Compile debug only ()
* release - Compile release only

To get a release ready executable simply do
```bash
make release
```

### Compilation Option(s):
By default smallsh has a limit of 512 'words' per line.
<br>You can change this during compilation by changing the WORD_LIMIT definition

### Usage

You can run SmallSH by executing the compiled binary:

```bash
./release/smallsh
```

Once SmallSH is running, you can enter commands and use various shell features.

## Examples

Here are some examples of using SmallSH:

- Running a command in the foreground:
  ```bash
  $ ls -l
  ```

- Running a command in the background:
  ```bash
  $ sleep 10 &
  ```

- Redirecting input and output:
  ```bash
  $ ls -l > output.txt
  ```

- Using built-in commands:
  ```bash
  $ cd /path/to/directory
  ```

- Exiting SmallSH:
  ```bash
  $ exit
  ```

## Expansion
### Smallsh performs expansion of the following tokens 
* “~/” at the beginning of a word will be replaced with the value of the HOME environment variable. The "/" will be retained.
* “$$” within a word will be replaced with the proccess ID of the smallsh process.
* “$?” within a word will be replaced with the exit status of the last foreground command.
* “$!” within a word will be replaced with the process ID of the most recent background process.
### Defaults
* Unset environment variables will be expanded to an empty string
* "$?" defaults to "0"
* "$!" defaults to an empty string if no background processes have been executed

## Parsing
Smallsh parses input into tokens and inteprets them as follows...
* Everything after the first "#" is interepreted as a comment
* A "&" at the end of line (excepting comments as above) runs the command in the background
* Infile and outfile tokens must appear after all arguments, but before a background token
* Valid command lines take one of two forms:
```bash
[command] [arguments...] [> outfile] [< infile] [&] [# [comment...]]
```
or
```bash
[command] [arguments...] [< infile] [> outfile] [&] [# [comment...]]
```
If after parsing a valid command word is not present smallsh will simply re-prompt without returning an error

## License




