#include "shellLogic.h"

int main()
{
   int status = 1;
   commandChain *commands;

   setbuf(stdout, NULL);

   while (status) {
      commands = calloc(1, sizeof (commandChain));
      commands->numChildren = 0;
      shell_loop(commands, &status);
      freeCommand(commands);
   }

   return 0;
}
