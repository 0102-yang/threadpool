#include "CppUnitTest.h"

#include <chrono>
#include <future>
#include <thread>
#include <vector>

#include "ThreadPool.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace thread_pool;

namespace thread_pool_unit_test
{

TEST_CLASS(ThreadPoolUnitTest)
{
public:
	TEST_METHOD(TestBasic)
	{
		ThreadPool pool(1);

		auto f1 = pool.Enqueue([] { return 6; });
		auto f2 = pool.Enqueue([] { return 42; });
		auto f3 = pool.Enqueue([] { return 100; });

		Assert::AreEqual(6, f1.get());
		Assert::AreEqual(42, f2.get());
		Assert::AreEqual(100, f3.get());
	}

	TEST_METHOD(TestConcurrencyBasic)
	{
		ThreadPool pool(4);
		std::vector<std::future<int>> futures;
		for (int i = 0; i < 8; ++i) {
			futures.push_back(pool.Enqueue([i]() { return i; }));
		}
		for (int i = 0; i < 8; ++i) {
			Assert::AreEqual(i, futures[i].get());
		}
	}

	TEST_METHOD(TestThreadPoolDestruction)
	{
		std::atomic counter = 0;
		{
			ThreadPool pool(2);
			for (int i = 0; i < 5; ++i) {
				pool.Enqueue([&counter]
				{
					++counter;
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				});
			}
		}
		Assert::AreEqual(5, counter.load(std::memory_order::memory_order_acquire));
	}
};

}
