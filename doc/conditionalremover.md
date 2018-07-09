# Class ConditionalRemover reference

## Description

ConditionalRemover is a utility class that automatically removes listeners after the listeners are triggered and certain condition is satisfied.  
ConditionalRemover is a pure functional class. After the member functions in ConditionalRemover are invoked, the ConditionalRemover object can be destroyed safely.  

<a name="apis"></a>
## API reference

### Header

eventpp/utilities/conditionalremover.h

### Template parameters

```c++
template <typename DispatcherType>
class ConditionalRemover;
```

`DispatcherType` can be CallbackList, EventDispatcher, or EventQueue.

### Member functions

```c++
explicit ConditionalRemover(DispatcherType & dispatcher);
```

Constructs an instance of ConditionalRemover.

**Member functions for EventDispatcher and EventQueue**
```c++
template <typename Condition>
typename DispatcherType::Handle appendListener(
		const typename DispatcherType::Event & event,
		const typename DispatcherType::Callback & listener,
		const Condition & condition
	);

template <typename Condition>
typename DispatcherType::Handle prependListener(
		const typename DispatcherType::Event & event,
		const typename DispatcherType::Callback & listener,
		const Condition & condition
	);

template <typename Condition>
typename DispatcherType::Handle insertListener(
		const typename DispatcherType::Event & event,
		const typename DispatcherType::Callback & listener,
		const typename DispatcherType::Handle & before,
		const Condition & condition
	);
```

**Member functions for CallbackList**
```c++
template <typename Condition>
typename CallbackListType::Handle append(
		const typename CallbackListType::Callback & listener,
		const Condition & condition
	);

template <typename Condition>
typename CallbackListType::Handle prepend(
		const typename CallbackListType::Callback & listener,
		const Condition & condition
	);

template <typename Condition>
typename CallbackListType::Handle insert(
		const typename CallbackListType::Callback & listener,
		const typename CallbackListType::Handle & before,
		const Condition & condition
	);
```

The member functions have the same names with the corresponding underlying class (CallbackList, EventDispatcher, or EventQueue), and also have the same parameters except there is one more parameter, `condition`. `condition` is a predicate function of prototype `bool()`. It's invoked after each trigger, if it returns true, the listener will be removed.  

### Free functions

```c++
template <typename DispatcherType>
ConditionalRemover<DispatcherType> conditionalRemover(DispatcherType & dispatcher);
```

Since ConditionalRemover takes one template parameter and it's verbose to instantiate its instance, the function `conditionalRemover` is used to construct an instance of ConditionalRemover via the deduced argument.

### Sample code

```c++
#include "eventpp/utilities/conditionalRemover.h"
#include "eventpp/eventdispatcher.h"

eventpp::EventDispatcher<int, void ()> dispatcher;
constexpr int event = 3;

dispatcher.appendListener(event, []() {
	// listener A
});

// Note the ConditionalRemover instance returned by conditionalRemover is invoked
// prependListener and destroyed immediately.
std::string removeWho;
eventpp::conditionalRemover(dispatcher).prependListener(event, [&dataList]() {
	// listener B
}, [&removeWho]() -> bool {
	return removeWho == "removeB";
});
auto handle = eventpp::conditionalRemover(dispatcher).appendListener(event, [&dataList]() {
	// listener C
}, [&removeWho]() -> bool {
	return removeWho == "removeC";
});
eventpp::conditionalRemover(dispatcher).insertListener(event, [&dataList]() {
	// listener D
}, handle, [&removeWho]() -> bool {
	return removeWho == "removeD";
});

dispatcher.dispatch(event);
// No listeners were removed since no conditions were met.

removeWho = "removeB";
dispatcher.dispatch(event);
// All listeners were triggered.
// Listener B was removed.

removeWho = "removeC";
dispatcher.dispatch(event);
// Listeners A, C, D were triggered.
// Listener C was removed.

removeWho = "removeD";
dispatcher.dispatch(event);
// Listeners A, D were triggered.
// Listener D was removed.

dispatcher.dispatch(event);
// Listeners A was triggered.

```
