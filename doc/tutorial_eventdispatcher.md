# Tutorials of EventDispatcher

## Table Of Contents

- [Tutorial 1 -- Basic usage](#tutorial1)
- [Tutorial 2 -- Listener with parameters](#tutorial2)
- [Tutorial 3 -- Customized event struct](#tutorial3)
- [Tutorial 4 -- Event filter](#tutorial4)

<a name="tutorials"></a>
## Tutorials

<a name="tutorial1"></a>
### Tutorial 1 -- Basic usage

**Code**  
```c++
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

<a name="tutorial2"></a>
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

<a name="tutorial3"></a>
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

<a name="tutorial4"></a>
### Tutorial 4 -- Event filter

**Code**  
```c++
eventpp::EventDispatcher<int, void (int e, int i, std::string)> dispatcher;

dispatcher.appendListener(3, [](const int e, const int i, const std::string & s) {
	std::cout << "Got event 3, i was 1 but actural is " << i << " s was Hello but actural is " << s << std::endl;
});
dispatcher.appendListener(5, [](const int e, const int i, const std::string & s) {
	std::cout << "Shout not got event 5" << std::endl;
});

// Add three event filters.

// The first filter modifies the input arguments to other values, then the subsequence filters
// and listeners will see the modified values.
dispatcher.appendFilter([](const int e, int & i, std::string & s) -> bool {
	std::cout << "Filter 1, e is " << e << " passed in i is " << i << " s is " << s << std::endl;
	i = 38;
	s = "Hi";
	std::cout << "Filter 1, changed i is " << i << " s is " << s << std::endl;
	return true;
});

// The second filter filters out all event of 5. So no listeners on event 5 can be triggered.
// The third filter is not invoked on event 5 also.
dispatcher.appendFilter([](const int e, int & i, std::string & s) -> bool {
	std::cout << "Filter 2, e is " << e << " passed in i is " << i << " s is " << s << std::endl;
	if(e == 5) {
		return false;
	}
	return true;
});

// The third filter just prints the input arguments.
dispatcher.appendFilter([](const int e, int & i, std::string & s) -> bool {
	std::cout << "Filter 3, e is " << e << " passed in i is " << i << " s is " << s << std::endl;
	return true;
});

// Dispatch the events, the first argument is always the event type.
dispatcher.dispatch(3, 1, "Hello");
dispatcher.dispatch(5, 2, "World");
```

**Output**  
> Filter 1, e is 3 passed in i is 1 s is Hello  
> Filter 1, changed i is 38 s is Hi  
> Filter 2, e is 3 passed in i is 38 s is Hi  
> Filter 3, e is 3 passed in i is 38 s is Hi  
> Got event 3, i was 1 but actural is 38 s was Hello but actural is Hi  
> Filter 1, e is 5 passed in i is 2 s is World  
> Filter 1, changed i is 38 s is Hi  
> Filter 2, e is 5 passed in i is 38 s is Hi  

**Remarks**  
`EventDispatcher<>::appendFilter(filter)` adds an event filter to the dispatcher. The `filter` receives the arguments which types are the callback prototype with lvalue reference, and must return a boolean value. Return `true` to allow the dispatcher continues the dispatching, `false` to prevent the dispatcher from invoking any subsequence listeners and filters.  
The event filters are invoked before any listeners are invoked.

