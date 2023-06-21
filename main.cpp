// QuqiCoro.cpp: 定义应用程序的入口点。
//

#include <iostream>
#include <string>
#include <thread>

#include "QuqiCoro.hpp"

using namespace std;

qcoro::generator<int, void> range(int a, int b, int c = 1)
{
	for (int i = 0; i < b; i += c)
	{
		co_yield i;
	}
	co_return;
}

qcoro::generator<std::string, void> strs()
{
	std::vector<std::string> a = { "123","456","789" };
	for (auto i : a)
	{
		co_yield i;
	}
	co_return;
}

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

int main()
{
	// range
	for (auto i : range(0, 100))
	{
		cout << i << '\n';
	}

	// strs
	for (auto i : strs())
	{
		cout << i << '\n';
	}

	// 多线程执行器
	qcoro::thread_pool_executor tp;
	//异步
	qcoro::co_spawn(func, tp);
	// 等待所有任务完成
	tp.join();

	// 单线程执行器
	//同步
	qcoro::co_spawn(func, qcoro::use_await_t);

	std::system("pause");
	
	return 0;
}
