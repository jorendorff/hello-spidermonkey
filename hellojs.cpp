#include <stdlib.h>
#include <stdio.h>
#include "jsapi.h"

using namespace JS;

// The class of the global object.
static JSClass globalClass = {
    "global",
    JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    nullptr, nullptr, nullptr, nullptr,
    JS_GlobalObjectTraceHook
};

// The error reporter callback.
void reportError(JSContext *cx, const char *message, JSErrorReport *report) {
     fprintf(stderr, "%s:%u:%s\n",
             report->filename ? report->filename : "[no filename]",
             (unsigned int) report->lineno,
             message);
}

// myjs_rand - A simple example of a JSNative.
//
// This is how you implement functions in C++ that can be called from JS code.
// This particular one is about as simple as possible: it calls the standard C
// function rand() and returns the result.
//
bool
myjs_rand(JSContext *cx, unsigned argc, Value *vp)
{
    // Every JSNative should start with this line. The C++ CallArgs object is
    // how you access the arguments passed from JS and set the return value.
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

    // Do the work this function is supposed to do, in this case just call the
    // rand() function.
    int result = rand();

    // Set the return value. Every JSNative must do this before returning true.
    // args.rval() returns a reference to a JS::Value, which has a variety of
    // setXyz() methods.
    //
    // In our case, we want to return a number, so we use setNumber(). It
    // requires either a double or a uint32_t argument. result is neither of
    // those, so we cast it.
    args.rval().setNumber(double(result));

    // Return true to indicate success. Later on we'll see that if a JSNative
    // throws an exception or encounters an error, it must return false.
    return true;
}

// myjs_srand - Another example JSNative function. Perhaps you can decipher
// this one on your own.
bool
myjs_srand(JSContext *cx, unsigned argc, Value *vp)
{
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    uint64_t seed;
    if (!JS::ToUint64(cx, args.get(0), &seed))
        return false;
    srand((unsigned int) seed);
    args.rval().setUndefined();
    return true;
}

// myjs_system - Another example JSNative function. This one includes some
// string conversion (always a pain) and shows how to throw a JS exception from
// C++.
bool
myjs_system(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedString cmd(cx);
    cmd = ToString(cx, args.get(0));
    if (!cmd)
        return false;

    char *cmdBytes = JS_EncodeString(cx, cmd);
    if (!cmdBytes)
        return false;

    int rc = system(cmdBytes);
    JS_free(cx, cmdBytes);
    if (rc != 0) {
        // Throw a JavaScript exception.
        JS_ReportError(cx, "Command failed with status code %d", rc);
        return false;
    }
    args.rval().setUndefined();
    return true;
}

// An array of all our global functions and their names.
static JSFunctionSpec myjs_global_functions[] = {
    JS_FS("rand",   myjs_rand,   0, 0),
    JS_FS("srand",  myjs_srand,  1, 0),
    JS_FS("system", myjs_system, 1, 0),
    JS_FS_END
};

bool
run(JSContext *cx, const char *code)
{
    // Enter a request before running anything in the context.
    JSAutoRequest ar(cx);

    // Create the global object and a new compartment.
    RootedObject global(cx);
    global = JS_NewGlobalObject(cx, &globalClass, nullptr,
                                JS::FireOnNewGlobalHook);
    if (!global)
        return false;

    // Enter the new global object's compartment.
    JSAutoCompartment ac(cx, global);

    // Populate the global object with the standard globals, like Object and
    // Array.
    if (!JS_InitStandardClasses(cx, global))
        return false;

    // Also populate the global object with some nonstandard globals we wrote
    // ourselves.
    if (!JS_DefineFunctions(cx, global, myjs_global_functions))
        return false;

    // Your application code here. This may include JSAPI calls to create your
    // own custom JS objects and run scripts. Here we'll just run the code
    // provided by the caller.
    RootedValue rval(cx);
    if (!JS_EvaluateScript(cx, global, code, strlen(code), "<command line>", 1, &rval))
        return false;

    return true;
}

const char *usage =
    "usage: hellojs CODE\n"
    "CODE can be any JS code. It can also use these functions:\n"
    "  rand() - return a pseudorandom number\n"
    "  srand(seed) - seed the random number generator\n"
    "  system(cmd) - run a command\n";

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        fputs(usage, stderr);
        return 1;
    }

    // Initialize the JS engine.
    if (!JS_Init())
       return 1;

    // Create a JS runtime.
    JSRuntime *rt = JS_NewRuntime(8L * 1024L * 1024L);
    if (!rt)
       return 1;

    // Create a context.
    JSContext *cx = JS_NewContext(rt, 8192);
    if (!cx)
       return 1;
    JS_SetErrorReporter(rt, reportError);

    // Now run the code provided on the command line.  If it succeeds, we'll
    // exit with a status code of 0; otherwise 1 to indicate an error.
    int exitCode = run(cx, argv[1]) ? 0 : 1;

    // Shut everything down.
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();

    return exitCode;
}
