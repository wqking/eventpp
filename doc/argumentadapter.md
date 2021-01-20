# Argument adapter reference

## Description

The header file `eventpp/utilities/argumentadapter.h` contains utilities that can cast pass-in argument types to the types of the functioning being called. It's as if the argument types are casted using `static_cast` for most types or `static_pointer_cast` for shared pointers.  

For example,  
```
struct Event
{
};

struct MouseEvent : public Event
{
};

eventpp::EventDispatcher<int, void (const Event &)> dispatcher;

// Below line won't compile because the listener must `const Event &` can't be converted to `const MouseEvent &` explicitly.
//dispatcher.appendListener(ON_MOUSE_DOWN, [](const MouseEvent &) {});

// This line works.
dispatcher.appendListener(ON_MOUSE_DOWN, eventpp::argumentAdapter<void(const MouseEvent &)>([](const MouseEvent &) {}));
```

# Utilities reference

## Header

eventpp/utilities/argumentadapter.h

## API reference

```c++
template <template parameters>
ArgumentAdapter<template parameters> argumentAdapter(Func func);
```

Function `argumentAdapter` receives a function `func`, and return a functor object of `ArgumentAdapter`. `ArgumentAdapter` has a function invoking operator that can cast the arguments to match the parameter types of `func`. The return value of `argumentAdapter` can be passed to CallbackList, EventDispatcher, or EventQueue.   
If `func` is a `std::function`, or a pointer to free function, `argumentAdapter` can deduce the parameter types of func, then `argumentAdapter` can be called without any template parameter.  
If `func` is a functor object that `argumentAdapter` can't deduce the parameter types, `argumentAdapter` needs a template parameter which is the prototype of `func`.  
`ArgumentAdapter` converts argument types using `static_cast`. For `std::shared_ptr`, `std::static_pointer_cast` is used. If `static_cast` or `std::static_pointer_cast` can't convert the types, compile errors are issued.

Below is the example code to demonstrate how to use `argumentAdapter`. There are full compile-able example code in file 'tests/tutorial/tutorial_argumentadapter.cpp '.


```
// In the tutorials here, we will use two event class, MouseEvent derives from Event.
// The callback prototype in EventDispatcher is reference or pointer to Event,
// then we should only be able to add listeners that only accept reference or pointer to Event,
// not MouseEvent.
// But with argumentAdapter, the listeners can accept reference or pointer to MouseEvent,
// and argumentAdapter converts any reference or pointer to Event to MouseEvent automatically, as
// long as object pointed to the reference or pointer is a MouseEvent.

class Event
{
};

class MouseEvent : public Event
{
public:
	MouseEvent(const int x, const int y) : x(x), y(y) {
	}

	int getX() const { return x; }
	int getY() const { return y; }

private:
	int x;
	int y;
};

constexpr int mouseEventId = 3;

// A free function that will be added as listener later.
// argumentAdapter works on all types of callables, include but not limited to,
// lambda, functor object, std::function, free function, etc.
void tutorialArgumentAdapterFreeFunction(const MouseEvent & e)
{
	std::cout << "Received MouseEvent in free function, x=" << e.getX() << " y=" << e.getY() << std::endl;
}

void main()
{
	std::cout << std::endl << "ArgumentAdapter tutorial 1, basic" << std::endl;

	eventpp::EventDispatcher<int, void (const Event &)> eventDispatcher;

	// callback 1 -- lambda, or any functor object

	// This can't compile because a 'const Event &' can be passed to 'const MouseEvent &'
	//eventDispatcher.appendListener(mouseEventId, [](const MouseEvent & e) {});

	// This compiles. eventpp::argumentAdapter creates a functor object that static_cast 
	// 'const Event &' to 'const MouseEvent &' automatically.
	// Note we need to pass the function type to eventpp::argumentAdapter because the lambda
	// doesn't have any function type information and eventpp::argumentAdapter can't deduce
	// the type. This rule also applies to other functor object.
	eventDispatcher.appendListener(
		mouseEventId,
		eventpp::argumentAdapter<void(const MouseEvent &)>([](const MouseEvent & e) {
			std::cout << "Received MouseEvent in lambda, x=" << e.getX() << " y=" << e.getY() << std::endl;
		})
	);

	// callback 2 -- std::function
	// We don't need to pass the function type to eventpp::argumentAdapter because it can
	// deduce the type from the std::function
	eventDispatcher.appendListener(
		mouseEventId,
		eventpp::argumentAdapter(std::function<void(const MouseEvent &)>([](const MouseEvent & e) {
			std::cout << "Received MouseEvent in std::function, x=" << e.getX() << " y=" << e.getY() << std::endl;
		}))
	);

	// callback 3 -- free function
	// We don't need to pass the function type to eventpp::argumentAdapter because it can
	// deduce the type from the free function
	eventDispatcher.appendListener(
		mouseEventId,
		eventpp::argumentAdapter(tutorialArgumentAdapterFreeFunction)
	);

	eventDispatcher.dispatch(mouseEventId, MouseEvent(3, 5));
}

```
