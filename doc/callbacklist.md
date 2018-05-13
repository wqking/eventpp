# Class CallbackList reference

## Tutorials

### CallbackList tutorial 1, basic

**Code**  
```c++
// The namespace is eventpp
// the first parameter is the prototype of the listener.
eventpp::CallbackList<void ()> callbackList;

// Add a callback.
// []() {} is the callback.
// Lambda is not required, any function or std::function
// or whatever function object with the required prototype is fine.
callbackList.append([]() {
	std::cout << "Got callback 1." << std::endl;
});
callbackList.append([]() {
	std::cout << "Got callback 2." << std::endl;
});

// Invoke the callback list
callbackList();
```

**Output**  
> Got callback 1.  
> Got callback 2.  

**Remarks**  
First let's define a callback list.
```c++
eventpp::CallbackList<void ()> callbackList;
```
class CallbackList takes at least one template arguments. It is the *prototype* of the callback.  
The *prototype* is C++ function type, such as `void (int)`, `void (const std::string &, const MyClass &, int, bool)`.  

Now let's add a callback.  
```c++
callbackList.append([]() {
	std::cout << "Got callback 1." << std::endl;
});
```
Function `append` takes one arguments, the *callback*.  
The *callback* can be any callback target -- functions, pointers to functions, , pointers to member functions, lambda expressions, and function objects. It must be able to be called with the *prototype* declared in `callbackList`.  
In the tutorial, we also add another callback.  

Now let's invoke the callbackList.
```c++
callbackList();
```
During the invoking, all callbacks will be invoked one by one in the order of they were added.

### CallbackList tutorial 2, callback with parameters

**Code**  
```c++
// The callback list has two parameters.
eventpp::CallbackList<void (const std::string &, const bool)> callbackList;

callbackList.append([](const std::string & s, const bool b) {
	std::cout << std::boolalpha << "Got callback 1, s is " << s << " b is " << b << std::endl;
});
// The callback prototype doesn't need to be exactly same as the callback list.
// It would be find as long as the arguments is compatible with the dispatcher.
callbackList.append([](std::string s, int b) {
	std::cout << std::boolalpha << "Got callback 2, s is " << s << " b is " << b << std::endl;
});

// Invoke the callback list
callbackList("Hello world", true);
```

**Output**  
> Got callback 1, s is Hello world b is true  
> Got callback 2, s is Hello world b is 1  

**Remarks**  
Now the callback list prototype takes two parameters, `const std::string &` and `const bool`.  
The callback's prototype is not required to be same as the callback list, it's fine as long as the prototype is compatible with the callback list. See the second callback, `[](std::string s, int b)`, its prototype is not same as the callback list.

### CallbackList tutorial 3, remove

**Code**  
```c++
using CL = eventpp::CallbackList<void ()>;
CL callbackList;

CL::Handle handle2;

// Add some callbacks.
callbackList.append([]() {
	std::cout << "Got callback 1." << std::endl;
});
handle2 = callbackList.append([]() {
	std::cout << "Got callback 2." << std::endl;
});
callbackList.append([]() {
	std::cout << "Got callback 3." << std::endl;
});

callbackList.remove(handle2);

// Invoke the callback list
// The "Got callback 2" callback should not be triggered.
callbackList();
```

**Output**  
> Got callback 1.  
> Got callback 3.  

**Remarks**  

### CallbackList tutorial 4, for each

