
#ifndef _FRAMERATECOUNTER_H
#define _FRAMERATECOUNTER_H

#include <pthread.h>
#include <deque>


/**
 * Frame rate counter class. Calculates frame rate using an average of n
 * images.
 */ 
class FrameRateCounter  
{
public:
    /** Constructor. */
    FrameRateCounter( unsigned long queueLength = 10 );

    /** Destructor. */
    virtual ~FrameRateCounter();

    /**
     * Get the frame rate.
     *
     * @return Frame rate.
     */
    double GetFrameRate();

    /**
     * Set the frame rate. This function is not implemented.
     *
     * @param frameRate The frame rate to set.
     */
    void SetFrameRate( double frameRate );

    /** Resets the counter. */
    void Reset();

    /** Inform the class that there is a new frame. */
    void NewFrame();

protected:
    std::deque<double>  m_frameTime; 

    pthread_mutex_t  m_dequeMutex;
};

#endif // #ifndef _FRAMERATECOUNTER_H
