# Policies

## Introduction

eventpp uses policy based design to configure and extend each components' behavior. The last template parameter in EventDispatcher, EventQueue, and CallbackList is the policies class. All those three classes have default policies class named `DefaultPolicies`.  
A policy is either a type or a static function member in the policies class. All policies must be public visible, so `struct` is commonly used to define the policies class.  
All policies are optional. If any policy is omitted, the default value is used.  In fact `DefaultPolicies` is just an empty struct.  
The same policy mechanism applies to all three classes, EventDispatcher, EventQueue, and CallbackList, though not all classes requires the same policy.

## Policies

### Function getEvent

**Prototype**: `static EventKey getEvent(const Args &...)`. The function receives same arguments as `EventDispatcher::dispatch` and `EventQueue::enqueue`, and must return an event type.  
**Default value**: the default implementation returns the first argument of `getEvent`.  
**Apply**: EventDispatcher, EventQueue.

eventpp forwards all arguments of `EventDispatcher::dispatch` and `EventQueue::enqueue` (both has same arguments) to `getEvent` to get the event type, then invokes the callback list of the event type.  

Sample code

```c++
// Define an Event to hold all parameters.
struct MyEvent {
	int type;
	std::string message;
	int param;
};

// Define policies to let the dispatcher knows how to
// extract the event type.
struct MyEventPolicies
{
	static int getEvent(const MyEvent & e, bool /*b*/) {
		return e.type;
	}
};

// Pass MyEventPolicies as the third template argument of EventDispatcher.
// Note: the first template argument is the event type type int, not MyEvent.
eventpp::EventDispatcher<
	int,
	void (const MyEvent &, bool),
	MyEventPolicies
> dispatcher;

// Add a listener.
// Note: the first argument is the event type of type int, not MyEvent.
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

### Function canContinueInvoking

**Prototype**: `static bool canContinueInvoking(const Args &...)`. The function receives same arguments as `EventDispatcher::dispatch` and `EventQueue::enqueue`, and must return true if the event dispatching or callback list invoking can continue, false if the dispatching should stop.  
**Default value**: the default implementation always returns true.  
**Apply**: CallbackList, EventDispatcher, EventQueue.

Sample code

```c++
struct MyEvent {
	MyEvent() : type(0), canceled(false) {
	}
	explicit MyEvent(const int type)
		: type(type), canceled(false) {
	}

	int type;
	mutable bool canceled;
};

struct MyEventPolicies
{
	static int getEvent(const MyEvent & e) {
		return e.type;
	}

	static bool canContinueInvoking(const MyEvent & e) {
		return ! e.canceled;
	}
};

eventpp::EventDispatcher<int, void (const MyEvent &), MyEventPolicies> dispatcher;

dispatcher.appendListener(3, [](const MyEvent & e) {
	std::cout << "Got event 3" << std::endl;
	e.canceled = true;
});
dispatcher.appendListener(3, [](const MyEvent & e) {
	std::cout << "Should not get this event 3" << std::endl;
});