**Code**  
```c++
using CL = eventpp::CallbackList<void ()>;
CL callbackList;

// Add some callbacks.
callbackList.append([]() {
	std::cout << "Got callback 1." << std::endl;
});
callbackList.append([]() {
	std::cout << "Got callback 2." << std::endl;
});
callbackList.append([]() {
	std::cout << "Got callback 3." << std::endl;
});

// Now call forEach to remove the second callback
// The forEach callback prototype is void(const CallbackList::Handle & handle, const CallbackList::Callback & callback)
int index = 0;
callbackList.forEach([&callbackList, &index](const CL::Handle & handle, const CL::Callback & callback) {
	std::cout << "forEach(Handle, Callback), invoked " << index << std::endl;
	if(index == 1) {
		callbackList.remove(handle);
		std::cout << "forEach(Handle, Callback), removed second callback" << std::endl;
	}
	++index;
});

// The forEach callback prototype can also be void(const CallbackList::Handle & handle)
callbackList.forEach([&callbackList, &index](const CL::Handle & handle) {
	std::cout << "forEach(Handle), invoked" << std::endl;
});

// The forEach callback prototype can also be void(const CallbackList::Callback & callback)
callbackList.forEach([&callbackList, &index](const CL::Callback & callback) {
	std::cout << "forEach(Callback), invoked" << std::endl;
});

// Invoke the callback list
// The "Got callback 2" callback should not be triggered.
callbackList();
```

**Output**  
> Got callback 1.  
> Got callback 3.  

**Remarks**  

## API reference

**Template parameters**

```c++
template <
	typename Prototype,
	typename Callback = void,
	typename Threading = MultipleThreading
>
class CallbackList;
```
`Prototype`:  the callback prototype. It's C++ function type such as `void(int, std::string, const MyClass *)`.  
`Callback`: the underlying type to hold the callback. Default is `void`, which will be expanded to `std::function`.  
`Threading`: threading model. Default is 'MultipleThreading'. Possible values:  
  * `MultipleThreading`: the core data is protected with mutex. It's the default value.  
  * `SingleThreading`: the core data is not protected and can't be accessed from multiple threads.  

**Public types**

`Handle`: the handle type returned by appendListener, prependListener and insertListener. A handle can be used to insert a callback or remove a callback. To check if a `Handle` is empty, convert it to boolean, *false* is empty.  
`Callback`: the callback storage type.

**Functions**

```c++
CallbackList() = default;
CallbackList(CallbackList &&) = delete;
CallbackList(const CallbackList &) = delete;
CallbackList & operator = (const CallbackList &) = delete;
```

CallbackList can not be copied, moved, or assigned.

```c++
Handle append(const Callback & callback)
```  
Add the *callback* to the callback list.  
The callback is added to the end of the callback list.  
Return a handle which represents the callback. The handle can be used to remove this callback or insert other callback before this callback.  
The time complexity is O(1).

```c++
Handle prepend(const Callback & callback)
```  
Add the *callback* to the callback list.  
The callback is added to the beginning of the callback list.  
Return a handle which represents the callback. The handle can be used to remove this callback or insert other callback before this callback.  
The time complexity is O(1).

```c++
Handle insert(const Callback & callback, const Handle before)
```  
Insert the *callback* to the callback list before the callback handle *before*. If *before* is not found, *callback* is added at the end of the callback list.  
Return a handle which represents the callback. The handle can be used to remove this callback or insert other callback before this callback.  
The time complexity is O(1).  

```c++
bool remove(const Handle handle)
```  
Remove the callback *handle* from the callback list.  
Return true if the callback is removed successfully, false if the callback is not found.  
The time complexity is O(1).  

```c++
template <typename Func>  
void forEach(Func && func)
```  
Apply `func` to all callbacks.  
The `func` can be one of the three prototypes:  
```c++
AnyReturnType func(const EventDispatcher::Handle &, const EventDispatcher::Callback &);
AnyReturnType func(const EventDispatcher::Handle &);
AnyReturnType func(const EventDispatcher::Callback &);
```
**Note**: the `func` can remove any callbacks, or add other callbacks, safely.

```c++
void operator() (Args ...args)
```  
Invoke each callbacks in the callback list.  
The callbacks are called with arguments `args`.  
The callbacks are called in the thread same as the callee of `operator()`.

## Internal data structure

CallbackList uses double linked list to manage the callbacks.  
Each node is linked by shared pointer. Using shared pointer allows the node be removed while iterating.  
