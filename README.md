# QuqiCoro
c++20 coroutine library

## Import this library
```cpp
#include "QuqiCoro.hpp"

```

## Use coroutine with QuqiCoro

### generator
- co_yield and co_return
```cpp
// range of python in c++20 (lol)
qcoro::generator<int, void> range(int a, int b, int c = 1)
{
	for (int i = 0; i < b; i += c)
	{
		co_yield i;
	}
	co_return;
}

for (auto i : range(0, 100))
{
	cout << i << '\n';
}

```

### awaiter
- co_await and co_return
```cpp
qcoro::awaiter<void> func()
{
	qcoro::awaiter<std::string> a([](std::function<void(std::string)> func, qcoro::executor& e) {
		e.post([func]()
			{
				std::string result;
				std::cin >> result;
				func(result);
			});

		}, qcoro::use_await_t);
	co_await a;
	co_return;
}

```

### executor
- run awaiter
1. normal executor
```cpp
// executor use_await_t; -> in "QuqiCoro.hpp"
qcoro::co_spawn(func, qcoro::use_await_t);

```

2. thread_pool_executor
```cpp
// multithread
qcoro::thread_pool_executor tp;
qcoro::co_spawn(func, tp);

// wait
tp.join();

```
