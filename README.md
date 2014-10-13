# hello-spidermonkey - A minimal JSAPI application

To build this, edit the configuration section of the Makefile and then:

    make hellojs

To run the program, the SpiderMonkey .so must be in your path. This should work (replacing the `<sm-build-dir>` bit with your actual SpiderMonkey build directory, of course):

    (Linux) $ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:<sm-build-dir>/dist/lib
    (Mac) $ export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:<sm-build-dir>/builddir/dist/lib

Then you can run it like this:

    $ ./hellojs 'system("echo hello world")'
    hello world

