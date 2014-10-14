/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * hellojs.cpp - A bare-bones JSAPI application.
 *
 * This is intended as introductory documentation to the JSAPI.
 * The JSAPI User Guide explains what's going on here in detail:
 * <https://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey/JSAPI_User_Guide>
 */

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
void
reportError(JSContext *cx, const char *message, JSErrorReport *report)
{
     fprintf(stderr, "%s:%u:%s\n",
             report->filename ? report->filename : "[no filename]",
             (unsigned int) report->lineno,
             message);
}

// === Custom functions
//
// The next three functions are examples of how you can implement functions in
// C++ that can be called from JS. All such functions have the same signature;
// this type of function is called a JSNative.

// myjs_rand - A very basic example of a JSNative. It calls the standard C
// function rand() and returns the result.
bool
myjs_rand(JSContext *cx, unsigned argc, Value *vp)
{
    // Every JSNative should start with this line. The C++ CallArgs object is
    // how you access the arguments passed from JS and set the return value.
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

    // Do the work this function is supposed to do. In this case just call the
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

// myjs_srand - Another example JSNative function. Maybe you can decipher this
// one on your own.
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
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

    // Temporary variable to hold a string we're about to create.
    // Variables must be "Rooted" to protect their values from GC.
    JS::RootedString cmd(cx);

    // Convert the argument passed by the user (which could be any JS::Value--
    // a string, number, boolean, object, or something else) to a string.
    cmd = JS::ToString(cx, args.get(0));
    if (!cmd) {
        // If we get here, JS::ToString returned null. That means some kind of
        // error occurred while converting the argument to a string. Most
        // likely, the argument was an object, and its .toString() method threw
        // an exception.  But it could be something else -- maybe we ran out of
        // memory, for example.
        //
        // In any case, it's a good thing we checked that return value! Now we
        // can gracefully propagate that error on to our caller, like so:
        return false;

        // If we hadn't checked, we would have crashed the next time we tried
        // to do anything with cmd. The moral: Always check return values.
        // Seriously, do it. Don't be lazy. You're better than that.
    }

    // One more hurdle: JS strings have 16-bit characters. The 'system()'
    // function expects 8-bit characters. We have to convert. We'll use UTF-8,
    // so this function will handle Unicode input correctly on Mac and
    // Linux. On Windows, alas, Unicode will be garbled. (You could work around
    // that using an #ifdef and some Windows-specific code.)
    char *cmdBytes = JS_EncodeStringToUTF8(cx, cmd);
    if (!cmdBytes)  // <-- eternal error-checking vigilance!
        return false;

    // Actually do the work we came here to do.
    int status = system(cmdBytes);

    // The documentation for JS_EncodeStringToUTF8 says that the caller (that's
    // us) is responsible for freeing the buffer.
    JS_free(cx, cmdBytes);

    // When system() returns nonzero, that means the command failed somehow.
    if (status != 0) {
        // Set up a JavaScript exception and return false.
        JS_ReportError(cx, "Command failed with status code %d", status);
        return false;
    }

    // Success!
    args.rval().setUndefined();
    return true;
}

// An array of all our JSNatives and their names. We'll pass this table to
// JS_DefineFunctions below.
//
// (The third argument to JS_FS is the argument count: rand.length will be 0
// and srand.length will be 1. The fourth argument is a "flags" argument,
// almost always 0.)
static JSFunctionSpec myjs_global_functions[] = {
    JS_FS("rand",   myjs_rand,   0, 0),
    JS_FS("srand",  myjs_srand,  1, 0),
    JS_FS("system", myjs_system, 1, 0),
    JS_FS_END
};


// === The main program

// run - Create a global object, populate it with standard library functions,
// and then run some JS code at last!
bool
run(JSContext *cx, const char *code)
{
    // Enter a request before trying to do anything in the context.  Sorry,
    // this is a bit of an API wart. Not every function needs this: if your
    // caller has a JSAutoRequest, you don't need one.
    JSAutoRequest ar(cx);

    // Create the global object and a new compartment.
    RootedObject global(cx);
    global = JS_NewGlobalObject(cx, &globalClass, nullptr, JS::FireOnNewGlobalHook);
    if (!global)
        return false;

    // Enter the new global object's compartment.
    JSAutoCompartment ac(cx, global);

    // Populate the global object with the JS standard library, like Object and
    // Array.
    if (!JS_InitStandardClasses(cx, global))
        return false;

    // Also populate the global object with some nonstandard globals we wrote
    // ourselves.
    if (!JS_DefineFunctions(cx, global, myjs_global_functions))
        return false;

    // Run the code provided by the caller!
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

int
main(int argc, const char *argv[])
{
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

    // Run the code supplied on the command line.
    bool success = run(cx, argv[1]);

    // Shut everything down.
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();

    return success ? 0 : 1;
}
