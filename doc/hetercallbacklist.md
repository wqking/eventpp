# Class HeterCallbackList reference

## Table Of Contents

<!--toc-->

## Description

HeterCallbackList is the heterogeneous counterpart of CallbackList in eventpp. The other classes HeterEventDispatcher and HeterEventQueue are built on HeterCallbackList.  

HeterCallbackList holds a list of callbacks. The callbacks can have different prototypes. At the time of the call, HeterCallbackList invokes each callback which matches the invoking parameters.  
The *callback* can be any callback target -- functions, pointers to functions, , pointers to member functions, lambda expressions, and function objects.  

## API reference

### Header

eventpp/hetercallbacklist.h

### Template parameters

```c++
template <
	typename PrototypeList,
	typename Policies = DefaultPolicies
>
class HeterCallbackList;
```
`Prototype`:  a list of function types in `eventpp::HeterTuple`, such as `eventpp::HeterTuple<void (), void (std::string), void (int, int)>`.  
`Policies`: the policies to configure and extend the callback list. The default value is `DefaultPolicies`. See [document of policies](policies.md) for details.  

### Public types

`Handle`: the handle type returned by append, prepend and insert. A handle can be used to insert a callback or remove a callback. To check if a `Handle` is empty, convert it to boolean, *false* is empty. `Handle` is copyable.  
`Callback`: the callback storage type.

### Member functions

```c++
HeterCallbackList() noexcept;
HeterCallbackList(const HeterCallbackList & other);
HeterCallbackList(HeterCallbackList && other) noexcept;
HeterCallbackList & operator = (const HeterCallbackList & other);
HeterCallbackList & operator = (HeterCallbackList && other) noexcept;
```

HeterCallbackList can be copied, moved,  assigned, and move assigned.

```c++
bool empty() const;
```
Return true if the callback list is empty.  
Note: in multi threading, this function returning true doesn't guarantee that the list is empty. The list may immediately become non-empty after the function returns true.

```c++
operator bool() const;
```
Return true if the callback list is not empty.  
This operator allows a HeterCallbackList instance be used in condition statement.

```c++
template <typename C>
Handle append(const C & callback);
```  
Add the `callback` to the callback list.  
The callback is added to the end of the callback list.  
The callback type `C` must be specified in `PrototypeList`.  
Return a handle that represents the callback. The handle can be used to remove this callback or to insert additional callbacks before this callback.  
If `append` is called in another callback during the invoking of the callback list, the new callback is guaranteed not to be triggered during the same callback list invoking.  
The time complexity is O(1).

```c++
template <typename C>
Handle prepend(const C & callback);
```  
Add the *callback* to the callback list.  
The callback is added to the beginning of the callback list.  
The callback type `C` must be specified in `PrototypeList`.  
Return a handle that represents the callback. The handle can be used to remove this callback or to insert additional callbacks before this callback.  
If `prepend` is called in another callback during the invoking of the callback list, the new callback is guaranteed not to be triggered during the same callback list invoking.  
The time complexity is O(1).

```c++
template <typename C>
Handle insert(const C & callback, const Handle & before);
```  
Insert the *callback* to the callback list before the callback handle *before*. If *before* is not found, *callback* is added at the end of the callback list.  
The callback type `C` must be specified in `PrototypeList`.  
Return a handle that represents the callback. The handle can be used to remove this callback or to insert additional callbacks before this callback.  
If `insert` is called in another callback during the invoking of the callback list, the new callback is guaranteed not to be triggered during the same callback list invoking.  
The time complexity is O(1).  

```c++
bool remove(const Handle & handle);
```  
Remove the callback *handle* from the callback list.  
Return true if the callback is removed successfully, false if the callback is not found.  
The time complexity is O(1).  

```c++
template <typename Prototype, typename Func>
void forEach(Func && func) const;
```  
Apply `func` to all callbacks which has the `Prototype`.  
The `func` can be one of the two prototypes:  
```c++
AnyReturnType func(const HeterCallbackList::Handle &, const std::function<Prototype> &);
AnyReturnType func(const std::function<Prototype> &);
```
**Note**: the `func` can remove any callbacks, or add other callbacks, safely.

```c++
template <typename Prototype, typename Func>
bool forEachIf(Func && func) const;
```  
Apply `func` to all callbacks. `func` must return a boolean value, and if the return value is false, forEachIf stops the looping immediately.  
Return `true` if all callbacks are invoked, or `event` is not found, `false` if `func` returns `false`.

```c++
template <typename ...Args>
void operator() (Args && ...args) const;
```  
Invoke each callbacks that can be called with `Args` in the callback list.  
The callbacks are called with arguments `args`.  
The callbacks are called in the thread same as the callee of `operator()`.

