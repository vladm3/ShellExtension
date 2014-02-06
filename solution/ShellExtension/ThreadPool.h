#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class ThreadPool
{
public:
	ThreadPool(size_t);
	ThreadPool(const ThreadPool &a);

	~ThreadPool();

	ThreadPool& operator=(const ThreadPool &a);

	template<class T, class F> std::future<T> enqueue(F f);

	void setCallbackFunction(std::function<void()> f);
private:
	std::vector< std::thread > workers;
	std::queue< std::function<void()> > tasks;
	std::function<void()> callBack;

	std::mutex queue_mutex;
	std::condition_variable condition;

	bool stop;
};

template<class T, class F>
std::future<T> ThreadPool::enqueue(F f)
{
	if(stop)
		throw std::runtime_error("enqueue on stopped ThreadPool");

	auto task = std::make_shared< std::packaged_task<T()> >(f);
	std::future<T> res = task->get_future();

	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		tasks.push([task](){ (*task)(); });
	}

	condition.notify_one();
	return res;
}