dispatcher.dispatch(MyEvent(3));
```

### Type Mixins

**Default value**: `using Mixins = eventpp::MixinList<>`. No mixins are enabled.  
**Apply**: EventDispatcher, EventQueue.  

A mixin is used to inject code in the EventDispatcher/EventQueue inheritance hierarchy to extend the functionalities. For more details, please read the [document of mixins](mixins.md).

### Type Callback

**Default value**: `using Callback = std::function<Parameters of callback>`.  
**Apply**: CallbackList, EventDispatcher, EventQueue.

`Callback` is the underlying storage type to hold the callback. Default is `std::function`.  

### Type Threading

**Default value**: `using Threading = MultipleThreading`.  
**Apply**: CallbackList, EventDispatcher, EventQueue.

`Threading` controls threading model. Default is 'MultipleThreading'. Possible values:  
  * `MultipleThreading`: the core data is protected with mutex. It's the default value.  
  * `SingleThreading`: the core data is not protected and can't be accessed from multiple threads.  

## Type ArgumentPassingMode

**Default value**: `using ArgumentPassingMode = ArgumentPassingAutoDetect`.  
**Apply**: EventDispatcher, EventQueue.

`ArgumentPassingMode` is the argument passing mode. Default is `ArgumentPassingAutoDetect`.

Let's see some examples. Assume we have the dispatcher  
```c++
eventpp::EventDispatcher<int, void(int, const std::string &)> dispatcher;
```
The event type is `int`.  
The listener's first parameter is also `int`. Depending on how the event is dispatched, the listener's first argument can be either the event type, or an extra argument.

```c++
dispatcher.dispatch(3, "hello");
```
The event *3* is dispatched with one argument *"hello"*, the listener will be invoked with the arguments `(3, "hello")`, the first argument is the event type.

```c++
dispatcher.dispatch(3, 8, "hello");
```
The event *3* is dispatched with two arguments *8* and *"hello"*, the listener will be invoked with the arguments `(8, "hello")`, the first argument is the extra argument, and the event type is omitted.

So by default, EventDispatcher automatically detects the argument count of `dispatch` and listeners prototype, and calls the listeners either with or without the event type.

The default rule is convenient, permissive, and, error prone. The `ArgumentPassingMode` policy can control the behavior.  

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
struct MyPolicies
{
	using ArgumentPassingMode = ArgumentPassingAutoDetect;
};
eventpp::EventDispatcher<
	int,
	void(int, const std::string &),
	MyPolicies
> dispatcher;
// or just
//eventpp::EventDispatcher<int, void(int, const std::string &)> dispatcher;
dispatcher.dispatch(3, "hello"); // Compile OK
dispatcher.dispatch(3, 8, "hello"); // Compile OK
dispatcher.enqueue(3, "hello"); // Compile OK
dispatcher.enqueue(3, 8, "hello"); // Compile OK
```

```c++
struct MyPolicies
{
	using ArgumentPassingMode = ArgumentPassingIncludeEvent;
};
eventpp::EventDispatcher<
	int,
	void(int, const std::string &),
	MyPolicies
> dispatcher;
dispatcher.dispatch(3, "hello"); // Compile OK
//dispatcher.dispatch(3, 8, "hello"); // Compile failure
dispatcher.enqueue(3, "hello"); // Compile OK
//dispatcher.enqueue(3, 8, "hello"); // Compile failure
```

```c++
struct MyPolicies
{
	using ArgumentPassingMode = ArgumentPassingExcludeEvent;
};
eventpp::EventDispatcher<
	int,
	void(int, const std::string &),
	MyPolicies
> dispatcher;
//dispatcher.dispatch(3, "hello"); // Compile failure
dispatcher.dispatch(3, 8, "hello"); // Compile OK
//dispatcher.enqueue(3, "hello"); // Compile failure
dispatcher.enqueue(3, 8, "hello"); // Compile OK
```

### Template Map

**Prototype**:  
```c++
template <typename Key, typename T>
using Map = // std::map <Key, T> or other map type
```
**Default value**: auto determined.  
**Apply**: EventDispatcher, EventQueue.  

`Map` is the associative container type used by EventDispatcher and EventQueue to hold the underlying (Event type, CallbackList) pairs.  
`Map` is a template with two parameters, the first parameter is the key, the second parameter is the value.  
`Map` must support operations `[]`, `find()`, and `end()`.  
If `Map` is not specified, eventpp will auto determine the type. If the event type supports `std::hash`, `std::unordered_map` is used, otherwise, `std::map` is used.

## How to use policies

To use policies, declare a struct, define the policies in it, and pass the struct to CallbackList, EventDispatcher, or EventQueue.  
```c++
struct MyPolicies //the struct name doesn't matter
{
	template <typename ...Args>
	static int getEvent(const MyEvent & e, const Args &...) {
		return e.type;
	}
};
EventDispatcher<int, void(const MyEvent &), MyPolicies> dispatcher;
```
Above sample code shows a policies class, which only redefined 'getEvent', and leave all other policies default.
