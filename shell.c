/*
 * Aim: - to create a custom shell
 * Developed By: - Meet Shah (109875889) & Rishabh Sharma (109874160)
 * Programming Assignment: - 1
 * Subject: - CSCI 244, Operating System
 * Programming Language Used:- C
 * Compiler: - gcc
 * Input/Output :- On console Window (STDIN/STDOUT)
 * Input type:- shell command
 * Output: - execution of the shell command inputted by the user
 * Steps to execute the program: - in command prompt first execute make file by command
 * “make” then type “./shell-assignment1” for executing the custom shell inside command
 * prompt

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include "shell.h"

#include <termios.h>


/* Shared global variables */
static char **history;                /* array of strings for storing history */
static int history_len;               /* current number of items in history */
static char *input;                   /* input entered by the user */

/* useful for keeping track of parent's prev call for cleanup */
static struct command *parent_cmd;
static struct commands *parent_cmds;
static char *temp_line;
static int history_arrow;

// Description: - check whether the input command is history command or not
// Input: - pointer to the character array
// Output: - (int) 0 if the input is not history
//           (int) 1 if the input is history
int is_history_command(char *input)
{
  const char *key = "history";
  // check the length of the input string
  // if the length is not equal to 7 return 0
  if (strlen(input) < strlen(key))
  return 0;
  int i;
  // compare the string character by character with "history"
  // if match not found return 0
  for (i = 0; i < (int) strlen(key); i++) {
    if (input[i] != key[i])
    return 0;
  }
  // if above conditions fails then the first 7 character will be history
  // so return 1
  return 1;
}

// Description: - check whether space is present in the input command
// Input: - pointer to the character array
// Output: - (int) 0 if spcae is not found in the string
//           (int) 1 is space is present in the string
int is_blank(char *input)
{
  int n = (int) strlen(input);
  int i;
  // check whether space is present in the input character array by using isspace function
  // return 0 if space is not found
  for (i = 0; i < n; i++) {
    if (!isspace(input[i]))
    return 0;
  }
  // return 1 if space is present in the input string
  return 1;
}


// Description: - read the command (without pipeline) inserted by the user in the from of character array
//                separate command with the arguments present in the input string
//                by assigning the separate memory location to for command and arguments
// Input: - command without pipeline in the form of character array
// Output:- Failure if memory can not be allocated to the command
//          struct command where command is stored in name and all the arguments are stored in argc
struct command *parse_command(char *input)
{
  // tokenCount is used for calculating number of spaces in the input string
  int tokenCount = 0;
  char *token;

  // assigning the memory for command and arguments inserted by the user
  struct command *cmd = calloc(sizeof(struct command) +
  ARG_MAX_COUNT * sizeof(char *), 1);
  // exit the shell with the failure message if memory is not allocated to the command
  if (cmd == NULL) {
    fprintf(stderr, "error: memory alloc error\n");
    exit(EXIT_FAILURE);
  }

  // split the input with the spaces and create a token array
  token = strtok(input, " ");
  // store all the tokens in argv
  while (token != NULL && tokenCount < ARG_MAX_COUNT) {
    cmd->argv[tokenCount++] = token;
    token = strtok(NULL, " ");
  }
  // store first token to name field of the structure as it represent what the command is.
  cmd->name = cmd->argv[0];
  // argc contain the token count present in the input string
  cmd->argc = tokenCount;
  return cmd;
}


