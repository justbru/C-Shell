#include "shellLogic.h"
#include "pipeLogic.h"
#include <stdio.h>

command* makeCommand()
{
   command *commands = calloc(1, sizeof (command));
   commands->redirectIn = NULL;
   commands->redirectOut = NULL;
   return commands;
}

void freeCommand(commandChain *commands)
{
   int i;
   for (i = 0; i <= commands->numChildren; i++)
      free(commands->commands[i]);
   free(commands);
}

int check_line_length(int encounteredError)
{
   if (encounteredError) {
      fprintf(stderr, "cshell: Command line too long\n");
      return 1;
   }
   return 0;
}

int check_arg_length(int numPipes)
{
   if (numPipes >= MAX_ARGS_PER_PIPE){
      fprintf(stderr, "cshell: Too many commands\n");
      return 1;
   }
   return 0;
}

int check_args_length(int numArgs, char *command)
{
   if (numArgs > MAX_ARGS + 1){
      fprintf(stderr, "cshell: %s: Too many arguments\n", command);
      return 1;
   }
   return 0;
}

int ensureCorrectCommandHelper(char *command, char *currentArg,
   char *prevArg, int *numPipes, int *locationInArgs)
{
   if (check_arg_length(*numPipes) ||
      check_args_length(*locationInArgs, command))
      return 1;

   if (strcmp(currentArg, "|") == 0 && !prevArg){
      fprintf(stderr, "cshell: Invalid pipe\n");
      return 1;
   }
   return 0;
}

int checkInvalidPipe(char *currentArg, char *prevArg)
{
   if (strcmp(prevArg, "|") == 0 && !currentArg)
   {
      fprintf(stderr, "cshell: Invalid pipe\n");
      return 1;
   }
   return 0;
}

int ensureCorrectCommand(char *input, int *status)
{
   char *currentArg = strtok(input, " ");
   int locationInArgs = 0;
   char *prevArg = NULL;
   int numPipes = 0;
   char *command;

   if (strcmp(currentArg, "exit") == 0) {
      *status = 0;
      return 1;
   }

   while (currentArg)
   {
      if (locationInArgs == 0)
         command = currentArg;

      if (strcmp(currentArg, "|") == 0 && ++numPipes)
         locationInArgs = 0;
      else
         locationInArgs++;

      if (ensureCorrectCommandHelper(command, currentArg,
         prevArg, &numPipes, &locationInArgs))
         return 1;

      prevArg = currentArg;
      currentArg = strtok(NULL, " ");

      if (checkInvalidPipe(currentArg, prevArg))
         return 1;
   }
   return 0;
}

void launchChild(information *info, int i, command currentCommand,
   int *status, int moreThanOneChild){
   pid_t pid;

   if (-1 == (pid = fork()))
      perrorAndExit();
   else if (pid == 0)
      determineWhichChildToAdd(info, i, currentCommand,
         status, moreThanOneChild);
   else
      updatePipes(info, i);
}

int testOpenFile(char *fileName)
{
   int inFD = open(fileName, O_RDONLY);
   if (inFD < 0)
   {
      fprintf(stderr, "cshell: %s: No such file or directory\n", fileName);
      return 1;
   }
   return 0;
}

int runLoopHelper(commandChain *commands, char *token, int *locationInArgs)
{
   if (strcmp(token, ">") == 0)
      commands->commands[commands->numChildren]->redirectOut =
         strtok(NULL, " ");
   else if (strcmp(token, "<") == 0) {
      if (testOpenFile(token = strtok(NULL, " ")))
         return 1;
      commands->commands[commands->numChildren]->redirectIn = token;
   }
   else if (strcmp(token, "|") == 0) {
      (commands->numChildren)++;
      commands->commands[commands->numChildren] = makeCommand();
      *locationInArgs = 0;
   }
   else{
      commands->commands[commands->numChildren]->
         arguments[*locationInArgs] = token;
      (*locationInArgs)++;
   }
   return 0;
}

int runLoop(commandChain *commands, char *input)
{
   int locationInArgs = 0;
   char *token = strtok(input, " ");

   commands->commands[commands->numChildren] = makeCommand();

   while (token) {
      if (runLoopHelper(commands, token, &locationInArgs))
         return 1;

      token = strtok(NULL, " ");
   }
   return 0;
}

void shell_loop(commandChain *commands, int *status) {
   char *input;
   char *inputDup;
   int encounteredError = 0;
   int hitEnter = 0;
   information *info;

   fprintf(stdout, ":-) ");

   input = read_line(status, &encounteredError, &hitEnter);
   check_line_length(encounteredError);
   if (*status == 0 || hitEnter || encounteredError) {
      free(input);
      return;
   }

   /* might not need */
   input[strlen(input)] = '\0';

   inputDup = calloc(1, MAX_LINE_LENGTH);
   memcpy(inputDup, input, MAX_LINE_LENGTH);

   if (ensureCorrectCommand(inputDup, status)) {
      free(inputDup);
      free(input);
      return;
   }

   free(inputDup);

   if (runLoop(commands, input)) {
      free(input);
      return;
   }

   info = malloc(sizeof (information));
   info->numberOfChildren = commands->numChildren + 1;

   if (pipe(info->fd1) < 0 || pipe(info->fd2))
      perrorAndExit();

   for(hitEnter = 0; hitEnter < info->numberOfChildren; hitEnter++)
      launchChild(info, hitEnter, *commands->commands[hitEnter],
         status, commands->numChildren);

   close(info->fd1[0]);
   close(info->fd1[1]);
   closeFd2(info);

   for (hitEnter = 0; hitEnter < info->numberOfChildren; hitEnter++)
      wait(NULL);

   free(info);
   free(input);

   return;
}

int read_line_check_EOF_or_Enter(int c, int position,
   int *status, int *hitEnter)
{
   if (c == EOF && position == 0){
      fprintf(stdout, "exit\n");
      *status = 0;
      return 1;
   }
   if (c == '\n' && position == 0) {
      *hitEnter = 1;
      return 1;
   }
   return 0;
}

int read_line_add_to_line(int c, char *readLine,
   int *encounteredError, int *position)
{
   if (c == EOF || c == 10) {
      if (*position < 1024) {
         readLine[*position] = '\0';
         return 1;
      }
      *encounteredError = *position > 1024 ? 1 : 0;
      return 1;
   }
   else if (*position < 1024 && isprint(c))
      readLine[*position] = c;

   (*position)++;

   return 0;
}

char* read_line(int* status, int* encounteredError, int *hitEnter)
{
   int c = 0;
   int position = 0;
   char *readLine = calloc(MAX_LINE_LENGTH, sizeof (char));

   while (c != EOF && c != 10){
      c = getchar();
      if (read_line_check_EOF_or_Enter(c, position, status, hitEnter))
         break;

      if (read_line_add_to_line(c, readLine, encounteredError, &position))
         break;
   }
   return readLine;
}
