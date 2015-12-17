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
		bool Push(T item);
		bool Pop(T &item);	//Keep waiting until any item popped, if noWait is true
		bool Pop_NoWait(T &item);
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
	}

	template <class T>
	BufferQueue<T>::~BufferQueue()
	{
	}

	template <class T>
	bool BufferQueue<T>::Push(T item)
	{
        std::lock_guard<std::mutex> lk(mtx);
		auto result = false;
		if (sizeLimit == 0 || queue.size() < sizeLimit)
		{
			queue.push(std::move(item));
			cond.notify_one();
			result = true;
		}
		return result;
	}

	template <class T>
	bool BufferQueue<T>::Pop_NoWait(T& item)
	{
		std::lock_guard<std::mutex> lk(mtx);
		if (queue.empty())
			return false;
		item = std::move(queue.front());
		queue.pop();
		return true;
	}

	template <class T>
	inline bool BufferQueue<T>::Pop(T &item)
	{
		std::unique_lock<std::mutex> lk(mtx);
		discarded = false;
        while (queue.empty())
		{
			cond.wait(lk);
			if (discarded)
				return false;
        }
        item = std::move(queue.front());
		queue.pop();
		return true;
	}

	template <class T>
	void BufferQueue<T>::Discard()
	{
        std::lock_guard<std::mutex> lk(mtx);
		discarded = true;
		cond.notify_one();
	}

	template <class T>
	bool BufferQueue<T>::Empty()
	{
        std::lock_guard<std::mutex> lk(mtx);
        return queue.empty();
	}
}

