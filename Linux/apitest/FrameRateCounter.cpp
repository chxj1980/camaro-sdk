#include "FrameRateCounter.h"
#ifdef _WIN32
#include <winsock.h>

int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
    (void)tzp;
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	static const uint64_t EPOCH = uint64_t(116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	time = uint64_t(file_time.dwLowDateTime);
	time += uint64_t(file_time.dwHighDateTime) << 32;

	tp->tv_sec = long((time - EPOCH) / 10000000L);
	tp->tv_usec = long(system_time.wMilliseconds * 1000);
	return 0;
}

#elif defined(__linux__)
#include <sys/time.h>
#include <unistd.h>
#endif



FrameRateCounter::FrameRateCounter( unsigned long queueLength )
{
    timeval tv;
    gettimeofday(&tv, nullptr);

	auto seconds = tv.tv_sec + tv.tv_usec * 1.0 / 1000000;

    for( unsigned int i = 0; i < queueLength; i++ )
    {
        m_frameTime.push_back( seconds );
    }
}

FrameRateCounter::~FrameRateCounter()
{
}

double FrameRateCounter::GetFrameRate()
{
    const unsigned int size = static_cast<unsigned int>(m_frameTime.size());

    std::unique_lock<std::mutex> ul(m_dequeMutex);

    const double start = m_frameTime[0];
    const double end = m_frameTime[size-1];

	ul.unlock();

    const double diff = end - start;

	auto frameRate = 1.0 / (diff / static_cast<double>(size-1));    

    return frameRate;
}

void FrameRateCounter::SetFrameRate( double /*frameRate*/ )
{
    // Nothing to do here 
}

void FrameRateCounter::Reset()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);

	auto seconds = tv.tv_sec + tv.tv_usec * 1.0 / 1000000;

	std::unique_lock<std::mutex> ul(m_dequeMutex);

    const unsigned int queueSize = static_cast<unsigned int>(m_frameTime.size());
    m_frameTime.clear();

    for( unsigned int i = 0; i < queueSize; i++ )
    {
        m_frameTime.push_back( seconds );
    }
	ul.unlock();
}

void FrameRateCounter::NewFrame()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);

	auto seconds = tv.tv_sec + tv.tv_usec * 1.0 / 1000000;

	std::unique_lock<std::mutex> ul(m_dequeMutex);

    m_frameTime.pop_front();
    m_frameTime.push_back( seconds );
	ul.unlock();
}
