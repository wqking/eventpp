# eventpp -- Event Dispatcher and callback list for C++

eventpp provides tools that allow your application components to communicate with each other by dispatching events and listening to them. With eventpp you can implement signal/slot mechanism, or observer pattern, very easily.

## Facts and features

1. Supports nested event. A listener can dispatch event, add other listeners, when capturing an event.
2. Thread safe.
3. Requires C++ 11 (tested with MSVC 2017, MSVC 2015, MinGW (Msys) gcc 7.2, and Ubuntu gcc 5.4).
4. Header only, no source file, no need to build.
5. Template based, less runtime overhead.
6. Backed by unit tests.
7. Written in portable and standard C++. (I'm not a C++ standard expert so if you find any non-standard code or undefined behavior please let me know.)
8. Doesn't depend any other libraries.
9. Namespace: `eventpp`.

## License

Apache License, Version 2.0  
If you have trouble with the license, contact me.

## Source code

[https://github.com/wqking/eventpp](https://github.com/wqking/eventpp)

## Classes

### EventDispatcher

Declaration
```c++
template <
	typename EventGetter,
	typename Prototype,
	typename Callback = void,
	typename ArgumentPassingMode = ArgumentPassingAutoDetect,
	typename Threading = MultipleThreading
>
class EventDispatcher;
```

Header
```c++
// Add the folder *include* to include path.
#include "eventpp/eventdispatcher.h"
```

### CallbackList

Declaration
```c++
template <
	typename Prototype,
	typename Callback = void,
	typename Threading = MultipleThreading
>
class CallbackList;
```

Header
```c++
#include "eventpp/callbacklist.h"
```

## Quick start

### Using EventDispatcher
```c++
// The namespace is eventpp
// The first template parameter int is the event type,
// the second is the prototype of the listener.
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
// The namespace is eventpp
// the first parameter is the prototype of the listener.
eventpp::CallbackList<void ()> callbackList;

// Add a callback.
// []() {} is the callback.
// Lambda is not required, any function or std::function
// or whatever function object with the required prototype is fine.
callbackList.append([]() {
	std::cout << "Got callback 1." << std::endl;
});
callbackList.append([]() {
	std::cout << "Got callback 2." << std::endl;
});

// Invoke the callback list
callbackList();
```

## Documentations

* [Event dispatcher](doc/eventdispatcher.md)
* [Callback list](doc/callbacklist.md)

## Build the unit tests

The library itself is header only and doesn't need building. If you want to run the unit tests, follow below steps:  
1. `cd tests/build`
2. Run `make` with different target.
    * make vc17 #generate solution files for Microsoft Visual Studio 2017, then open eventpptest.sln in folder project_vc17
    * make vc15 #generate solution files for Microsoft Visual Studio 2015, then open eventpptest.sln in folder project_vc15
    * make mingw #build using MinGW
    * make linux #build on Linux

## Roadmap (what's next)

* Move GCallback from my [cpgf library](https://github.com/cpgf/cpgf), so eventpp becomes a completed callback, callback list, and event dispatcher library.

* Let me know your requirement.

## Motivations

I (wqking) am a big fan of observer pattern (publish/subscribe pattern), I used such pattern a lot in my code. I either used GCallbackList in my cpgf library which is too simple and not safe, or repeated coding event dispatching mechanism such as I did in my [Gincu game engine](https://github.com/wqking/gincu). Both approaches are neither fun nor robust.  
Thanking to C++11, now it's quite easy to write a reusable event library with beautiful syntax (it's nightmare to simulate the variadic template in C++03), so here comes `eventpp`.

