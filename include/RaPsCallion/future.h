#pragma once

#ifndef BOOST_THREAD_PROVIDES_FUTURE
# error boost::future<T> not provided instead of boost::unique_future<T>
#endif
#ifndef BOOST_THREAD_PROVIDES_EXECUTORS
# error boost::future<T> does not provide executors
#endif
#ifndef BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
# error boost::when_(all|any)(boost::future<T>...) not provided
#endif
#ifndef BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
# error boost::future<T>::then([](boost::future<T>){}) not provided
#endif

#include <boost/thread/future.hpp>

template <typename T>
using future = boost::future<T>;

template <typename T>
using promise = boost::promise<T>;
