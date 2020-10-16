<br><img src="./images/shell.png" width="10%" alt = "image" align = "left"/> Welcome! Want to write your own shell, but don't know where to start? Use my open source shell and add your own ideas. Perhaps you can find simpler algorithms for what is already described in my program. I look forward to any of your comments!
<cut />
<br clear = "left">

### This shell can:
 * Standard launch of the program;
 * Redirect the input stream to a file;
 * Redirect input and output:
     - Redirect the output stream to a file;
     - Read from file;
     - Reading and writing to a file, writing and reading from a file in one run;
 * Run "&&" pipeline for multiple commands.
 * Run "|" pipeline for commands with redirection of input and output streams;
 * Change directory using `cd` command:
     - `cd` and `cd ~` to go to home directory;
     - `cd directory_name` to go to the specified directory (`cd ..` to go to the parent directory);
     - `cd -` to go to the previous directory;
 * Run programs in the background;
 * Ctrl + C.

### How to compile:

 - Download "sources" and Makefile;
 - Press the key combination Ctrl + Alt + T;
 - Use the `cd` command to go to the directory where these files are stored. Pass the path to the required directory as a parameter;
 - Use the `make` command to compile.
<p align="center">
<img src="https://github.com/Valeriya-avt/shell/blob/master/images/Compile.gif" width="80%"></p>

### How to run:

 - `cd bin`
 - `./main` ... Yahoo! Shell is ready to go!
<p align="center">
<img src="https://github.com/Valeriya-avt/shell/blob/master/images/Run%20.gif" width="80%"></p>
 
### How to use:
 First, let's look at the contents of the current directory. To do this, we will use the command **ls**.
<p align="center">
<img src="https://github.com/Valeriya-avt/shell/blob/master/images/sl1.gif" width="80%"></p>
 Oops! Well... Now we know that the <del>most important</del> funniest command can work!
 We know where to find a locomotive. Let's travel through directories!
 - Let's go to the parent directory using `сd` and using `pwd` to check that we really are in the parent directory:
<p align="center">
<img src="https://github.com/Valeriya-avt/shell/blob/master/images/parent_dir%20and%20pwd.gif" width="80%"></p>
 - Let's try using a combination of `cd` (or `cd ~`) and `cd -` to move between the current and home directories:
<p align="center">
<img src="https://github.com/Valeriya-avt/shell/blob/master/images/home_dir%20and%20cd%20-%20.gif" width="80%"></p>
 Very well! We can move from one directory to another!
 
 Before choosing the next destination, let's look at the contents of the current directory using the `ls` command and go to any directory we find.
<p align="center">
<img src="https://github.com/Valeriya-avt/shell/blob/master/images/ls%2C%20cd%20test.gif" width="80%"></p>

 Let's create some files, write to the file only what we are looking for, and display it on the screen using 
`ls > file.txt && grep -r .c < file.txt`

 Let's check how the "|" pipeline works:
 1. `ls -l | grep .txt`
 2. `ls | sort | grep c` and `ls | grep c | sort`
 3. `sleep 10 | ls | sort`
