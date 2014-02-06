#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t threads) : stop(false)
{
	for(size_t i = 0; i < threads; i++)
	{
		workers.push_back(std::thread(
			[this]
			{
				while(true)
				{
					std::unique_lock<std::mutex> lock(this->queue_mutex);
					while(!this->stop && this->tasks.empty())
					{
						this->condition.wait(lock);
					}
					if(this->stop && this->tasks.empty())
					{
						return;
					}
					std::function<void()> task(this->tasks.front());
					this->tasks.pop();
					lock.unlock();
					task();
				}
			}
		));
	}
}

ThreadPool::ThreadPool(const ThreadPool &a)
{
	this->tasks = a.tasks;
	this->stop = a.stop;

	for(size_t i = 0; i < a.workers.size(); i++)
	{
		workers.push_back(std::thread(
			[this]
			{
				while(true)
				{
					std::unique_lock<std::mutex> lock(this->queue_mutex);
					while(!this->stop && this->tasks.empty())
					{
						this->condition.wait(lock);
					}
					if(this->stop && this->tasks.empty())
					{
						return;
					}
					std::function<void()> task(this->tasks.front());
					this->tasks.pop();
					lock.unlock();
					task();
				}
			}
		));
	}
}

ThreadPool& ThreadPool::operator=(const ThreadPool &a)
{
	stop = true;
	condition.notify_all();

	for(size_t i = 0; i < workers.size(); ++i)
	{
		workers[i].join();
	}

	stop = false;
	this->tasks = a.tasks;
	this->stop = a.stop;

	for(size_t i = 0; i < a.workers.size(); i++)
	{
		workers.push_back(std::thread(
			[this]
			{
				while(true)
				{
					std::unique_lock<std::mutex> lock(this->queue_mutex);
					while(!this->stop && this->tasks.empty())
					{
						this->condition.wait(lock);
					}
					if(this->stop && this->tasks.empty())
					{
						return;
					}
					std::function<void()> task(this->tasks.front());
					this->tasks.pop();
					lock.unlock();
					task();
				}
			}
		));
	}
	return *this;
}

void ThreadPool::setCallbackFunction(std::function<void()> f)
{
	this->callBack = f;
}

ThreadPool::~ThreadPool()
{
	stop = true;
	condition.notify_all();
	for(size_t i = 0; i < workers.size(); ++i)
	{
		workers[i].join();
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	this->callBack();
}