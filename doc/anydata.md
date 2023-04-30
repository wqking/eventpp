# Class AnyData reference
<!--begintoc-->
## Table Of Contents

* [Description](#a2_1)
* [Use AnyData](#a2_2)
  * [Header](#a3_1)
  * [Class AnyData template parameters](#a3_2)
  * [Use AnyData in EventQueue, the simplest but not recommend way](#a3_3)
  * [Use AnyData in EventQueue, the recommend way](#a3_4)
  * [Extend with new events](#a3_5)
* [Global function](#a2_3)
  * [maxSizeOf](#a3_6)
* [Tutorial](#a2_4)
<!--endtoc-->

<a id="a2_1"></a>
## Description

Class `AnyData` is a data structure that can hold any data types without any dynamic heap allocation, and `AnyData` can be passed to `EventQueue` in place of the data it holds. The purpose is to eliminate the heap allocation time which is commonly used as `std::shared_ptr`. `AnyData` can improve the performance by about 30%~50%, comparing to heap allocation with `std::shared_ptr`.  

`AnyData` is not type safe. Abusing it may lead you problems. If you understand how it works, and use it properly, you can gain performance improvement.  
`AnyData` should be used for extreme performance optimization, such as the core event system in a game engine. In a performance non-critical system such as desktop GUI, you may not need `AnyData`, otherwise, it's premature optimization.  
`AnyData` should be only used in `EventQueue`, it's not general purpose class, don't use it for other purpose.  

For example, assume we have the event class hierarchy (we will use these event classes in example code all over this document),

```c++
enum class EventType
{
	// for MouseEvent
	mouse,
	// for KeyEvent
	key,
	// for MessageEvent
	message,
	// For a simple std::string event which is not derived from Event 
	text,
};

class Event
{
public:
	Event() {
	}
};

class MouseEvent : public Event
{
public:
	MouseEvent(const int x, const int y)
		: x(x), y(y)
	{
	}

	int getX() const { return x; }
	int getY() const { return y; }

private:
	int x;
	int y;
};

class KeyEvent : public Event
{
public:
	explicit KeyEvent(const int key)
		: key(key)
	{
	}

	int getKey() const { return key; }

private:
	int key;
};

class MessageEvent : public Event
{
public:
	explicit MessageEvent(const std::string & message)
		: message(message) {
	}

	std::string getMessage() const { return message; }

private:
	std::string message;
};

// eventMaxSize is the maximum size of all possible types to use in AnyData, it's used to construct AnyData.
constexpr std::size_t eventMaxSize = eventpp::maxSizeOf<
		Event,
		KeyEvent,
		MouseEvent,
		MessageEvent,
		std::string
	>();
```

Without using `AnyData`, we need to use shared pointer to put an event into an `EventQueue`, for example,

```c++
using Queue = eventpp::EventQueue<EventType, void (const std::shared_ptr<Event> &)>;
Queue eventQueue;
eventQueue.enqueue(EventType::key, std::make_shared<KeyEvent>(123));
eventQueue.enqueue(EventType::mouse, std::make_shared<MouseEvent>(100, 200));
```

The problem of the shared pointer approach is that it needs dynamic heap allocation, and it is pretty slow.
One solution is to use a small object pool to reuse the allocated objects, another solution is `AnyData`.

Now with `AnyData`, we can eliminate the usage of shared pointer and heap allocation, for example,

```c++
using Queue = eventpp::EventQueue<EventType, void (const eventpp::AnyData<eventMaxSize> &), SomePolicies>;
Queue eventQueue;
eventQueue.enqueue(EventType::key, KeyEvent(123));
eventQueue.enqueue(EventType::mouse, MouseEvent(100, 200));
```

<a id="a2_2"></a>
## Use AnyData

<a id="a3_1"></a>
### Header

eventpp/utilities/anydata.h  

<a id="a3_2"></a>
### Class AnyData template parameters

```c++
template <std::size_t maxSize>
class AnyData;
```

`AnyData` requires one constant template parameter. It's the max size of the underlying types. Any data types can be used to construct `AnyData`, as long as the data type size is not larger than `maxSize`. If it's larger, compile time error is produced.  
`AnyData` uses at least `maxSize` bytes, even if the underlying data is only 1 byte long. So `AnyData` might use slightly more memory than the shared pointer solution, but also may not, because shared pointer solution has other memory overhead.  

<a id="a3_3"></a>
### Use AnyData in EventQueue, the simplest but not recommend way

`AnyData` can be used as the callback arguments in EventQueue. 

```c++
eventpp::EventQueue<EventType, void (const EventType, const eventpp::AnyData<eventMaxSize> &)> queue;
```

In such form, the listener function prototype must be `void (const EventType, const eventpp::AnyData<eventMaxSize> &)`, for example,  

```c++
queue.appendListener(EventType::key, [](const EventType type, const eventpp::AnyData<eventMaxSize> & e) {
	std::cout << "Received KeyEvent, key=" << e.get<KeyEvent>().getKey() << std::endl;
});
```

Then every listener function must receive `AnyData` as an argument. This is really bad idea because then your code is coupled with `eventpp` tightly. I highly recommend you not to do so. But if you do want to do it, here are the `AnyData` member functions which helps you to use it.  

#### get

```c++
template <typename T>
const T & get() const;
```

Return a reference to the underlying data as type `T`. `AnyData` doesn't check if the underlying data is of type `T`, it simply returns a reference to the underlying data, so it's not type safe.

#### getAddress

```c++
const void * getAddress() const;
```

Return a pointer to the address of the underlying data.

#### isType

```c++
template <typename T>
bool isType() const;
```

Return true if the underlying data type is `T`, false if not.  
This function compares the exactly types, it doesn't check any class hierarchy. For example, if an `AnyData` holds `KeyEvent`, then `isType<KeyEvent>()` will return true, but `isType<Event>()` will return false.  

<a id="a3_4"></a>
### Use AnyData in EventQueue, the recommend way

```c++
struct Policies {
	using Callback = std::function<void (const Event &)>;
};
eventpp::EventQueue<EventType, void (const EventType, const eventpp::AnyData<eventMaxSize> &, Policies)> queue;
queue.appendListener(EventType::key, [](const Event & e) {
	std::cout << "Received KeyEvent, key=" << static_cast<const KeyEvent &>(e).getKey() << std::endl;
});
```

We specify the `Callback` policy as `std::function<void (const Event &)>`, and pass the policy to `EventQueue`. Now the listener functions can receive `const Event &` instead of `AnyData`. It looks more nature than the "not recommend" method.  
The magic is, `AnyData` can convert to any types automatically. So when `EventQueue` passes `AnyData` to the listener, `AnyData` can cast to `const Event &` automatically.  
`AnyData` can convert to reference or pointer. When it converts to reference, the reference refers to the underlying data. When it converts to pointer, the pointer points to the address of the underlying data. The special conversion of pointer allow we use unrelated data types as event arguments and receive the argument as `const void *`. For example,  

```c++
struct Policies {
	using Callback = std::function<void (const EventType, const void *)>;
};
eventpp::EventQueue<EventType, void (const EventType, const eventpp::AnyData<eventMaxSize> &), Policies> queue;
queue.appendListener(EventType::key, [](const EventType type, const void * e) {
	assert(type == EventType::key);
	std::cout << "Received KeyEvent, key=" << static_cast<const KeyEvent *>(e)->getKey() << std::endl;
});
queue.appendListener(EventType::text, [](const EventType type, const void * e) {
	assert(type == EventType::text);
	std::cout << "Received text event, text=" << *static_cast<const std::string *>(e) << std::endl;
});
queue.enqueue(EventType::key, KeyEvent(255));
queue.enqueue(EventType::text, std::string("This is a text"));
queue.process();
```

<a id="a3_5"></a>
### Extend with new events

`AnyData` requires compile time known max data size. What if a user needs to add new events but he can't modify the `AnyData` declaration thus can't modify the max size?  
The user can add any new events as long as the data size is not larger then the max size. If any new events has larger data, the data can be put in dynamic allocated memory and hold in `std::shared_ptr`. For example,  

```c++
class MyLargeEvent : public Event
{
private:
	std::shared_ptr<char> myLargeGigaBytesData;
};
```

Then it works as long as `sizeof(MyLargeEvent) <= eventMaxSize`.

<a id="a2_3"></a>
## Global function

<a id="a3_6"></a>
### maxSizeOf

```c++
template <typename ...Ts>
constexpr std::size_t maxSizeOf();
```

Return the maximum size of types Ts... For example,  
```c++
maxSizeOf<KeyEvent, MouseEvent, int, double>();
```

<a id="a2_4"></a>
## Tutorial

Below is the tutorial code. The complete code can be found in `tests/tutorial/tutorial_anydata.cpp`  

```c++
TEST_CASE("AnyData tutorial 1, basic")
{
	std::cout << std::endl << "AnyData tutorial 1, basic" << std::endl;

	// Define the Policies struct, set Callback as std::function<void (const Event &)>,
	// then the listener can have prototype as `void (const Event &)`.
	// If we don't customize the Callback, the listener prototype have to be
	// `void (const eventpp::AnyData<eventMaxSize> &)`.
	// The argument is `const Event &` because all events we send in this tutorial are derived from Event.
	struct Policies {
		using Callback = std::function<void (const Event &)>;
	};
	// Construct EventQueue with Policies. Here we use `const eventpp::AnyData<eventMaxSize> &` as the
	// callback argument.
	eventpp::EventQueue<EventType, void (const eventpp::AnyData<eventMaxSize> &), Policies> queue;
	queue.appendListener(EventType::key, [](const Event & e) {
		std::cout << "Received KeyEvent, key="
			<< static_cast<const KeyEvent &>(e).getKey() << std::endl;
	});
	queue.appendListener(EventType::mouse, [](const Event & e) {
		std::cout << "Received MouseEvent, x=" << static_cast<const MouseEvent &>(e).getX()
			<< " y=" << static_cast<const MouseEvent &>(e).getY() << std::endl;
	});
	queue.appendListener(EventType::message, [](const Event & e) {
		std::cout << "Received MessageEvent, message="
			<< static_cast<const MessageEvent &>(e).getMessage() << std::endl;
	});
	// Put events into the queue. Any data type, such as KeyEvent, MouseEvent, can be put
	// as long as the data size doesn't exceed eventMaxSize.
	queue.enqueue(EventType::key, KeyEvent(255));
	queue.enqueue(EventType::mouse, MouseEvent(3, 5));
	queue.enqueue(EventType::message, MessageEvent("Hello, AnyData"));
	queue.process();
}

TEST_CASE("AnyData tutorial 2, unrelated data")
{
	std::cout << std::endl << "AnyData tutorial 2, unrelated data" << std::endl;

	// It's possible to send event with data that doesn't have the same base class, such as Event.
	// To do so, the listener prototype must be `const void *` instead of `const Event &` in previous tutorial.
	struct Policies {
		using Callback = std::function<void (const EventType, const void *)>;
	};
	eventpp::EventQueue<
		EventType,
		void (const EventType, const eventpp::AnyData<eventMaxSize> &),
		Policies> queue;
	queue.appendListener(EventType::key, [](const EventType type, const void * e) {
		REQUIRE(type == EventType::key);
		std::cout << "Received KeyEvent, key=" << static_cast<const KeyEvent *>(e)->getKey() << std::endl;
	});
	queue.appendListener(EventType::mouse, [](const EventType type, const void * e) {
		REQUIRE(type == EventType::mouse);
		std::cout << "Received MouseEvent, x=" << static_cast<const MouseEvent *>(e)->getX()
			<< " y=" << static_cast<const MouseEvent *>(e)->getY() << std::endl;
	});
	queue.appendListener(EventType::message, [](const EventType type, const void * e) {
		REQUIRE(type == EventType::message);
		std::cout << "Received MessageEvent, message="
			<< static_cast<const MessageEvent *>(e)->getMessage() << std::endl;
	});
	queue.appendListener(EventType::text, [](const EventType type, const void * e) {
		REQUIRE(type == EventType::text);
		std::cout << "Received text event, text=" << *static_cast<const std::string *>(e) << std::endl;
	});
	// Send events
	queue.enqueue(EventType::key, KeyEvent(255));
	queue.enqueue(EventType::mouse, MouseEvent(3, 5));
	queue.enqueue(EventType::message, MessageEvent("Hello, AnyData"));
	// Send a std::string as the event data which doesn't derive from Event.
	queue.enqueue(EventType::text, std::string("This is a text"));
	queue.process();
}

TEST_CASE("AnyData tutorial 3, use AnyData in listener")
{
	std::cout << std::endl << "AnyData tutorial 3, use AnyData in listener" << std::endl;

	using MyData = eventpp::AnyData<eventMaxSize>;
	eventpp::EventQueue<EventType, void (const EventType, const MyData &)> queue;
	queue.appendListener(EventType::key, [](const EventType type, const MyData & e) {
		REQUIRE(type == EventType::key);
		REQUIRE(e.isType<KeyEvent>());
		std::cout << "Received KeyEvent, key=" << e.get<KeyEvent>().getKey() << std::endl;
	});
	queue.appendListener(EventType::mouse, [](const EventType type, const MyData & e) {
		REQUIRE(type == EventType::mouse);
		REQUIRE(e.isType<MouseEvent>());
		std::cout << "Received MouseEvent, x=" << e.get<MouseEvent>().getX()
			<< " y=" << e.get<MouseEvent>().getY() << std::endl;
	});
	queue.appendListener(EventType::message, [](const EventType type, const MyData & e) {
		REQUIRE(type == EventType::message);
		REQUIRE(e.isType<MessageEvent>());
		std::cout << "Received MessageEvent, message=" << e.get<MessageEvent>().getMessage() << std::endl;
	});
	queue.appendListener(EventType::text, [](const EventType type, const MyData & e) {
		REQUIRE(type == EventType::text);
		REQUIRE(e.isType<std::string>());
		std::cout << "Received text event, text=" << e.get<std::string>() << std::endl;
	});
	queue.enqueue(EventType::key, KeyEvent(255));
	queue.enqueue(EventType::mouse, MouseEvent(3, 5));
	queue.enqueue(EventType::message, MessageEvent("Hello, AnyData"));
	queue.enqueue(EventType::text, std::string("This is a text"));
	queue.process();
}
```
