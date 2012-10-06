Topaz - a toy compiler/interpreter loosely based on Ruby syntax. This was originally designed to be cross platform and run under PALMOS on the Palm Pilot (see *archive* branch). It also works via the command line.

There are two parts: the compiler, which compiles down to bytecode, and the interpreter, which executes the bytecode.

For a description of the language see doc/topaz.pdf and ignore the PalmOS stuff.

- build: make
- compile a script: ./tc -o tower.bin test/tower.t
- run a script: ./tr tower.bin
- There are a number of test scripts in the test directory

All files copyright (c) 2012 Mark Chenoweth and released under the MIT license.



