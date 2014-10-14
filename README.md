# hello-spidermonkey - A minimal JSAPI application

To build this, you must first
[build SpiderMonkey itself](https://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey/Build_Documentation#Developer_%28debug%29_build).

Then, edit the configuration section of the hello-spidermonkey Makefile. And then:

    make hellojs

To run the program, the SpiderMonkey .so must be in your path. This should work (replacing the `<sm-build-dir>` bit with your actual SpiderMonkey build directory, of course):

```bash
    (Linux) $ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:<sm-build-dir>/dist/lib
    (Mac) $ export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:<sm-build-dir>/builddir/dist/lib
```

Then you can run it like this:

```bash
    $ ./hellojs 'system("echo hello world")'
    hello world
```

Other things to try:

```bash
    # js arithmetic works
    ./hellojs 'system("echo " + (2 + 2))'

    # exceptions are handled
    ./hellojs 'throw new Error("FAIL")'; echo $?

    # syntax errors are handled
    ./hellojs '#include <stdio.h>'; echo $?

    # rand and srand work (see the code)
    ./hellojs 'srand(Date.now()); system("echo " + rand());'
```
