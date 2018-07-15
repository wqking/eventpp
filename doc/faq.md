# Frequently Asked Questions

## Why can't rvalue reference be used as callback prototype in EventDispatcher and CallbackList? Such as CallbackList<void (int &&)>

```c++
eventpp::CallbackList<void(std::string &&)> callbackList;
callbackList("Hello"); // compile error
```

The above code doesn't compile. This is intended design and not a bug.  
A rvalue reference `std::string &&` means the argument can be moved by the callback and become invalid (or empty). Keep in mind CallbackList invokes many callbacks one by one. So what happens if the first callback moves the argument and the other callbacks get empty value? In above code example, that means the first callback sees the value "Hello" and moves it, then the other callbacks will see empty string, not "Hello"!  
To avoid such potential bugs, rvalue reference is forbidden deliberately.

## Can the callback prototype have return value? Such as CallbackList<std::string (const std::string &, int)>?

Yes you can, but both EventDispatcher and CallbackList just discard the return value. It's not efficient nor useful to return value from EventDispatcher and CallbackList.

## Why can't callback prototype be function pointer such as CallbackList<void (*)()>?

It's rather easy to support function pointer, but it's year 2018 at the time written, and there is proposal for C++20 standard already, so let's use modern C++ features. Stick with function type `void ()` instead of function pointer `void (*)()`.

## Why aren't there APIs to remove listeners directly from an EventDispatcher? Why do we have to remove by handle?

Both `EventDispatcher::removeListener(const Event & event, const Handle handle)` and `CallbackList::remove(const Handle handle)` requires the handle of a listener is passed in. So why can't we pass the listener object directly? The reason is, it's not guaranteed that the underlying callback storage is comparable while removing a listener object requires the comparable ability. Indeed the default callback storage, `std::function` is not comparable.  
If we use some customized callback storage and we are sure it's comparable, there is free functions 'removeListener' in [utility APIs](eventutil.md).

## Isn't CallbackList equivalent to std::vector<Callback>? It's simple for me to use std::vector<Callback> directly.

`CallbackList` works like a `std::vector<Callback>`. But one common usage is to implement one-shot callback that a callback removes itself from the callback list when it's invoked. In such case a simple `std::vector<Callback>` will bang and crash.  
With `CallbackList` a callback can be removed at any time, even when the callback list is under invoking.

## I want to inherit my class from EventDispatcher, but EventDispatcher's destructor is not virtual?

It's intended not to use any virtual functions in eventpp to avoid bloating the code size. New class can still inherit from EventDispatcher, as long as the object is not deleted via a pointer to EventDispatcher, which will cause resource leak. If you need to delete object via pointer to base class, make your own base class that inherits from EventDispatcher, and make the base class destructor virtual.  
For example,  

```c++
class MyEventDispatcher : public EventDispatcher<blah blah>
{
public:
	virtual ~MyEventDispatcher();
};

class MyClass : public MyEventDispatcher
{
}

MyEventDispatcher * myObject = new MyClass();
delete myObject;
```

## How to automatically remove listeners when certain object is destroyed (aka auto disconnection)?  

[Use utility class ScopedRemover](doc/scopedremover.md)

