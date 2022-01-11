#include "pipeLogic.h"

void perrorAndExit()
{
   perror(NULL);
   exit(EXIT_FAILURE);
}

void closeFd2(information *info){
   close(info->fd2[0]);
   close(info->fd2[1]);
}

int redirectIn(char *fileName)
{
   int inFD = open(fileName, O_RDONLY);
   dup2(inFD, STDIN_FILENO);
   close(inFD);
   return 0;
}

void redirectOut(char *fileName)
{
   int outFD = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
   dup2(outFD, STDOUT_FILENO);
   close(outFD);
}

void createFirstChild(information *info){
   close(info->fd1[0]);
   closeFd2(info);
   info->readFD = 0;
   info->writeFD = info->fd1[1];
}

void createMiddleChild(information *info, int i)
{
   if(i % 2 == 1) {
      close(info->fd2[0]);
      info->readFD = info->fd1[0];
      info->writeFD = info->fd2[1];
      return;
   }

   close(info->fd1[0]);
   info->readFD = info->fd2[0];
   info->writeFD = info->fd1[1];
}

void createLastChild(information *info)
{
   if (info->numberOfChildren  % 2 == 0) {
      close(info->fd1[1]);
      info->readFD = info->fd1[0];
      info->writeFD = 1;
      return;
   }

   close(info->fd1[0]);
   close(info->fd2[1]);
   info->readFD = info->fd2[0];
   info->writeFD = 1;
}

void updatePipes(information *info, int i)
{
   if(i % 2 == 0 && i + 1 != info->numberOfChildren - 1) {
      closeFd2(info);
      close(info->fd1[1]);
      pipe(info->fd2);
   }
   else if(i + 1 != info->numberOfChildren - 1 &&
      i != info->numberOfChildren - 1)
   {
      close(info->fd1[0]);
      close(info->fd2[1]);
      pipe(info->fd1);
   }
}

void determineWhichChildToAdd(information *info, int i,
   command currentCommand, int *status, int moreThanOneChild)
{
   /* Create pipes */
   if (i == 0)
      createFirstChild(info);
   else if (i == info->numberOfChildren - 1)
      createLastChild(info);
   else
      createMiddleChild(info, i);

   /* Handle Redirection In */
   if (currentCommand.redirectIn)
      redirectIn(currentCommand.redirectIn);
   else
      dup2(info->readFD, STDIN_FILENO);

   /* Handle Redirection Out */
   if (currentCommand.redirectOut)
      redirectOut(currentCommand.redirectOut);
   else if (moreThanOneChild)
      dup2(info->writeFD, STDOUT_FILENO);

   /* Execute command */
   if (execvp(currentCommand.arguments[0], currentCommand.arguments) < 0) {
      fprintf (stderr, "cshell: %s: ", currentCommand.arguments[0]);
      perrorAndExit();
   }
}
