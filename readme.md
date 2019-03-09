# eventpp -- C++ library for event dispatcher and callback list

eventpp is a C++ event library that provides tools that allow your application components to communicate with each other by dispatching events and listening to them. With eventpp you can implement signal/slot mechanism, or observer pattern, very easily.

## Facts and features

- **Powerful**
  - Supports both synchronous event dispatching and asynchronous event queue.
  - Configurable and extensible with policies and mixins.
  - Supports event filter via mixins.
- **Robust**
  - Supports nested event. During handling an event a listener can safely dispatch event, append/prepend/insert/remove other listeners.
  - Thread safety. Supports multiple threading.
  - Exception safety. Most operations guarantee strong exception safety.
  - Well tested. Backed by unit tests.
- **Fast**
  - The EventQueue can process 10M events in 1 second (10K events per millisecond).
  - The CallbackList can invoke 100M callbacks in 1 second (100K callbacks per millisecond).
  - The CallbackList can add/remove 5M callbacks in 1 second (5K callbacks per millisecond).
- **Flexible and easy to use**
  - The listeners and events can be any type, no need to inherit from any base class.
  - Header only, no source file, no need to build. No dependencies on other libraries.
  - Requires C++ 11 (tested with MSVC 2017, MSVC 2015, MinGW (Msys) gcc 7.2, and Ubuntu gcc 5.4).
  - Written in portable and standard C++, no hacks or quirks.

## License

Apache License, Version 2.0  

## Version 0.1.0

eventpp is currently usable and near stable.

## Source code

[https://github.com/wqking/eventpp](https://github.com/wqking/eventpp)

## Quick start

### Namespace

`eventpp`

### Add eventpp to your project

eventpp is header only library. Just add the 'include' folder in eventpp to your project, then you can use the library.  
You don't need to link to any source code.

### Using CallbackList
```c++
#include "eventpp/callbacklist.h"
eventpp::CallbackList<void (const std::string &, const bool)> callbackList;
callbackList.append([](const std::string & s, const bool b) {
	std::cout << std::boolalpha << "Got callback 1, s is " << s << " b is " << b << std::endl;
});
callbackList.append([](std::string s, int b) {
	std::cout << std::boolalpha << "Got callback 2, s is " << s << " b is " << b << std::endl;
});
callbackList("Hello world", true);
```

### Using EventDispatcher
```c++
#include "eventpp/eventdispatcher.h"
eventpp::EventDispatcher<int, void ()> dispatcher;
dispatcher.appendListener(3, []() {
	std::cout << "Got event 3." << std::endl;
});
dispatcher.appendListener(5, []() {
	std::cout << "Got event 5." << std::endl;
});
dispatcher.appendListener(5, []() {
	std::cout << "Got another event 5." << std::endl;
});
// dispatch event 3
dispatcher.dispatch(3);
// dispatch event 5
dispatcher.dispatch(5);
```

### Using EventQueue
```c++
eventpp::EventQueue<int, void (const std::string &, const bool)> queue;

dispatcher.appendListener(3, [](const std::string s, bool b) {
	std::cout << std::boolalpha << "Got event 3, s is " << s << " b is " << b << std::endl;
});
dispatcher.appendListener(5, [](const std::string s, bool b) {
	std::cout << std::boolalpha << "Got event 5, s is " << s << " b is " << b << std::endl;
});

// The listeners are not triggered during enqueue.
queue.enqueue(3, "Hello", true);
queue.enqueue(5, "World", false);

// Process the event queue, dispatch all queued events.
queue.process();
```

## Documentations

* [Overview, thread and exception safety](doc/introduction.md)
* [Tutorials of CallbackList](doc/tutorial_callbacklist.md)
* [Tutorials of EventDispatcher](doc/tutorial_eventdispatcher.md)
* [Tutorials of EventQueue](doc/tutorial_eventqueue.md)
* [Class CallbackList](doc/callbacklist.md)
* [Class EventDispatcher](doc/eventdispatcher.md)
* [Class EventQueue](doc/eventqueue.md)
* [Utility class CounterRemover -- auto remove listeners after triggered certain times](doc/counterremover.md)
* [Utility class ConditionalRemover -- auto remove listeners when certain condition is satisfied](doc/conditionalremover.md)
* [Utility class ScopedRemover -- auto remove listeners when out of scope](doc/scopedremover.md)
* [Document of utilities](doc/eventutil.md)
* [Policies -- configure eventpp](doc/policies.md)
* [Mixins -- extend eventpp](doc/mixins.md)
* [Performance benchmarks](doc/benchmark.md)
* [FAQs, tricks, and tips](doc/faq.md)
* There are compilable tutorials in the unit tests.

## Build the unit tests

The library itself is header only and doesn't need building.  
The unit test requires CMake to build, and there is a makefile to ease the building.  
Go to folder `tests/build`, then run `make` with different target.
- `make vc17` #generate solution files for Microsoft Visual Studio 2017, then open eventpptest.sln in folder project_vc17
- `make vc15` #generate solution files for Microsoft Visual Studio 2015, then open eventpptest.sln in folder project_vc15
- `make mingw` #build using MinGW
- `make linux` #build on Linux

## Motivations

I (wqking) am a big fan of observer pattern (publish/subscribe pattern), I used such pattern a lot in my code. I either used GCallbackList in my [cpgf library](https://github.com/cpgf/cpgf) which is too simple and not safe (not support multi threading nor nested event), or repeated coding event dispatching mechanism such as I did in my [Gincu game engine](https://github.com/wqking/gincu) (the latest version has be rewritten to use eventpp). Both approaches are neither fun nor robust.  
Thanking to C++11, now it's quite easy to write a reusable event library with beautiful syntax (it's a nightmare to simulate the variadic template in C++03), so here comes `eventpp`.

