# Testing and Debugging Lab 2

In order to test your submission properly, you need to think like someone who is trying to break it.  In other words, there are two types of tests you'll want to write:
- basic 1+1=2 tests, verifying that each of the (few) functions your shell performs are working
- diabolical tests that try to provoke your code into using NULL pointers and crashing

Note - for some of these tests you may be running the `shell36` executable a whole bunch of times. The `-fsanitize-address` compile option causes it to take about a second to start up, but you'll probably save more time in the long run if you keep it enabled.

I would suggest that you automate your tests, and check them into your repository in files named `test.sh` (batch-mode tests) and optionally `test.txt` (interactive tests to be copy-and-pasted into a terminal window.) We may be a little more lenient in grading if you can demonstrate that you did a fairly good job testing your code.

### Test cases - step 1 

Start your shell in interactive mode, verify that `^C` doesn't kill it.

### Test cases - step 2

Note that a lot of these tests mention checking the value of `$?` **inside** your shell - obviously you'll have to defer this until step 4 when you implement that. (and you'll need external commands from step 3, so you can use `echo $?` to see its value) 

But at this stage you can test `$?` **outside** of your shell, verifying that you called `exit()` with the right argument. (see [Automating tests](#automating-tests) for more details on the syntax here)
```
./shell36 <<EOF
exit 5
EOF
rv=$?
if [ $rv != 5 ] ; then
  echo FAILED
fi
```
(note: You might want to get in the habit of saving the value of `$?` immediately after running your program under test, as any additional commands in your test script - even `echo` - will reset it.)

- **empty line:** should not crash, should prompt for next command (in interactive mode)
- **end-of-file/^D:** A control-D character on the console should cause your shell to exit gracefully, i.e. with `$?` set to 0
- **cd/pwd:** 
    - no argument: `cd` with no should change (as reported by `pwd` ) to your home directory, $HOME. Both should set `$?` to zero.
	- valid arg: `cd /tmp` (or other real directory) should work, as reported by `pwd`, set `$?` to zero
	- invalid: `cd /not-a-directory` should print `cd: No such file or directory` and set `$?` to 1
	- extra args: `cd a b` should print `cd: wrong number of arguments` and set `$?` to 1
- **exit**
    - `exit` should exit, with `$?` set to 0
	- `exit 7` (or whatever) should exit, with `$?` set to 7
	- `exit 1 2` should print `exit: too many arguments` and set `$?` to 1
	
If you're writing a long test script, checking `$?` after every time you run your shell is a good idea - if nothing else verify that it's 0 or 1, as a segfault will give a much different value:
```
hw1$ sh
hw1$ ./segfault
Segmentation fault (core dumped)
hw1$ rv=$?
hw1$ echo $rv
139
hw1$ if [ $rv = 0 -o $rv = 1 ] ; then
>   echo OK
> else
>   echo FAILED
> fi
FAILED
hw1$ 
```

### Tests - step 3

- verify that you can run a few simple commands - e.g. `ls`, `echo a b c`, `/bin/ls` etc.
- check that it fails correctly on bogus commands, e.g.
```
$ this-is-not-a-command
this-is-not-a-command: No such file or directory
$ ./this-is-not
./this-is-not: No such file or directory
```

### Tests - step 4

- Go back to the tests for step 2 and verify the status after each command
- verify that you handle commands returning a status of 0, 1, and another value, and report a status of 1 for command not found:
```
$ true
$ echo $?
0
$ false
$ echo $?
1
$ sh -c 'exit 7'
$ echo $?
7
$ not-a-command
not-a-command: No such file or directory
$ echo $?
1
$
```

### Tests - step 5

First you'll need to test that you're setting up pipes correctly. Test multiple lengths, so that you're testing commands that have pipes as input, or as output, or as both:
```
echo foo | cat
echo foo | cat | cat
```
If you're writing a test script, you can redirect the output of your shell into a file and verify that the output is correct:
```
./shell36 > out <<EOF
echo foo | cat
EOF
if [ $? = 0 -a "$(cat out)" = foo ] ; then
  echo OK
fi
./shell36 > out <<EOF
echo foo | cat | cat
EOF
if [ $? = 0 -a "$(cat out)" = foo ] ; then
  echo OK
fi
```

Now you'll need to test that you handle bizarre combinations. I don't care how you handle them, as long as your shell doesn't crash:
```
|
| |
| | echo
echo |
echo | |
```

It's actually quite a pain handling all of these cases correctly. A suggestion: if you have logic that creates a sequence of (start,len) pairs, but it generates some zero-length pairs, it may be easier to copy everything to a new sequence and filter out the zeroes while you do that.

### Tests - step 6

You'll need to test the "normal" cases, i.e. that:
- input and output redirection work for a single command
- input redirection works at the start of a pipeline 
- output redirection works at the end of a pipeline 

plus some moderately abnormal cases:
- a pipe stage with both piped and redirected input (e.g. `ls | cat < /dev/null`)
- same, but with redirected output (e.g. `ls > /dev/null | cat`)
In each case it doesn't matter which of the two inputs or outputs are used, but you can't crash and the commands have to run.

and finally some really abnormal cases:
- multiple `> file` or `< file` for a single command
- one and multiple '>' or '<' without files
- '> file', '< file', '>' and '>' (and combinations) on a line with no commands

Finally, you need to check that you're not "leaking" open file descriptors. The easiest way to do this is to run your shell in one terminal window, running a number of commands with pipes and redirected I/O, then go to another window, find the process ID of your program with `ps`, and use the `lsof` utility to list its open files:
```
hw1$ ps aux |grep shell36
cs5600     58863  0.0  0.0   2196  1152 pts/1    S+   15:52   0:00 ./shell36
cs5600     58865  0.0  0.0   8492  2048 pts/3    S+   15:52   0:00 grep --color=auto shell36
hw1$ lsof -p 58863
COMMAND   PID   USER   FD   TYPE DEVICE SIZE/OFF    NODE NAME
shell56 58863 cs5600  cwd    DIR  259,2     4096  918097 /home/cs5600/cs3650-s25/hw2
shell56 58863 cs5600  rtd    DIR  259,2     4096       2 /
shell56 58863 cs5600  txt    REG  259,2   107968  917589 /home/cs5600/cs3650-s25/hw2/shell36
shell56 58863 cs5600  mem    REG  259,2  1641496 2359916 /usr/lib/aarch64-linux-gnu/libc.so.6
shell56 58863 cs5600  mem    REG  259,2   187776 2359751 /usr/lib/aarch64-linux-gnu/ld-linux-aarch64.so.1
shell56 58863 cs5600    0u   CHR  136,1      0t0       4 /dev/pts/1
shell56 58863 cs5600    1u   CHR  136,1      0t0       4 /dev/pts/1
shell56 58863 cs5600    2u   CHR  136,1      0t0       4 /dev/pts/1
```
Those three lines at the bottom are the three file descriptors: 0, 1 and 2, i.e. standard input, output, and error. The 'u' means they're open for read+write, and the actual "file" is a terminal device, `/dev/pts/1`. If you have a bunch of higher-numbered file descriptors listed, you're leaking them.

### Automating tests

You'll want to run tests for both interactive mode and batch mode, and you'll want some level of automation for each so that you can keep re-running the same suite of tests, as you'll often find that fixing one test causes a previously-passing test to start failing.

Automating interactive tests is a bit of a pain. Unless you're one of the instructors who has to grade dozens of tests, I would suggest just keeping a list of commands in an editor buffer, and copying and pasting them to the command line. 

To automate your batch-mode tests I would suggest writing a shell script, and add it to your repository as `test.sh`.  Here are a few things that will help in writing a shell script - try copy and pasting these to the command line and see how they work. (the shell is a read-eval loop, and like the shell you're writing, the main difference between interactive and batch modes is whether it prints a command prompt)

**Shell variables:** Set them with `name=value` and use them with a dollar sign: `$name`. Note that there can't be spaces around `=`, and if there are any spaces in `value` you need to enclose it with single or double quotes.

The shell preserves anything inside single quotes without modification, while it expands shell variables inside double quotes:
```
hw1$ echo "$HOME" '$HOME'
/home/cs5600 $HOME
```

**Command substitution:** if you write `$(cmd)`, the shell will run `cmd` and substitute its output:
```
hw1$ echo shell36.c > file1
hw1$ ls -l $(cat file1)
-rw-rw-r-- 1 cs5600 cs5600 2130 Sep 21 11:46 shell36.c
```

**If statements:** These typically use the `[` and `]` operators. (well, they're not really operators - you'll notice that there's a `/bin/[` executable, and it expects its last argument to be `]`)
```
hw1$ echo abc > file1
hw1$ if [ $(cat file1) = abc ] ; then
  echo IS abc
else
  echo is NOT abc
fi
```
To figure out the different operators you cap put between `[` and `]`, type `man [`. 

**"here documents":** I have no idea why they're called that, but they let you put the input to a program in your shell script along with invoking the function:
```
./shell36 <<EOF
cd /tmp/
pwd
EOF
```

There are two types of here documents - **with** variable expansion (i.e. normal) and **without**:
```
hw1$ cat <<EOF
> $HOME
> EOF
/home/cs5600
hw1$ cat <<"EOF"
$HOME
EOF
$HOME
```

**Shell functions:** These start with `name(){` and end with `}`, with arguments assigned to `$1`, `$2`, `$3`, etc.

You might find functions like the following two useful in a test script:
```
failed=''
# check_equal(testnum, val1, val2)
check_equal(){
  if [ "$2" = "$3" ] ; then
    echo Test $1: PASSED
  else
    echo Test $1: FAILED
	failed="$failed $1"
  fi
}
final_check(){
  if [ "$failed" = "" ] ; then
    echo ALL TESTS PASSED
  else
    echo FAILED: $failed
  fi
}
```
Note that functions don't return a value in the normal sense, but they can write to standard output like regular commands (e.g. using `echo`) and you can capture that output with `$( ... )`, again like a regular command.

## Various notes

Step 6:
  * Note that “<” (or “>”) may be followed by zero, one, or multiple
    words before “>” (“<”) or end of line:*
	
Your code may be tested to see that it handles the cases like the
following ones without crashing, but any reasonable non-crashing
behavior will be acceptable:
```
  ls > a > b > >
  cat < a b c < d
```

## Debugging notes

The following link gives better instructions on debugging parent/child processes with GDB:

[https://sourceware.org/gdb/current/onlinedocs/gdb.html/Forks.html](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Forks.html)

It's still pretty tricky, though. I personally found it easier to debug with `strace` and with `fprintf` to standard error:

**strace** - note that the example in the assignment echoes a single
line into `strace -f ./shell36`. You probably don't want to run the
shell interactively with `strace`, as the huge amount of output will
be confusing. You may find the following useful, as it prunes down the
output somewhat:
```
echo 'ls | cat' | strace -f -e 'open,openat,close,dup3,pipe2,execve,clone' ./shell36
```

Note that you may find it easier to debug by printing to `stderr`:, e.g.:
```
    fprintf(stderr, "parent: closing fd %d\n", fd);
```
Note that a regular `printf` may not work well if you're redirecting standard input and output.

** REMOVE ALL DEBUG PRINTFS BEFORE SUBMITTING** - the test scripts
look at standard output and standard error to verify correct
behavior. The command `echo foo` should output a single line, "foo";
outputing multiple lines of debug statements is incorrect and will
fail the test.

**"Questions"** - you are asked several questions in the assignment file -
why `cd` is a built-in command, how `execvp` finds the executable, and
why you have to exit if `execvp` fails. The purpose of these questions
is to help you understand the assignment; you are only responsible for
submitting code that implements the shell properly.

**testing waitpid** To check whether you are calling `waitpid`
properly you can run the following command:
```
sh -c 'sleep 2; echo foo'
```
This will execute a single command (`sh`) which in turn will run 2
commands; the first one will sleep for 2 seconds, making it very
obvious whether you print out the prompt before or after the command
completes and outputs "foo".
