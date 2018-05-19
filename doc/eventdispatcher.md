# Class EventDispatcher reference

## Table Of Contents

- [API reference](#apis)
- [Event getter](#event-getter)
- [Event filter](#event-filter)
- [Argument passing mode](#argument-passing-mode)
- [Nested listener safety](#nested-listener-safety)
- [Time complexities](#time-complexities)
- [Internal data structure](#internal-data-structure)

<a name="apis"></a>
## API reference

**Header**

eventpp/eventdispatcher.h

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
`Event`: the event type. The event type must be able to be used as the key in `std::map`, so it must be comparable with `operator <`.   
`FilterHandle`: the handle type returned by appendFilter. A filter handle can be used to remove a filter. To check if a `FilterHandle` is empty, convert it to boolean, *false* is empty. `FilterHandle` is copyable.  

**Functions**

```c++
EventDispatcher() = default;
EventDispatcher(EventDispatcher &&) = delete;
EventDispatcher(const EventDispatcher &) = delete;
EventDispatcher & operator = (const EventDispatcher &) = delete;
```

EventDispatcher can not be copied, moved, or assigned.

```c++
Handle appendListener(const Event & event, const Callback & callback);
```  
Add the *callback* to the dispatcher to listen to *event*.  
The listener is added to the end of the listener list.  
Return a handle which represents the listener. The handle can be used to remove this listener or insert other listener before this listener.  
If `appendListener` is called in another listener during a dispatching, the new listener is guaranteed not triggered during the same dispatching.  
The time complexity is O(1).

```c++
Handle prependListener(const Event & event, const Callback & callback);
```  
Add the *callback* to the dispatcher to listen to *event*.  
The listener is added to the beginning of the listener list.  
Return a handle which represents the listener. The handle can be used to remove this listener or insert other listener before this listener.  
If `prependListener` is called in another listener during a dispatching, the new listener is guaranteed not triggered during the same dispatching.  
The time complexity is O(1).

```c++
Handle insertListener(const Event & event, const Callback & callback, const Handle before);
```  
Insert the *callback* to the dispatcher to listen to *event* before the listener handle *before*. If *before* is not found, *callback* is added at the end of the listener list.  
Return a handle which represents the listener. The handle can be used to remove this listener or insert other listener before this listener.  
If `insertListener` is called in another listener during a dispatching, the new listener is guaranteed not triggered during the same dispatching.  
The time complexity is O(1).  

```c++
bool removeListener(const Event & event, const Handle handle);
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
The function is synchronous. The listeners are called in the thread same as the caller of `dispatch`.

```c++
template <typename Func>  
void forEach(const Event & event, Func && func);
```  
Apply `func` to all listeners of `event`.  
The `func` can be one of the three prototypes:  
```c++
AnyReturnType func(const EventDispatcher::Handle &, const EventDispatcher::Callback &);
AnyReturnType func(const EventDispatcher::Handle &);
AnyReturnType func(const EventDispatcher::Callback &);
```
**Note**: the `func` can remove any listeners, or add other listeners, safely.

```c++
template <typename Func>  
bool forEachIf(const Event & event, Func && func);
```  
Apply `func` to all listeners of `event`. `func` must return a boolean value, and if the return value is false, forEachIf stops the looping immediately.  
Return `true` if all listeners are invoked, or `event` is not found, `false` if `func` returns `false`.

```c++
FilterHandle appendFilter(const Filter & filter);
```
Add the *filter* to the dispatcher.  
Return a handle which can be used in removeFilter.

```c++
bool removeFilter(const FilterHandle & filterHandle);
```
Remove a filter from the dispatcher.  
Return true if the filter is removed successfully.

<a name="event-getter"></a>
## Event getter

The first template parameter of EventDispatcher is the *event getter*.  

If *event getter* is not a struct or class inherits from eventpp::EventGetterBase, it's the event type.  
For example `EventDispatcher<std::string, void()>` is a dispatcher with event type `std::string`, prototype `void()`.

If *event getter* inherits from tag eventpp::EventGetterBase, it's the event type getter. See [Tutorial 3](tutorial_eventdispatcher.md#tutorial3) for example.  
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


<a name="event-filter"></a>
## Event filter

`EventDispatcher<>::appendFilter(filter)` adds an event filter to the dispatcher. The `filter` receives the arguments which types are the callback prototype with lvalue reference, and must return a boolean value. Return `true` to allow the dispatcher continues the dispatching, `false` to prevent the dispatcher from invoking any subsequence listeners and filters.  

The event filters are invoked for all events, and invoked before any listeners are invoked.  
The event filters can modify the arguments since the arguments are passed as lvalue reference, no matter whether they are reference in the callback prototype (of course we can't modify a reference to const).  

Below table shows the cases of how event filters receive the arguments.

|Argument type in callback prototype |Argument type received by filter |Can filter modify the argument? | Comment |
|-----|-----|:-----:|-----|
|int, const int |int &, int & |Yes |The constness of the value is discarded|
|int &, std::string & |int &, std::string & |Yes ||
|const int &, const int *|const int &, const int * & |No |The constness of the reference/pointer must be respected|

Event filter is a powerful and useful technology, below is some sample use cases, though the real world use cases are unlimited.  

1, Capture and block all interested events. For example, in a GUI window system, all windows can receive mouse events. However, when a window is under mouse dragging, only the window under dragging should receive the mouse events even when the mouse is moving on other window. So when the dragging starts, the window can add a filter. The filter redirects all mouse events to the window and prevent other listeners from the mouse events, and bypass all other events.  

2, Setup catch-all event listener. For example, in a phone book system, the system sends events based on the actions, such as adding a phone number, remove a phone number, look up a phone number, etc. A module may be only interested in special area code of a phone number, not the actions. One approach is the module can listen to all possible events (add, remove, look up), but this is very fragile -- how about a new action event is added and the module forgets to listen on it? The better approach is the module add a filter and check the area code in the filter.


<a name="argument-passing-mode"></a>
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

<a name="nested-listener-safety"></a>
## Nested listener safety
1. If a listener adds another listener of the same event to the dispatcher during a dispatching, the new listener is guaranteed not to be triggered within the same dispatching. This is guaranteed by an unsigned 64 bits integer counter. This rule will be broken is the counter is overflowed to zero in a dispatching, but this rule will continue working on the subsequence dispatching.  
2. Any listeners that are removed during a dispatching are guaranteed not triggered.  
3. All above points are not true in multiple threading. That's to say, if one thread is invoking a callback list, the other thread add or remove a callback, the added or removed callback may be triggered during the invoking.

<a name="time-complexities"></a>
## Time complexities
The time complexities being discussed here is about when operating on the listener in the underlying list, and `n` is the number of listeners. It doesn't include the event searching in the underlying `std::map` which is always O(log n).
- `appendListener`: O(1)
- `prependListener`: O(1)
- `insertListener`: O(1)
- `removeListener`: O(1)
- `enqueue`: O(1)

<a name="internal-data-structure"></a>
## Internal data structure

EventDispatcher uses [CallbackList](doc/callbacklist.md) to manage the listener callbacks.  
