# eventpp -- Event Dispatcher and callback list for C++

eventpp provides tools that allow your application components to communicate with each other by dispatching events and listening to them. With eventpp you can implement signal/slot mechanism, or observer pattern, very easily.

## Facts and features

- Template based, less runtime overhead, unlimited possibilities. The event and callback can be almost any C++ types meeting minimum requirements.
- Supports nested event. A listener can dispatch event, append/prepend/insert/remove other listeners during capturing an event safely.
- Thread safe.
- Requires C++ 11 (tested with MSVC 2017, MSVC 2015, MinGW (Msys) gcc 7.2, and Ubuntu gcc 5.4).
- Header only, no source file, no need to build.
- Backed by unit tests.
- Written in portable and standard C++. (I'm not a C++ standard expert so if you find any non-standard code or undefined behavior please let me know.)
- Doesn't depend on any other libraries.

## License

Apache License, Version 2.0  
If you have trouble with the license, contact me.

## Source code

[https://github.com/wqking/eventpp](https://github.com/wqking/eventpp)

## Quick start

### Namespace

`eventpp`

### Using EventDispatcher
```c++
// Add the folder *include* to include path.
#include "eventpp/eventdispatcher.h"

// The namespace is eventpp
// The first template parameter int is the event type,
// the event type can be any type such as std::string, int, etc.
// The second is the prototype of the listener.
eventpp::EventDispatcher<int, void ()> dispatcher;

// Add a listener. As the type of dispatcher,
// here 3 and 5 is the event type,
// []() {} is the listener.
// Lambda is not required, any function or std::function
// or whatever function object with the required prototype is fine.
dispatcher.appendListener(3, []() {
	std::cout << "Got event 3." << std::endl;
});
dispatcher.appendListener(5, []() {
	std::cout << "Got event 5." << std::endl;
});
dispatcher.appendListener(5, []() {
	std::cout << "Got another event 5." << std::endl;
});

// Dispatch the events, the first argument is always the event type.
dispatcher.dispatch(3);
dispatcher.dispatch(5);
```

### Using CallbackList
```c++
// Add the folder *include* to include path.
#include "eventpp/callbacklist.h"

// The namespace is eventpp
// The callback list has two parameters.
eventpp::CallbackList<void (const std::string &, const bool)> callbackList;

callbackList.append([](const std::string & s, const bool b) {
	std::cout << std::boolalpha << "Got callback 1, s is " << s << " b is " << b << std::endl;
});
// The callback prototype doesn't need to be exactly same as the callback list.
// It would be find as long as the arguments is compatible with the dispatcher.
callbackList.append([](std::string s, int b) {
	std::cout << std::boolalpha << "Got callback 2, s is " << s << " b is " << b << std::endl;
});

// Invoke the callback list
callbackList("Hello world", true);
```

## Documentations

* [Event dispatcher](doc/eventdispatcher.md)
* [Callback list](doc/callbacklist.md)

## Build the unit tests

The library itself is header only and doesn't need building.  
The unit test requires CMake to build, and there is a makefile to ease the building.  
Go to folder `tests/build`, then run `make` with different target.
- `make vc17` #generate solution files for Microsoft Visual Studio 2017, then open eventpptest.sln in folder project_vc17
- `make vc15` #generate solution files for Microsoft Visual Studio 2015, then open eventpptest.sln in folder project_vc15
- `make mingw` #build using MinGW
- `make linux` #build on Linux

## Roadmap (what's next)

- Move GCallback from my [cpgf library](https://github.com/cpgf/cpgf), so eventpp becomes a completed callback, callback list, and event dispatcher library.

- Let me know your requirement.

## Motivations

I (wqking) am a big fan of observer pattern (publish/subscribe pattern), I used such pattern a lot in my code. I either used GCallbackList in my cpgf library which is too simple and not safe, or repeated coding event dispatching mechanism such as I did in my [Gincu game engine](https://github.com/wqking/gincu). Both approaches are neither fun nor robust.  
Thanking to C++11, now it's quite easy to write a reusable event library with beautiful syntax (it's nightmare to simulate the variadic template in C++03), so here comes `eventpp`.

