# Class EventDispatcher reference

## Table Of Contents

- [Tutorials](#tutorials)
    - [Tutorial 1 -- Basic usage](#tutorial1)
    - [Tutorial 2 -- Listener with parameters](#tutorial2)
    - [Tutorial 3 -- Customized event struct](#tutorial3)
    - [Tutorial 4 -- Event queue](#tutorial4)
	
- [API reference](#apis)
- [Event getter](#event-getter)
- [Argument passing mode](#argument-passing-mode)
- [Nested listener safety](#nested-listener-safety)
- [Thread safety](#thread-safety)
- [Exception safety](#exception-safety)
- [Time complexities](#time-complexities)
- [Internal data structure](#internal-data-structure)

<a name="tutorials" />
## Tutorials

<a name="tutorial1" />
### Tutorial 1 -- Basic usage

**Code**  
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

**Output**  
> Got event 3.  
> Got event 5.  
> Got another event 5.  

**Remarks**  
First let's define a dispatcher.
```c++
eventpp::EventDispatcher<int, void ()> dispatcher;
```
class EventDispatcher takes two template arguments. The first argument is the *event type*, here is `int`. The second is the *prototype* of the listener.  
The *event type* must be able to use as the key of `std::map`, that's to say, it must support `operator <`.  
The *prototype* is C++ function type, such as `void (int)`, `void (const std::string &, const MyClass &, int, bool)`.  

Now let's add a listener.  
```c++
dispatcher.appendListener(3, []() {
	std::cout << "Got event 3." << std::endl;
});
```
Function `appendListener` takes at least two arguments. The first argument is the *event* of type *event type*, here is `int`. The second is the *callback*.  
The *callback* can be any callback target -- functions, pointers to functions, , pointers to member functions, lambda expressions, and function objects. It must be able to be called with the *prototype* declared in `dispatcher`.  
In the tutorial, we also add two listeners for event 5.  

Now let's dispatch some event.
```c++
dispatcher.dispatch(3);
dispatcher.dispatch(5);
```
Here we dispatched two events, one is event 3, the other is event 5.  
During the dispatching, all listeners of that event will be invoked one by one in the order of they were added.

<a name="tutorial2" />
### Tutorial 2 -- Listener with parameters

**Code**  
```c++
// The listener has two parameters.
eventpp::EventDispatcher<int, void (const std::string &, const bool)> dispatcher;

dispatcher.appendListener(3, [](const std::string & s, const bool b) {
	std::cout << std::boolalpha << "Got event 3, s is " << s << " b is " << b << std::endl;
});
// The listener prototype doesn't need to be exactly same as the dispatcher.
// It would be find as long as the arguments is compatible with the dispatcher.
dispatcher.appendListener(5, [](std::string s, int b) {
	std::cout << std::boolalpha << "Got event 5, s is " << s << " b is " << b << std::endl;
});
dispatcher.appendListener(5, [](const std::string & s, const bool b) {
	std::cout << std::boolalpha << "Got another event 5, s is " << s << " b is " << b << std::endl;
});

// Dispatch the events, the first argument is always the event type.
dispatcher.dispatch(3, "Hello", true);
dispatcher.dispatch(5, "World", false);
```

**Output**  
> Got event 3, s is Hello b is true  
> Got event 5, s is World b is false  
> Got another event 5, s is World b is false  

**Remarks**  
Now the dispatcher callback prototype takes two parameters, `const std::string &` and `const bool`.  
The listener's prototype is not required to be same as the dispatcher, it's fine as long as the prototype is compatible with the dispatcher. See the second listener, `[](std::string s, int b)`, its prototype is not same as the dispatcher.

<a name="tutorial3" />
### Tutorial 3 -- Customized event struct

**Code**  
```c++
// Define an Event to hold all parameters.
struct MyEvent {
	int type;
	std::string message;
	int param;
};

// Define an event type getter to let the dispatcher knows how to
// extract the event type.
// The getter must derive from eventpp::EventGetterBase
// The getter must have:
// 1, A type named Event indicating the event type.
// 2, A static member function named getEvent. It receives all parameters
// same as the dispatcher prototype, and returns Event.
struct MyEventTypeGetter : public eventpp::EventGetterBase
{
	using Event = int;

	static Event getEvent(const MyEvent & e, bool b) {
		return e.type;
	}
};

// Pass MyEventTypeGetter as the first template argument of EventDispatcher
eventpp::EventDispatcher<
	MyEventTypeGetter,
	void (const MyEvent &, bool)
> dispatcher;

// Add a listener.
// Note: the first argument, event type, is MyEventTypeGetter::Event,
// not Event
dispatcher.appendListener(3, [](const MyEvent & e, bool b) {
	std::cout
		<< std::boolalpha
		<< "Got event 3" << std::endl
		<< "Event::type is " << e.type << std::endl
		<< "Event::message is " << e.message << std::endl
		<< "Event::param is " << e.param << std::endl
		<< "b is " << b << std::endl
	;
});

// Dispatch the event.
// The first argument is Event.
dispatcher.dispatch(MyEvent { 3, "Hello world", 38 }, true);
```

**Output**  

> Got event 3  
> Event::type is 3  
> Event::message is Hello world  
> Event::param is 38  
> b is true  

**Remarks**
Previous tutorials pass the event type as the first argument in `dispatch`, and all other event parameters as other arguments of `dispatch`. Another common situation is an Event class is defined as the base, all other events derive from Event, and the actual event type is a data member of Event (think QEvent in Qt).  

<a name="tutorial4" />
### Tutorial 4 -- Event queue

**Code**  
```c++
eventpp::EventDispatcher<int, void (const std::string &, const bool)> dispatcher;

dispatcher.appendListener(3, [](const std::string & s, const bool b) {
	std::cout << std::boolalpha << "Got event 3, s is " << s << " b is " << b << std::endl;
});
// The listener prototype doesn't need to be exactly same as the dispatcher.
// It would be find as long as the arguments is compatible with the dispatcher.
dispatcher.appendListener(5, [](std::string s, int b) {
	std::cout << std::boolalpha << "Got event 5, s is " << s << " b is " << b << std::endl;
});
dispatcher.appendListener(5, [](const std::string & s, const bool b) {
	std::cout << std::boolalpha << "Got another event 5, s is " << s << " b is " << b << std::endl;
});

// Enqueue the events, the first argument is always the event type.
// The listeners are not triggered during enqueue.
dispatcher.enqueue(3, "Hello", true);
dispatcher.enqueue(5, "World", false);

// Process the event queue, dispatch all queued events.
dispatcher.process();
```

**Output**  
> Got event 3, s is Hello b is true  
> Got event 5, s is World b is 0  
> Got another event 5, s is World b is false  

**Remarks**  
`EventDispatcher<>::dispatch()` invokes the listeners synchronously. Sometimes an asynchronous event queue is more useful (think about Windows message queue, or an event queue in a game). EventDispatcher supports such kind of event queue.  
`EventDispatcher<>::enqueue()` puts an event to the queue. Its parameters are exactly same as `dispatch`.  
`EventDispatcher<>::process()` must be called to dispatch the queued events.  
A typical use case is in a GUI application, each components call `EventDispatcher<>::enqueue()` to post the events, then the main event loop calls `EventDispatcher<>::process()` to dispatch the events.


<a name="apis" />
## API reference

**Template parameters**

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
`EventGetter`: the *event getter*. The simplest form is an event type. For details, see [Event getter](#event-getter) for details.  
`Prototype`:  the listener prototype. It's C++ function type such as `void(int, std::string, const MyClass *)`.  
`Callback`: the underlying type to hold the callback. Default is `void`, which will be expanded to `std::function`.  
`ArgumentPassingMode`: the argument passing mode. Default is `ArgumentPassingAutoDetect`. See [Argument passing mode](#argument-passing-mode) for details.  
`Threading`: threading model. Default is 'MultipleThreading'. Possible values:  
  * `MultipleThreading`: the core data is protected with mutex. It's the default value.  
  * `SingleThreading`: the core data is not protected and can't be accessed from multiple threads.  

**Public types**

`Handle`: the handle type returned by appendListener, prependListener and insertListener. A handle can be used to insert a listener or remove a listener. To check if a `Handle` is empty, convert it to boolean, *false* is empty. `Handle` is copyable.  
`Callback`: the callback storage type.  
`Event`: the event type.  

**Functions**

```c++
EventDispatcher() = default;
EventDispatcher(EventDispatcher &&) = delete;
EventDispatcher(const EventDispatcher &) = delete;
EventDispatcher & operator = (const EventDispatcher &) = delete;
```

EventDispatcher can not be copied, moved, or assigned.

```c++
Handle appendListener(const Event & event, const Callback & callback)
```  
Add the *callback* to the dispatcher to listen to *event*.  
The listener is added to the end of the listener list.  
Return a handle which represents the listener. The handle can be used to remove this listener or insert other listener before this listener.  
If `appendListener` is called in another listener during a dispatching, the new listener is guaranteed not triggered during the same dispatching.  
The time complexity is O(1).

```c++
Handle prependListener(const Event & event, const Callback & callback)
```  
Add the *callback* to the dispatcher to listen to *event*.  
The listener is added to the beginning of the listener list.  
Return a handle which represents the listener. The handle can be used to remove this listener or insert other listener before this listener.  
If `prependListener` is called in another listener during a dispatching, the new listener is guaranteed not triggered during the same dispatching.  
The time complexity is O(1).

```c++
Handle insertListener(const Event & event, const Callback & callback, const Handle before)
```  
Insert the *callback* to the dispatcher to listen to *event* before the listener handle *before*. If *before* is not found, *callback* is added at the end of the listener list.  
Return a handle which represents the listener. The handle can be used to remove this listener or insert other listener before this listener.  
If `insertListener` is called in another listener during a dispatching, the new listener is guaranteed not triggered during the same dispatching.  
The time complexity is O(1).  

```c++
bool removeListener(const Event & event, const Handle handle)
```  
Remove the listener *handle* which listens to *event* from the dispatcher.  
Return true if the listener is removed successfully, false if the listener is not found.  
The time complexity is O(1).  

```c++
void dispatch(Args ...args);  

template <typename T>  
void dispatch(T && first, Args ...args);
```  
Dispatch an event. The event type is deducted from the arguments of `dispatch`.  
In both overloads, the listeners are called with arguments `args`.  
The listeners are called in the thread same as the caller of `dispatch`.

```c++
void enqueue(Args ...args);

template <typename T>  
void enqueue(T && first, Args ...args);
```  
Put an event into the event queue. The event type is deducted from the arguments of `enqueue`.  
All arguments are copied to internal data structure, so the arguments must be copyable.

```c++
void process()
```  
Process the event queue. All events in the event queue are dispatched once and then removed from the queue.  
The listeners are called in the thread same as the caller of `process`.  
**Note**: if `process()` is called from multiple threads simultaneously, the events in the event queue are guaranteed dispatched only once.  

```c++
template <typename Func>  
void forEach(const Event & event, Func && func)
```  
Apply `func` to all listeners of `event`.  
The `func` can be one of the three prototypes:  
```c++
AnyReturnType func(const EventDispatcher::Handle &, const EventDispatcher::Callback &);
AnyReturnType func(const EventDispatcher::Handle &);
AnyReturnType func(const EventDispatcher::Callback &);
```
**Note**: the `func` can remove any listeners, or add other listeners, safely.

<a name="event-getter" />
## Event getter

The first template parameter of EventDispatcher is the *event getter*.  

If *event getter* is not a struct or class inherits from eventpp::EventGetterBase, it's the event type.  
For example `EventDispatcher<std::string, void()>` is a dispatcher with event type `std::string`, prototype `void()`.

If *event getter* inherits from tag eventpp::EventGetterBase, it's the event type getter. See [Tutorial 3](#tutorial3) for example.  
Assume we have an EventDispatcher which callback prototype is `void (const MyEvent &, bool)`, where `MyEvent` is a unified event structure.
```c++
struct MyEvent {
	int type;
	std::string message;
	int param;
	// blah blah
};
```

A typical *event getter* looks like:  
```c++
struct MyEventTypeGetter : public eventpp::EventGetterBase
{
	using Event = int;

	static Event getEvent(const MyEvent & e, bool b) {
		return e.type;
	}
};
```

Then we can define the dispatcher as
```c++
eventpp::EventDispatcher<MyEventTypeGetter, void (const MyEvent &, bool)> dispatcher;
```

An *event getter* must have a type `Event` which is the event type and used in the internal 'std::map`, and it must have a static function `getEvent`, which receives the arguments of callback prototype and return the event type.

To add a listener
```c++
dispatcher.appendListener(5, [](const MyEvent & e, bool b) {});
```
Note the first argument is the `Event` in the *event getter*, here is `int`, not `MyEvent`.

To dispatch or enqueue an event
```c++
MyEvent myEvent { 5, "Hello", 38 };
dispatcher.dispatch(myEvent, true);
```
Note the first argument is `MyEvent`, not `Event`.  
`dispatch` and `enqueue` use the function `getEvent` in the *event getter* to deduct the event type.  
`dispatch` and `enqueue` don't assume the meaning of any arguments. How to get the event type completely depends on `getEvent`.   `getEvent` can simple return a member for the first argument, or concatenate all arguments, or even hash the arguments and return the hash value as the event type.


<a name="argument-passing-mode" />
## Argument passing mode

We have the dispatcher  
```c++
eventpp::EventDispatcher<int, void(int, const std::string &)> dispatcher;
```
The event type is `int`.  
The listener's first parameter is also `int`. Depending how the event is dispatched, the listener's first argument can be either the event type, or an extra argument.

```c++
dispatcher.dispatch(3, "hello");
```
The event *3* is dispatched with an argument *"hello"*, the listener will be invoked with the arguments `(3, "hello")`, the first argument is the event type.

```c++
dispatcher.dispatch(3, 8, "hello");
```
The event *3* is dispatched with two arguments *8* and *"hello"*, the listener will be invoked with the arguments `(8, "hello")`, the first argument is the extra argument, and the event type is omitted.

So by default, EventDispatcher automatically detects the argument count of `dispatch` and listeners prototype, and calls the listeners either with or without the event type.

The default rule is convenient, permissive, and, may be error prone. The second parameter `typename ArgumentPassingMode` in the policies can control the behavior.  

```c++
struct ArgumentPassingAutoDetect;
struct ArgumentPassingIncludeEvent;
struct ArgumentPassingExcludeEvent;
```

`ArgumentPassingAutoDetect`: the default policy. Auto detects whether to pass the event type.  
`ArgumentPassingIncludeEvent`: always passes the event type. If the argument count doesn't match, compiling fails.  
`ArgumentPassingExcludeEvent`: always omits and doesn't pass the event type. If the argument count doesn't match, compiling fails.  

Assumes the number of arguments in the listener prototype is P, the number of arguments (include the event type) in `dispatch` is D, then the relationship of P and D is,  
For `ArgumentPassingAutoDetect`: P == D or P + 1 == D  
For `ArgumentPassingIncludeEvent`: P == D  
For `ArgumentPassingExcludeEvent`: P + 1 == D  

**Note**: the same rules also applies to `EventDispatcher<>::enqueue`, since `enqueue` has same parameters as `dispatch`.

Examples to demonstrate argument passing mode  

```c++
eventpp::EventDispatcher<
	int,
	void(int, const std::string &),
	ArgumentPassingAutoDetect
> dispatcher;
// or just
//eventpp::EventDispatcher<int, void(int, const std::string &)> dispatcher;
dispatcher.dispatch(3, "hello"); // Compile OK
dispatcher.dispatch(3, 8, "hello"); // Compile OK
dispatcher.enqueue(3, "hello"); // Compile OK
dispatcher.enqueue(3, 8, "hello"); // Compile OK
```

```c++
eventpp::EventDispatcher<
	int,
	void(int, const std::string &),
	ArgumentPassingIncludeEvent
> dispatcher;
dispatcher.dispatch(3, "hello"); // Compile OK
//dispatcher.dispatch(3, 8, "hello"); // Compile failure
dispatcher.enqueue(3, "hello"); // Compile OK
//dispatcher.enqueue(3, 8, "hello"); // Compile failure
```

```c++
eventpp::EventDispatcher<
	int,
	void(int, const std::string &),
	ArgumentPassingExcludeEvent
> dispatcher;
//dispatcher.dispatch(3, "hello"); // Compile failure
dispatcher.dispatch(3, 8, "hello"); // Compile OK
//dispatcher.enqueue(3, "hello"); // Compile failure
dispatcher.enqueue(3, 8, "hello"); // Compile OK
```

<a name="nested-listener-safety" />
## Nested listener safety
1. If a listener adds another listener of the same event to the dispatcher during a dispatching, the new listener is guaranteed not to be triggered within the same dispatching. This is guaranteed by an unsigned 64 bits integer counter. This rule will be broken is the counter is overflowed to zero in a dispatching, but this rule will continue working on the subsequence dispatching.  
2. Any listeners that are removed during a dispatching are guaranteed not triggered.  
3. All above points are not true in multiple threading. That's to say, if one thread is invoking a callback list, the other thread add or remove a callback, the added or removed callback may be triggered during the invoking.


<a name="thread-safety" />
## Thread safety
`EventDispatcher` is thread safe. All public functions can be invoked from multiple threads simultaneously. If it failed, please report a bug.  
`EventDispatcher` guarantees the integration of each append/prepend/insert/remove/dispatching operations, but it doesn't guarantee the order of the operations in multiple threads. For example, if a thread is dispatching an event, the other thread removes a listener in the mean time, the removed listener may be still triggered after it's removed.  
But the operations on the listeners, such as copying, moving, comparing, or invoking, may be not thread safe. It depends on listeners.  


<a name="exception-safety" />
## Exception safety

EventDispatcher doesn't throw any exceptions.  
Exceptions may be thrown by underlying code when,  
1. Out of memory, new memory can't be allocated.  
2. The listeners throw exceptions during copying, moving, comparing, or invoking.

<a name="time-complexities" />
## Time complexities
The time complexities being discussed here is about when operating on the listener in the underlying list, and `n` is the number of listeners. It doesn't include the event searching in the underlying `std::map` which is always O(log n).
- `appendListener`: O(1)
- `prependListener`: O(1)
- `insertListener`: O(1)
- `removeListener`: O(1)
- `enqueue`: O(1)

<a name="internal-data-structure" />
## Internal data structure

Beside using [CallbackList](doc/callbacklist.md) to manage the listener callbacks, EventDispatcher uses three `std::list` to manage the event queue.  
The first busy list holds all nodes with queued events.  
The second idle list holds all idle nodes. After an event is dispatched and removed from the queue, instead of freeing the memory, EventDispatcher moves the unused node to the idle list. This can improve performance and avoid memory fragment.  
The third list is a local temporary list used in function `process()`. During processing, the busy list is swapped to the temporary list, all events are dispatched from the temporary list, then the temporary list is returned and appended to the idle list.