// Description: - read the commands (with pipeline) inserted by the user in the from of character array
//                separate each command with the pipeline and assign a cmds struct each command
// Input: - commands with pipeline in the form of character array
// Output:- Failure if memory can not be allocated to the commands
//          struct commands array where commands contain all the list of input command inserted by the user using pipeline
struct commands *parse_commands_with_pipes(char *input)
{
  // contain total number of command separated by pipeline
  int commandCount = 0;
  int i = 0;
  char *token;
  char *saveptr;
  char *c = input;
  // contain list of all the commands
  struct commands *cmds;
  // calculate the toal number of the commands
  while (*c != '\0') {
    if (*c == '|')
    commandCount++;
    c++;
  }
  // increment counter for getting n + 1 commands seperated by n pipelines
  commandCount++;
  // assign the memory location for each command
  cmds = calloc(sizeof(struct commands) +
  commandCount * sizeof(struct command *), 1);
  // exit the shell with the failure message if memory is not allocated to the command
  if (cmds == NULL) {
    fprintf(stderr, "error: memory alloc error\n");
    exit(EXIT_FAILURE);
  }
  // create a array of individual command seperated by pipeline and store them in
  token = strtok_r(input, "|", &saveptr);
  while (token != NULL && i < commandCount) {
    cmds->cmds[i++] = parse_command(token);
    token = strtok_r(NULL, "|", &saveptr);
  }
  // cmd_count contains the count of total number of commands inserted by the user using pipeline
  cmds->cmd_count = commandCount;
  return cmds;
}


// Description: - checks whether the commmand is build-in command or not.
//                list of build-in commands for our shell :-
//                                                          exit, cd
//                                                          history
// Input: - command structure
// Output:- (int) 0 if the command name matches the command present in build-in
//          (int) 1 if the command name does not matches the command present in build-in
int check_built_in(struct command *cmd)
{
  return strcmp(cmd->name, "exit") == 0 ||
  strcmp(cmd->name, "cd") == 0 ||
  strcmp(cmd->name, "history") == 0;
}

// Description: - Responsible for handling the build-in history command in the shell
// Input: - all commands seperated using the pipeline and a single command in case of just history is inputted by the user
// Output:- (int) 0 if history command is handled succesfully
//          (int) 1 if history command is not handled succesfully
int handle_history(struct commands *cmds, struct command *cmd)
{
  // if only one command is inserted without using pipeline
  if (cmd->argc == 1) {
    int i;
    // print the history of the previous commands in the console
    for (i = 0; i < history_len ; i++) {
      printf("%d %s\n", i, history[i]);
    }
    return 1;
  }
  // check the argumemt values present in the history
  // and perform the related operation to the argument
  if (cmd->argc > 1) {
    if (strcmp(cmd->argv[1], "-c") == 0) {
      // clear the history from the shell if the argument to the history command is -c
      clear_history();
      return 0;
    }

    // exec command from history
    char *end;
    long loffset;
    int offset;
    // check if the argument present is a valid number or not
    loffset = strtol(cmd->argv[1], &end, 10);
    if (end == cmd->argv[1]) {
      fprintf(stderr, "error: cannot convert to number\n");
      return 1;
    }

    offset = (int) loffset;
    if (offset > history_len) {
      fprintf(stderr, "error: offset > number of items\n");
      return 1;
    }

    // parse execute command
    char *line = strdup(history[offset]);

    if (line == NULL)
    return 1;

    struct commands *new_commands = parse_commands_with_pipes(line);

    // set pointers so that commands in pipeline can be freed when their
    // child processes die during execution
    parent_cmd = cmd;
    temp_line = line;
    parent_cmds = cmds;
    // execute commands in pipeline
    exec_commands(new_commands);
    // free the memory once the execution of the command is completed and command is no longer required
    cleanup_commands(new_commands);
    free(line);

    // reset the pointer values once the execution of rhe commmands is completed
    parent_cmd = NULL;
    temp_line = NULL;
    parent_cmds = NULL;

    return 0;
  }
  return 0;
}


