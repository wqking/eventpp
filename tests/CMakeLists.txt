project(eventpptest)

cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 11)

set(TARGET_TEST tests)

set(THIRDPARTY_PATH ../../thirdparty)

set(SRC_TEST
	testmain.cpp
	benchmark.cpp
	tutorial_callbacklist.cpp
	tutorial_eventdispatcher.cpp
	tutorial_eventqueue.cpp
	test_dispatch.cpp
	test_callbacklist.cpp
	test_queue.cpp
)

include_directories(../include)

add_executable(
	${TARGET_TEST}
	${SRC_TEST}
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${TARGET_TEST} Threads::Threads)

