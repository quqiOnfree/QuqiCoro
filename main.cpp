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

qcoro::awaiter<void> func(qcoro::executor& e)
{
	qcoro::awaiter<std::string> a([](std::function<void(std::string)> func, qcoro::executor& e) {
		e.post([func]()
			{
				std::string result;
				std::cin >> result;
				func(result);
			});

		}, e);
	std::cout << co_await a << '\n';
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
	//异步 cin
	qcoro::co_spawn(func, tp);
	// 等待所有任务完成
	tp.join();

	// 单线程执行器
	qcoro::executor e;
	//同步 cin
	qcoro::co_spawn(func, e);
	
	return 0;
}
