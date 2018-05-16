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
