# QuqiCoro
c++20 coroutine library

## Import this library
```cpp
#include "QuqiCoro.hpp"

```

## Use coroutine with QuqiCoro

### coroutine
- co_yield and co_return
```cpp
// range of python in c++20 (lol)
qcoro::coroutine<int, void> range(int a, int b, int c = 1)
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
qcoro::awaiter<void> func(qcoro::executor& e)
{
	// async_cin
	qcoro::awaiter<std::string> a([](std::function<void(std::string)> func, qcoro::executor& e) {
		e.post([func]()
			{
				std::string result;
				std::cin >> result;
				func(result);
			});

		}, e);

	// get cin result
	std::cout << co_await a << '\n';
	co_return;
}

```

### executor
- run awaiter
1. normal executor
```cpp
qcoro::executor e;
qcoro::co_spawn(func, e);

```

2. thread_pool
```cpp
qcoro::thread_pool tp;
qcoro::co_spawn(func, tp);
tp.join();

```
