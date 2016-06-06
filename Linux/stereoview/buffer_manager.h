/*
 * buffer_manager.h
 *
 *  Created on: Jun 1, 2016
 *      Author: nick
 */

#ifndef SRC_PROCESSOR_BUFFER_MANAGER_H_
#define SRC_PROCESSOR_BUFFER_MANAGER_H_

#include <memory>
#include <array>
#include <map>
#include <mutex>
#include <vector>
#include <functional>
#include <chrono>
#include <iterator>

//namespace LibraF
//{

template<typename T, std::size_t DIMENSION, std::size_t BUFFER_NUM>
class BufferManager
{
public:
	typedef std::function<std::vector<std::shared_ptr<T>>(std::vector<uint8_t *> &)> Fn;
	explicit BufferManager(std::array<uint32_t, DIMENSION> &&bufferSizeList);
	~BufferManager() = default;

	std::vector<std::shared_ptr<T>> Update(Fn &&generator);
	std::pair<uint64_t, std::vector<std::shared_ptr<T>>> GetLastest();
	std::pair<uint64_t, std::vector<std::shared_ptr<T>>> GetNeighbor(uint64_t token);

private:
	struct FrameBuffer
	{
		bool used;
		std::array<std::weak_ptr<T>, DIMENSION> refs;
		std::array<std::unique_ptr<uint8_t []>, DIMENSION> buffers;
	};
	std::array<FrameBuffer, BUFFER_NUM> bufferPool;
	std::map<uint64_t, FrameBuffer *> bufferList;
	std::mutex mtx;

	std::vector<uint8_t *> FindAvailable(int &slot);
	bool UpdateSlot(int slot, std::vector<std::shared_ptr<T>> &frames);
};

template<typename T, std::size_t DIMENSION, std::size_t BUFFER_NUM>
BufferManager<T, DIMENSION, BUFFER_NUM>::BufferManager(std::array<uint32_t, DIMENSION> &&bufferSizeList)
{
	for(auto &item : bufferPool)
	{
		item.used = false;
		for(auto i=0; i<DIMENSION; ++i)
			item.buffers[i]= std::unique_ptr<uint8_t[]>(new uint8_t[bufferSizeList[i]]);
	}
}

template<typename T, std::size_t DIMENSION, std::size_t BUFFER_NUM>
std::vector<uint8_t *> BufferManager<T, DIMENSION, BUFFER_NUM>::FindAvailable(int &slot)
{
	std::lock_guard<std::mutex> lg(mtx);
	//Recycle
	auto it = bufferList.begin();
	while(it != bufferList.end())
	{
		FrameBuffer &fb = *(it->second);
		auto expired = true;
		for(auto &ref : fb.refs)
		{
			if (!ref.expired())
			{
				expired = false;
				break;
			}
		}

		if (expired)
		{
			fb.used = false;
			it = bufferList.erase(it);
		}
		else
			++it;
	}
	//Search
	auto i = 0;
	for(auto &item : bufferPool)
	{
		if (!item.used)
		{
			slot = i;
			std::vector<uint8_t *> result;
			for(auto &buffer : item.buffers)
                result.emplace_back(buffer.get());
			return result;
		}
		++i;
	}
	return {};
}

template<typename T, std::size_t DIMENSION, std::size_t BUFFER_NUM>
bool BufferManager<T, DIMENSION, BUFFER_NUM>::UpdateSlot(int slot, std::vector<std::shared_ptr<T>> &frames)
{
	if (frames.size()!=DIMENSION)
		return false;
	std::lock_guard<std::mutex> lg(mtx);
	bufferPool[slot].used = true;
	for(auto i=0;i<DIMENSION;++i)
		bufferPool[slot].refs[i] = frames[i];
	auto tm = std::chrono::duration_cast<std::chrono::microseconds>(
	                std::chrono::high_resolution_clock::now().time_since_epoch());
	bufferList.emplace(tm.count(), &bufferPool[slot]);
	return true;
}

template<typename T, std::size_t DIMENSION, std::size_t BUFFER_NUM>
std::vector<std::shared_ptr<T>> BufferManager<T, DIMENSION, BUFFER_NUM>::Update(Fn &&generator)
{
    if (generator == nullptr)
		return {};
	int slot;
	auto ptrs = FindAvailable(slot);
	if (ptrs.empty())
		return {};
    auto frames = generator(ptrs);
	if (!UpdateSlot(slot, frames))
		return {};
	return frames;
}

template<typename T, std::size_t DIMENSION, std::size_t BUFFER_NUM>
std::pair<uint64_t, std::vector<std::shared_ptr<T>>> BufferManager<T, DIMENSION, BUFFER_NUM>::GetLastest()
{
	std::lock_guard<std::mutex> lg(mtx);

	auto it = bufferList.rbegin();
	if (it==bufferList.rend())
		return {};
	FrameBuffer &fb = *(it->second);
	std::vector<std::shared_ptr<T>> vector;
	for(auto &ref : fb.refs)
		vector.emplace_back(ref.lock());
	return std::make_pair(it->first, std::move(vector));
}

template<typename T, std::size_t DIMENSION, std::size_t BUFFER_NUM>
std::pair<uint64_t, std::vector<std::shared_ptr<T>>> BufferManager<T, DIMENSION, BUFFER_NUM>::GetNeighbor(uint64_t token)
{
	std::lock_guard<std::mutex> lg(mtx);
	auto it = bufferList.find(token);
	if (it==bufferList.end())
		return {};
	auto next = std::next(it);
	auto prev = std::prev(it);

	int64_t diff = -1;

	it=bufferList.end();

	if (next!=bufferList.end())
	{
		diff = it->first - token;
		it = next;
	}
	if (prev!=bufferList.end())
	{
		if (diff<0 || diff>token-it->first)
			it = prev;
	}

	if (it==bufferList.end())
		return {};

	FrameBuffer &fb = *(it->second);
	std::vector<std::shared_ptr<T>> vector;
	for(auto &ref : fb.refs)
		vector.emplace_back(ref.lock());
	return std::make_pair(it->first, std::move(vector));
}

//} /* namespace LibraF */

#endif /* SRC_PROCESSOR_BUFFER_MANAGER_H_ */
