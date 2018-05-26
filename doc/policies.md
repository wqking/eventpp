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

eventpp forwards all arguments of `EventDispatcher::dispatch` and `EventQueue::enqueue` (both has same arguments) to `getEvent` to get the event type.

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

**Default value**: 'using ArgumentPassingMode = ArgumentPassingAutoDetect'.  
**Apply**: EventDispatcher, EventQueue.

`ArgumentPassingMode` is the argument passing mode. Default is `ArgumentPassingAutoDetect`.

Let's see some examples. Assume we have the dispatcher  
```c++
eventpp::EventDispatcher<int, void(int, const std::string &)> dispatcher;
```
The event type is `int`.  
The listener's first parameter is also `int`. Depending how the event is dispatched, the listener's first argument can be either the event type, or an extra argument.

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
