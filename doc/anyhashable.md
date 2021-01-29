# Class AnyHashable and AnyHashableValue reference

## Description

The class `AnyHashable` can be used as the event ID type in `EventDispatcher` and `EventQueue`, then any hashable types can be used as the event type.

For example,  

```c++
eventpp::EventQueue<eventpp::AnyHashable, void()> eventQueue;

eventQueue.appendListener(3, []() {}); // listener 1
eventQueue.appendListener(std::string("hello"), []() {}); // listener 2

eventQueue.dispatch(3); // trigger listener 1
eventQueue.dispatch(std::string("hello")); // trigger listener 2
```

`AnyHashableValue` is similar to `AnyHashable`. `AnyHashableValue` holds the underlying event ID and can be obtained by the users. `AnyHashable` doesn't hold the underlying event ID for better performance.

## Header

AnyHashable  
eventpp/utilities/anyhashable.h  

AnyHashableValue  
eventpp/utilities/anyhashablevalue.h

## Details

Any values of hashable types can be converted to `AnyHashable` implicit, thus the values can be passed to `EventDispatcher` or `EventQueue` as the event type.  
A hashable type is a type that can be used with `std::hash`.  
Note: the hash value calculated by `std::hash` has chances to collide. Two different event IDs with the same hash will be treated as the same ID.  

`AnyHashable` has only one member function,

```c++
std::size_t AnyHashable::getHash() const;
```

The function returns the hash value.

`AnyHashableValue` supports `getHash` as well, and another member function,

```c++
const std::any & AnyHashableValue::getValue() const;
```

`getValue` returns the underlying event ID value in `std::any`.  

Example code for `AnyHashableValue`,  

```c++
eventpp::EventQueue<
		eventpp::AnyHashableValue,
		void(const eventpp::AnyHashableValue & e)
	> eventQueue;

eventQueue.appendListener(
	3,
	[](const eventpp::AnyHashableValue & e) {
		assert(std::any_cast<int>(e.getValue()) == 3);
	}
);
eventQueue.appendListener(
	std::string("hello"),
	[](const eventpp::AnyHashableValue & e) {
		assert(std::any_cast<std::string>(e.getValue()) == "hello");
	}
);

eventQueue.enqueue(3);
eventQueue.process();
eventQueue.dispatch(std::string("hello"));
```

## When to use AnyHashable and AnyHashableValue?

Even though `AnyHashable` and `AnyHashableValue` look smart and very flexible, I highly don't encourage you to use them at all because that means the architecture has flaws. You should always prefer to single event type, such as `int`, or `std::string`, than mixing them.  
If you want to use `AnyHashable` or `AnyHashableValue`, don't forget to take into account of the collision created by `std::hash`, and be sure your event IDs don't collide with each other.  
If you find there are good reasons to mix the event types and there are good cases to use `AnyHashable` and `AnyHashableValue`, you can let me know.  