// Description: - handle the build in commands
//                list of build-in commands for our shell :-
//                                                          exit, cd
//                                                          history
// Input: - all commands seperated using the pipeline and a single command in case of command is inputted by the user without pipeline
// Output:- (int) 0 if command is handled succesfully
//          (int) 1,-1 if command is not handled succesfully
int handle_built_in(struct commands *cmds, struct command *cmd)
{
  int ret;
  // check if the inputted commmand is exit
  if (strcmp(cmd->name, "exit") == 0)
  return -1;
  // check if the inputted command is cd
  if (strcmp(cmd->name, "cd") == 0) {
	char p[1024];
	getcwd(p,1024);
	sprintf(p,"%s/%s",p,cmd->argv[1]);
	ret = chdir(p);
    if (ret != 0) {
      fprintf(stderr, "error: unable to change dir\n");
      return 1;
    }
    return 0;
  }
  // check if the inputted command is history
  if (strcmp(cmd->name, "history") == 0)
  return handle_history(cmds, cmd);
  return 0;
}


// Description: - free the memory by clearing the history
// Input: - void
// Output:- (int) 0 if command is handled succesfully
int clear_history(void)
{
  int i;

  for (i = 0; i < history_len; i++)
    free(history[i]);
  history_len = 0;
  return 0;
}


// Description: - Adds the user's input to the history. Limit is = 100
// Input: - user inputted command in the form of the character array.
// Output: - (int) 1 if the function execution is succesful
//           (int) 0 if the function execution is unsuccesful
int add_to_history(char *input)
{
  // If No history of commands is present
  // then, create the memory location for initializing the history
  if (history == NULL) {
    history = calloc(sizeof(char *) * HISTORY_MAXITEMS, 1);
    if (history == NULL) {
      fprintf(stderr, "error: memory alloc error\n");
      return 0;
    }
  }

  /* make a copy of the input */
  char *line;

  line = strdup(input);
  if (line == NULL)
  return 0;

  // when max items have been reached, move the old
  // contents to a previous position, and decrement len
  if (history_len == HISTORY_MAXITEMS) {
    free(history[0]);
    int space_to_move = sizeof(char *) * (HISTORY_MAXITEMS - 1);

    memmove(history, history+1, space_to_move);
    if (history == NULL) {
      fprintf(stderr, "error: memory alloc error\n");
      return 0;
    }

    history_len--;
  }

  history[history_len++] = line;
  history_arrow = history_len;
  return 1;
}

// Description: - Executes a command by forking of a child and calling execevp command which will use PATH variable.
//                Causes the calling progress to halt until the child is done executing.
// Input: - user inputted command, commands seperated by pipeline
// Output: - (int) 1 if the function execution is succesful
//           (int) 0 if the function execution is unsuccesful
int exec_command(struct commands *cmds, struct command *cmd, int (*pipes)[2])
{
  // check if the command is build in command
  if (check_built_in(cmd) == 1)
    return handle_built_in(cmds, cmd);

  pid_t child_pid = fork();
  // if the creation of the child process fails return with 0
  if (child_pid == -1) {
    fprintf(stderr, "error: fork error\n");
    return 0;
  }

  // execute the child process upon its sucessful creation
  if (child_pid == 0) {
    // file descriptor 0 for read
    // file descriptor 1 for writes
    int input_fd = cmd->fds[0];
    int output_fd = cmd->fds[1];

    // change input/output file descriptors if they aren't standard
    if (input_fd != -1 && input_fd != STDIN_FILENO)
      dup2(input_fd, STDIN_FILENO);

    if (output_fd != -1 && output_fd != STDOUT_FILENO)
      dup2(output_fd, STDOUT_FILENO);

    if (pipes != NULL) {
      int pipe_count = cmds->cmd_count - 1;

      close_pipes(pipes, pipe_count);
    }

    /* execute the command */
    execvp(cmd->name, cmd->argv);

    /* execv returns only if an error occurs */
    fprintf(stderr, "error: %s\n", strerror(errno));

    /* cleanup in the child to avoid memory leaks */
    clear_history();
    free(history);
    free(pipes);
    free(input);
    cleanup_commands(cmds);
    if (parent_cmd != NULL) {
      free(parent_cmd);
      free(temp_line);
      free(parent_cmds);
    }
    /* exit from child so that the parent can handle the scenario*/
    _exit(EXIT_FAILURE);
  }
  /* parent continues here */
  return child_pid;
}

