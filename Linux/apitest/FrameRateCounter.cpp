#include "FrameRateCounter.h"
#include <sys/time.h>
#include <unistd.h>


FrameRateCounter::FrameRateCounter( unsigned long queueLength )
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    double seconds = tv.tv_sec + tv.tv_usec * 1.0 / 1000000;

    for( unsigned int i = 0; i < queueLength; i++ )
    {
        m_frameTime.push_back( seconds );
    }
    pthread_mutex_init(&m_dequeMutex,NULL);
}

FrameRateCounter::~FrameRateCounter()
{
     pthread_mutex_destroy(&m_dequeMutex);
}

double FrameRateCounter::GetFrameRate()
{
    const unsigned int size = static_cast<unsigned int>(m_frameTime.size());

    pthread_mutex_lock(&m_dequeMutex);

    const double start = m_frameTime[0];
    const double end = m_frameTime[size-1];

    pthread_mutex_unlock(&m_dequeMutex);

    const double diff = end - start;

    double frameRate = 1.0 / (diff / static_cast<double>(size-1));    

    return frameRate;
}

void FrameRateCounter::SetFrameRate( double /*frameRate*/ )
{
    // Nothing to do here 
}

void FrameRateCounter::Reset()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    double seconds = tv.tv_sec + tv.tv_usec * 1.0 / 1000000;

    pthread_mutex_lock(&m_dequeMutex);

    const unsigned int queueSize = static_cast<unsigned int>(m_frameTime.size());
    m_frameTime.clear();

    for( unsigned int i = 0; i < queueSize; i++ )
    {
        m_frameTime.push_back( seconds );
    }
    pthread_mutex_unlock(&m_dequeMutex);
}

void FrameRateCounter::NewFrame()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    double seconds = tv.tv_sec + tv.tv_usec * 1.0 / 1000000;

    pthread_mutex_lock(&m_dequeMutex);

    m_frameTime.pop_front();
    m_frameTime.push_back( seconds );
    pthread_mutex_unlock(&m_dequeMutex);
}
