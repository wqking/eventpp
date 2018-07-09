# Class CallbackList reference

## Table Of Contents

- [API reference](#apis)
- [Nested callback safety](#nested-callback-safety)
- [Time complexities](#time-complexities)
- [Internal data structure](#internal-data-structure)

## Description

CallbackList is the fundamental class in eventpp. The other classes EventDispatcher and EventQueue are built on CallbackList.  

CallbackList holds a list of callbacks. On invocation, CallbackList simply invokes each callbacks one by one. Think CallbackList as the signal/slot system in Qt, or the callback function pointer in some Windows APIs (such as lpCompletionRoutine in `ReadFileEx`).  

<a name="apis"></a>
## API reference

### Header

eventpp/callbacklist.h

**Template parameters**

```c++
template <
	typename Prototype,
	typename Policies = DefaultPolicies
>
class CallbackList;
```
`Prototype`:  the callback prototype. It's C++ function type such as `void(int, std::string, const MyClass *)`.  
`Policies`: the policies to configure and extend the callback list. The default value is `DefaultPolicies`. See [document of policies](policies.md) for details.  

### Public types

`Handle`: the handle type returned by appendListener, prependListener and insertListener. A handle can be used to insert a callback or remove a callback. To check if a `Handle` is empty, convert it to boolean, *false* is empty. `Handle` is copyable.  
`Callback`: the callback storage type.

### Member functions

```c++
CallbackList() noexcept;
CallbackList(const CallbackList & other);
CallbackList(CallbackList && other) noexcept;
CallbackList & operator = (const CallbackList & other);
CallbackList & operator = (CallbackList && other) noexcept;
```

CallbackList can be copied, moved,  assigned, and move assigned.

```c++
bool empty() const;
```
Return true if the callback list is empty.  
Note: in multi threading, this function returning true doesn't guarantee the list is empty. The list may become non-empty immediately after the function returns true.

```c++
operator bool() const;
```
Return true if the callback list is not empty.  
This operator allows a CallbackList instance be used in condition statement.

```c++
Handle append(const Callback & callback);
```  
Add the *callback* to the callback list.  
The callback is added to the end of the callback list.  
Return a handle which represents the callback. The handle can be used to remove this callback or insert other callback before this callback.  
If `append` is called in another callback during the invoking of the callback list, the new callback is guaranteed not triggered during the same callback list invoking.  
The time complexity is O(1).

```c++
Handle prepend(const Callback & callback);
```  
Add the *callback* to the callback list.  
The callback is added to the beginning of the callback list.  
Return a handle which represents the callback. The handle can be used to remove this callback or insert other callback before this callback.  
If `prepend` is called in another callback during the invoking of the callback list, the new callback is guaranteed not triggered during the same callback list invoking.  
The time complexity is O(1).

```c++
Handle insert(const Callback & callback, const Handle before);
```  
Insert the *callback* to the callback list before the callback handle *before*. If *before* is not found, *callback* is added at the end of the callback list.  
Return a handle which represents the callback. The handle can be used to remove this callback or insert other callback before this callback.  
If `insert` is called in another callback during the invoking of the callback list, the new callback is guaranteed not triggered during the same callback list invoking.  
The time complexity is O(1).  

```c++
bool remove(const Handle handle);
```  
Remove the callback *handle* from the callback list.  
Return true if the callback is removed successfully, false if the callback is not found.  
The time complexity is O(1).  

```c++
template <typename Func>  
void forEach(Func && func);
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
template <typename Func>  
bool forEachIf(Func && func);
```  
Apply `func` to all callbacks. `func` must return a boolean value, and if the return value is false, forEachIf stops the looping immediately.  
Return `true` if all callbacks are invoked, or `event` is not found, `false` if `func` returns `false`.

```c++
void operator() (Args ...args);
```  
Invoke each callbacks in the callback list.  
The callbacks are called with arguments `args`.  
The callbacks are called in the thread same as the callee of `operator()`.

<a name="nested-callback-safety"></a>
## Nested callback safety
1. If a callback adds another callback to the callback list during a invoking, the new callback is guaranteed not to be triggered within the same invoking. This is guaranteed by an unsigned 64 bits integer counter. This rule will be broken is the counter is overflowed to zero in a invoking, but this rule will continue working on the subsequence invoking.  
2. Any callbacks that are removed during a invoking are guaranteed not triggered.  
3. All above points are not true in multiple threading. That's to say, if one thread is invoking a callback list, the other thread add or remove a callback, the added or removed callback may be called during the invoking.


<a name="time-complexities"></a>
## Time complexities
- `append`: O(1)
- `prepend`: O(1)
- `insert`: O(1)
- `remove`: O(1)

<a name="internal-data-structure"></a>
## Internal data structure

CallbackList uses doubly linked list to manage the callbacks.  
Each node is linked by shared pointer. Using shared pointer allows the node be removed while iterating.  
