# C-Shell

For this project, I created a basic shell in C. The shell handles any of the basic shell commands including vim, cat, grep, ls, cd, rm, etc. In order to handle these commands, the shell implements pipeline logic and only utilizes two active pipelines at any given time. The shell can also handle redirection of both STDIN and STDOUT.

## Installation

Shell.out should be able to run in command line via the comman ./shell.out. If not, compile all three *.c files and run from there

## Functionality
Command | Function
------------ | -------------
ls | shows all of the files in the current directory
cd 'directory name' | change to specified directory or home if none specified
rm 'filename' | remove the specified file
cat 'filename' | view the specified file
vim 'filename' | edit existing file or create new file with name 'filename'
< 'filename' | redirect STDIN to 'filename'
\> 'filename' | redirect STDOUT to 'filename'
head -n 'filename' | Used to print the first N lines of 'filename'
tail -n 'filename' | Used to print the last N lines of 'filename'
grep 'string' 'filename' | searches for 'string' in 'filename'


## Contributing
Please Contact for pull-requests. There will be no push requests.
