#pragma once

#include <future>
#include <mutex>
#include <queue>

namespace thread_pool
{

class ThreadPool
{
public:
	explicit ThreadPool(const size_t threads)
	{
		for (size_t i = 0; i < threads; ++i) {
			workers_.emplace_back(
				[this]
			{
				while (true) {
					std::function<void()> task;

					{
						std::unique_lock lock(queue_mutex_);
						condition_.wait(lock,
							[this] { return stop_ || !tasks_.empty(); });
						if (stop_ && tasks_.empty()) {
							return;
						}
						task = std::move(tasks_.front());
						tasks_.pop();
					}

					try {
						task();
					}
					catch (...) {
					}
				}
			}
			);
		}
	}

	ThreadPool(const ThreadPool&) = delete;

	ThreadPool(ThreadPool&&) = delete;

	ThreadPool& operator=(const ThreadPool&) = delete;

	ThreadPool& operator=(ThreadPool&&) = delete;

	template<class F, class... Args>
	auto Enqueue(F&& f, Args&&... args)
		-> std::future<std::invoke_result_t<F, Args...>>
	{
		using ReturnType = std::invoke_result_t<F, Args...>;

		auto task = std::make_shared<std::packaged_task<ReturnType()>>(
			[Func = std::forward<F>(f)] { return Func(); }
		);

		std::future<ReturnType> res = task->get_future();

		{
			std::unique_lock lock(queue_mutex_);

			// Don't allow enqueueing after stopping the pool.
			if (stop_) {
				throw std::runtime_error("Enqueue on stopped ThreadPool.");
			}

			tasks_.emplace([task] { (*task)(); });
		}

		condition_.notify_one();
		return res;
	}

	~ThreadPool()
	{
		{
			std::lock_guard lock(queue_mutex_);
			stop_ = true;
		}

		condition_.notify_all();
		for (std::thread& worker : workers_) {
			worker.join();
		}
	}

private:
	// The worker threads.
	std::vector<std::thread> workers_;
	// The tasks queue.
	std::queue<std::function<void()>> tasks_;

	// For synchronization.
	std::mutex queue_mutex_;
	std::condition_variable condition_;
	bool stop_ = false;
};

}
