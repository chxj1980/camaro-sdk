#pragma once

// Concurrent Queue for Buffer
// Standard C++11 Implementation
// Nick Luo

// ReSharper disable once CppUnusedIncludeDirective
#include <condition_variable>
#include <thread>
#include <queue>
#include <mutex>

namespace TopGear
{
	template<class T>
	class BufferQueue
	{
	public:
		explicit BufferQueue(size_t limit = 0);
		virtual ~BufferQueue();
		BufferQueue(const BufferQueue&) = delete;            // disable copying
		BufferQueue& operator=(const BufferQueue&) = delete; // disable assignment
		bool Push(const T &item);
		bool Pop(T &item, bool noWait = false);	//Keep waiting until any item popped, if noWait is true
		void Discard();		//Stop popping waiting
		bool Empty();
	private:
		std::queue<T> queue;
		std::mutex mtx;
		std::condition_variable cond;
		size_t sizeLimit;
		bool discarded;
	};

	template <class T>
	BufferQueue<T>::BufferQueue(size_t limit)
		:sizeLimit(limit), discarded(false)
	{
		std::this_thread::get_id();
	}

	template <class T>
	BufferQueue<T>::~BufferQueue()
	{
	}

	template <class T>
	bool BufferQueue<T>::Push(const T &item)
	{
		std::unique_lock<std::mutex> mlock(mtx);
		auto result = false;
		if (sizeLimit == 0 || queue.size() < sizeLimit)
		{
			queue.push(item);
			mlock.unlock();
			cond.notify_one();
			result = true;
		}
		return result;
	}

	template <class T>
	inline bool BufferQueue<T>::Pop(T &item, bool noWait)
	{
		std::unique_lock<std::mutex> mlock(mtx);
		discarded = false;
		if (noWait && queue.empty())
			return false;
		while (queue.empty())
		{
			cond.wait(mlock);
			if (discarded)
				return false;
		}
		item = queue.front();
		queue.pop();
		return true;
	}

	template <class T>
	void BufferQueue<T>::Discard()
	{
		std::unique_lock<std::mutex> mlock(mtx);
		discarded = true;
		mlock.unlock();
		cond.notify_one();
	}

	template <class T>
	bool BufferQueue<T>::Empty()
	{
		mtx.lock();
		auto empty = queue.empty();
		mtx.unlock();
		return empty;
	}
}

