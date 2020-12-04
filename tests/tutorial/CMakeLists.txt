set(TARGET_TUTORIAL tutorial)

set(SRC_TUTORIAL
	tutorialmain.cpp
	tutorial_callbacklist.cpp
	tutorial_eventdispatcher.cpp
	tutorial_eventqueue.cpp
	tutorial_hetercallbacklist.cpp
)

add_executable(
	${TARGET_TUTORIAL}
	${SRC_TUTORIAL}
)


