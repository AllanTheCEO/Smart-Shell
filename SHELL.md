# Shell scripting

Here are the shell script basics you'll need in order to write automated tests for Lab 2.

NOTE - this describes the syntax of the **real** shell you are using in your VM, not the shell you are implementing. This "real" shell will be `bash` on your VM (unless you've changed things); recent Mac systems use `zsh` which is a little bit different. The `sh` shell is a simple least-common-denominator shell that often gets used for portable scripts.

### Syntax

The shell sees everything in terms of *words* and *lines*, except:
- anything inside a pair of single or double quotes behaves like a single word, and
- a backslash at the end of a line causes the next line to be appended to the current one

### Variable expansion and quotes

To set a variable you use `var=value`, with no spaces between the variable name, `=`, and the value. If you have spaces in your value, you'll need to quote it:
```
    file=/tmp/abc.txt
    msg='This is a test'
```

Variable names must begin with a letter, and can contain letters, numbers, and underscore. 
 
To substitute the value of variable `x`, use `$x`. This will be expanded in every context except single quotes:
```
% x=test
% echo $x
test
% echo "value: $x"
value: test
% echo '$x'
$x
% 
```
(`echo` does pretty much what its name says)

### Automatic variables

The shell defines several "automatic" variables:
- `$?` - the exit code from the last command. Save it quickly. (question - why?)
- `$1`, `$2`, etc. - arguments to a shell script or function

plus a bunch we're going to ignore.

### What is a shell script?

It's just a sequence of shell commands that you could have typed on the command line. Or that you actually did type in a terminal, then copied to an editor buffer and cleaned them up. (that's my typical method of developing shell scripts)

The first line of the script should be `#!` - that way if you make the script executable, it will be executed with the default shell; however you can always execute a shell with the `sh` command:
```
    $ sh myscript.sh
```

The only difference between a script and commands typed in a terminal is that any arguments passed to the script are available in the variables `$1`, `$2` etc.

### Control flow
Most shell scripts use the following three control flow mechanisms:
```
while command; do
   ... commands ...
done
```
While loop - run `command` each time around the loop, if it fails then stop

```
for f in a b c; do
   ... commands ...
done
```
For each word in the list (`a b c` in this case) set the loop variable (`f`) to that value and execute the loop body.

The `break` and `continue`  keywords do the expected thing for both `while` and `for` loos.

```
if command ; then
   ... commands ...
fi
```
If `command` executes successfully, then evaluate the statement body.

With `if` you'll almost always need the `test` command, which you typically use via the `[` alias:
```
if [ $var1 = abc ] ; then
    echo value was abc
fi
```
You'll probably want to check the man page (`man test`) every time you use it, as the options are obscure and there are a lot of them.

### In-script input, capturing output

When you're providing test input to a command, it's cumbersome to redirect input from individual files - you'll end up with files like `input_test1`, `input_test2`, etc. and their contents won't be visible when you're editing the script.

If you want to send a single line to a command, you can use `echo`:
```
    echo 'exit 7' | ./shell36
```

For multiple lines, it's easier to use what's called a [here document](https://en.wikipedia.org/wiki/Here_document) (I have no idea where the name comes from), with the input lines appearing in your shell script:
```
./shell36 <<FLAG
echo in shell36
exit 0
FLAG
echo back in real shell
```
should give the results:
```
in shell36
back in real shell
```

Note that variables will be expanded inside those lines, **except** when you put double quotes around the **first** flag:
```
X=abc
cat <<FLAG
X: $X
FLAG
cat <<"FLAG"
X: $X
FLAG
```
gives the results:
```
X: abc
X: $X
```
This is horribly inconsistent, but it's the way it works.

Often you want to capture the output from a command and use it in the script. You can redirect it into a file (typically in the `/tmp` directory):
```
command > /tmp/out.1
```
To actually do something with it you'll need to read it into the shell. Two ways to do this:

**Substitution:** the expression `$(cmd)` doesn't substitute a variable - it executes `cmd`, then substitutes the data written to standard output by that command. If the command is `$(cat file.out)`, then it will substitute the output of `cat file.out` - i.e. the contents of the file.

**Line by line:** you can read each line into one or more variables:
```
while read word1 remainder < /tmp/1; do
    echo first word: $word1
    echo remainder: $remainder
done
```
The `read` shell builtin will take a line of input, split it into words, and assign them to the arguments; it returns FALSE at end-of-file.

### Shell functions

You can define a function in the shell:
```
my_function(){
   echo first arg: $1
   echo second: $2
}
my_function a b
```

**returning data from functions:** Getting data from a function is like getting data from an external command - it has to be written to standard output, or redirected into a file, or something like that. 

### Numeric calculations

The `$[ ]` operator allows you to do limited arithmetic on strings that look like integers:
```
$ echo $[5+6]
11
$ N_PASSED=$[$N_PASSED+1]
```

### Test-specific stuff

You can use strings to accumulate lists of things, which can be useful when you're running a series of tests:

```
if [ ... pass ... ] ; then
    TESTS_PASSED="$TESTS_PASSED 5"
else
    TESTS_FAILED="$TESTS_FAILED 5"
fi
```

The `bash` shell has arrays and other fancy features, but the syntax is pretty horrible, the documention is complicated, and if you really want to use them, your instructors won't be much help if you run into problems.
