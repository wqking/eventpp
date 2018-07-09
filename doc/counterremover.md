# Class CounterRemover reference

## Description

CounterRemover is a utility class that automatically removes listeners after the listeners are triggered for certain times.  
CounterRemover is a pure functional class. After the member functions in CounterRemover are invoked, the CounterRemover object can be destroyed safely.  

<a name="apis"></a>
## API reference

### Header

eventpp/utilities/counterremover.h

### Template parameters

```c++
template <typename DispatcherType>
class CounterRemover;
```

`DispatcherType` can be CallbackList, EventDispatcher, or EventQueue.

### Member functions

```c++
explicit CounterRemover(DispatcherType & dispatcher);
```

Constructs an instance of CounterRemover.

**Member functions for EventDispatcher and EventQueue**
```c++
typename DispatcherType::Handle appendListener(
		const typename DispatcherType::Event & event,
		const typename DispatcherType::Callback & listener,
		const int triggerCount = 1
	);

typename DispatcherType::Handle prependListener(
		const typename DispatcherType::Event & event,
		const typename DispatcherType::Callback & listener,
		const int triggerCount = 1
	);

typename DispatcherType::Handle insertListener(
		const typename DispatcherType::Event & event,
		const typename DispatcherType::Callback & listener,
		const typename DispatcherType::Handle & before,
		const int triggerCount = 1
	);
```

**Member functions for CallbackList**
```c++
typename CallbackListType::Handle append(
		const typename CallbackListType::Callback & listener,
		const int triggerCount = 1
	);

typename CallbackListType::Handle prepend(
		const typename CallbackListType::Callback & listener,
		const int triggerCount = 1
	);

typename CallbackListType::Handle insert(
		const typename CallbackListType::Callback & listener,
		const typename CallbackListType::Handle & before,
		const int triggerCount = 1
	);
```

The member functions have the same names with the corresponding underlying class (CallbackList, EventDispatcher, or EventQueue), and also have the same parameters except there is one more parameter, `triggerCount`. `triggerCount` is decreased by one on each trigger, and when `triggerCount` is zero or negative, the listener will be removed.  
The default value of `triggerCount` is 1, that means the listener is removed after the first trigger, which is one shot listener.

### Free functions

```c++
template <typename DispatcherType>
CounterRemover<DispatcherType> counterRemover(DispatcherType & dispatcher);
```

Since CounterRemover takes one template parameter and it's verbose to instantiate its instance, the function `counterRemover` is used to construct an instance of CounterRemover via the deduced argument.

### Sample code

```c++
#include "eventpp/utilities/counterremover.h"
#include "eventpp/eventdispatcher.h"

eventpp::EventDispatcher<int, void ()> dispatcher;
constexpr int event = 3;

dispatcher.appendListener(event, []() {
	// listener A
});

// Note the CounterRemover instance returned by counterRemover is invoked
// prependListener and destroyed immediately.
eventpp::counterRemover(dispatcher).prependListener(event, []() {
	// listener B
});
auto handle = eventpp::counterRemover(dispatcher).appendListener(event, []() {
	// listener C
}, 2);
eventpp::counterRemover(dispatcher).insertListener(event, []() {
	// listener D
}, handle, 3);

dispatcher.dispatch(event);
// All listeners were triggered.
// Listener B was removed.

dispatcher.dispatch(event);
// Listeners A, C, D were triggered.
// Listener C was removed.

dispatcher.dispatch(event);
// Listeners A, D were triggered.
// Listener D was removed.

dispatcher.dispatch(event);
// Listeners A was triggered.

```