// Description: - close the pipe once the execution of the command is completed
// Input: - pointer to the parent and child pipe, total number of pipes been used
// Output: - close the pipes upon execution
void close_pipes(int (*pipes)[2], int pipe_count)
{
  int i;

  for (i = 0; i < pipe_count; i++) {
    close(pipes[i][0]);
    close(pipes[i][1]);
  }

}


// Description: - Executes a set of commands that are piped together.
//                If it's a single command, it simply calls `exec_command`.
// Input: - structur of the commands inputted by the user
// Output: - (int) 1 if the function execution is succesful
//           (int) 0 if the function execution is unsuccesful
int exec_commands(struct commands *cmds)
{
  int exec_ret = 0;

  // if only single command is inputted then execute the command
  if (cmds->cmd_count == 1) {
    cmds->cmds[0]->fds[STDIN_FILENO] = STDIN_FILENO;
    cmds->cmds[0]->fds[STDOUT_FILENO] = STDOUT_FILENO;
    exec_ret = exec_command(cmds, cmds->cmds[0], NULL);
    wait(NULL);
  } else {
    /* execute a pipeline */
    int pipe_count = cmds->cmd_count - 1;

    /* if any command in the pipeline is a built-in, raise error */
    int i;

    for (i = 0; i < cmds->cmd_count; i++) {
      if (check_built_in(cmds->cmds[i])) {
        fprintf(stderr, "error: no builtins in pipe\n");
        return 0;
      }

    }

    /* allocate an array of pipes. Each member is array[2] */
    int (*pipes)[2] = calloc(pipe_count * sizeof(int[2]), 1);

    if (pipes == NULL) {
      fprintf(stderr, "error: memory alloc error\n");
      return 0;
    }


    /* create pipes and set file descriptors on commands */
    cmds->cmds[0]->fds[STDIN_FILENO] = STDIN_FILENO;
    for (i = 1; i < cmds->cmd_count; i++) {
      pipe(pipes[i-1]);
      cmds->cmds[i-1]->fds[STDOUT_FILENO] = pipes[i-1][1];
      cmds->cmds[i]->fds[STDIN_FILENO] = pipes[i-1][0];
    }
    cmds->cmds[pipe_count]->fds[STDOUT_FILENO] = STDOUT_FILENO;

    /* execute the commands */
    for (i = 0; i < cmds->cmd_count; i++)
      exec_ret = exec_command(cmds, cmds->cmds[i], pipes);

    close_pipes(pipes, pipe_count);

    /* wait for children to finish */
    for (i = 0; i < cmds->cmd_count; ++i)
      wait(NULL);

    free(pipes);
  }
  return exec_ret;
}


// Description: - Frees up memory for the commands
// Input: - structure of the commands inputted by the user
// Output: - free the memory space occupued by commands
void cleanup_commands(struct commands *cmds)
{
  int i;
  for (i = 0; i < cmds->cmd_count; i++){
      free(cmds->cmds[i]);
  }
  free(cmds);
}


// Description: - cleans up history before exits
// Input: - status if the command execute succesfully or not
//          0 program execution fails and shell should exit now
//          1 program execution is sucessful and ready to exity now
// Output: - free the memory space occupued by history before exiting
void cleanup_and_exit(int status)
{

  clear_history();
  free(history);
  exit(status);
}



// Description: - get the character without echoing it back to the prompt.
// Input: - void
// Output: - character inputted by the user
int getch(void)
{
  struct termios oldattr, newattr;
  int ch;
  tcgetattr( STDIN_FILENO, &oldattr );
  newattr = oldattr;
  newattr.c_lflag &= ~( ICANON | ECHO );
  tcsetattr( STDIN_FILENO, TCSANOW, &newattr );
  ch = getchar();
  tcsetattr( STDIN_FILENO, TCSANOW, &oldattr );
  return ch;
}


// Description: - get the character and echo it back to the prompt.
// Input: - void
// Output: - character inputted by the user and echoed in the console window
int getche(void)
{
  struct termios oldattr, newattr;
  int ch;
  tcgetattr( STDIN_FILENO, &oldattr );
  newattr = oldattr;
  newattr.c_lflag &= ~( ICANON );
  tcsetattr( STDIN_FILENO, TCSANOW, &newattr );
  ch = getchar();
  tcsetattr( STDIN_FILENO, TCSANOW, &oldattr );
  return ch;
}

// Description: - read the input provided by the user on the console
// Input: - void
// Output: - pointer to the character array inputted by the user.
char *read_input(void)
{
  int buffer_size = 2048;
  char *input = malloc(buffer_size * sizeof(char));
  int i = 0;
  char c;

  if (input == NULL) {
    fprintf(stderr, "error: malloc failed\n");
    cleanup_and_exit(EXIT_FAILURE);
  }

  while ((c = getch()) != '\n') {
    if(c == 65 && input[i-2] == '\033')//up arrow
    {
      //printf("Up arrow\r\n");
      return 1;
    }
    else if(c == 66 && input[i-2] == '\033')//down arrow
    {
      //printf("down arrow\r\n");
      return 2;
    }
    if(c != '\033' && input[i-1] != '\033')
      putchar(c);
    /* did user enter ctrl+d ?*/
    if (c == EOF) {
      free(input);
      return NULL;
    }

    /* allocate more memory for input */
    if (i >= buffer_size) {
      buffer_size = 2 * buffer_size;
      input = realloc(input, buffer_size);
    }

    input[i++] = c;
  }
  putchar('\n');
  input[i] = '\0';
  return input;
}

// Description: - Execution of the Shell begins from here.
//                Main is responsible for the executing the shell in the proper order.
// Input: - void
int main(void)
{

  int exec_ret = 0;
  char p[1024];
  history_arrow = 0;
  while (1) {

    getcwd(p,1024);
    sprintf(p,"%s>",p);
    fputs(p, stdout);

    input = read_input();
    if (input == 1)
    {
      //free(linecopy);
      if(history_arrow != 0)
      {
        history_arrow--;
      }
      //printf("\r\n");
      //getcwd(p,1024);
      //sprintf(p,"%s>",p);
      //fputs(p, stdout);
      //fputs(history[history_arrow],stdout);
      if(history_len !=0)
        printf("%s\n",history[history_arrow]);
      //fputs(history[history_arrow],stdin);

      //free(input);
      continue;
    }
    if (input == 2)
    {
      if(history_arrow < history_len-1)
      {
        history_arrow++;
      }
      //getcwd(p,1024);
      //sprintf(p,"%s>",p);
      //fputs(p, stdout);
      if(history_len!=0)
      printf("%s\n",history[history_arrow]);
      //fputs(history[history_arrow],stdout);
      //fputs(history[history_arrow],stdin);

      continue;
    }
    if (input == NULL) {
      /* user entered ctrl+D, exit gracefully */
      cleanup_and_exit(EXIT_SUCCESS);
    }

    if (strlen(input) > 0 && !is_blank(input) && input[0] != '|') {
      char *linecopy = strdup(input);

      struct commands *commands =
      parse_commands_with_pipes(input);

      /* add pipeline cmds & other commands to history */
      if (commands->cmd_count > 1
        || !is_history_command(input))
        add_to_history(linecopy);

        free(linecopy);
        exec_ret = exec_commands(commands);
        cleanup_commands(commands);
      }

      free(input);

      /* get ready to exit */
      if (exec_ret == -1)
      break;
    }

    /* perform cleanup to ensure no leaks */
    cleanup_and_exit(EXIT_SUCCESS);
    return 0;
  }
