//#include "SDL.h"
#include "msgq.h"


/******************************************************************************
 * 
 *
 * 
 *****************************************************************************/
/*-----------------------------------------------------------------------------
 *
 *
 *
 *---------------------------------------------------------------------------*/
MSGQ_T *MSGQ_Create(
	Uint32 msg_size,
	Uint32 max_msg
	)
{
	MSGQ_T * hndl;
	
	hndl = (MSGQ_T *)malloc(sizeof(MSGQ_T));
	if (!hndl) return NULL;	
	memset(hndl, 0, sizeof(MSGQ_T));

	hndl->queue = (Uint8 *)malloc(msg_size * max_msg);
	if (!hndl->queue)
	{
		free(hndl);
		return NULL;
	}
	memset(hndl->queue, 0, msg_size * max_msg);

	hndl->msg_size = msg_size;
	hndl->max_msg = max_msg;
	hndl->write = 0;
	hndl->read = 0;
	hndl->nmsg = 0;
	
	hndl->signal = SDL_CreateSemaphore(0);
	hndl->guard = SDL_CreateMutex();
	
	return hndl;
}


/*-----------------------------------------------------------------------------
 *
 *
 *
 *---------------------------------------------------------------------------*/
void
MSGQ_Destroy(
	MSGQ_T * hndl
	)
{
	if (!hndl) return ;

	SDL_DestroySemaphore(hndl->signal);
	SDL_DestroyMutex(hndl->guard);

	if (hndl->queue) free(hndl->queue);
	free(hndl);

}


/*-----------------------------------------------------------------------------
 *
 *
 *
 *---------------------------------------------------------------------------*/
int
MSGQ_Post(
	MSGQ_T * hndl,
	void * msg
	)
{

	if (!hndl) return -1;
	
	SDL_mutexP(hndl->guard);

	if (hndl->nmsg >= hndl->max_msg)
	{
		SDL_mutexV(hndl->guard);
		return -2;
	}

	memcpy(
		(Uint8 *)&hndl->queue[hndl->msg_size * hndl->write], msg, hndl->msg_size
	);
	hndl->nmsg++;

	if (++hndl->write >= hndl->max_msg)
	{
		hndl->write = 0;
	}

	SDL_mutexV(hndl->guard);
	SDL_SemPost(hndl->signal);

	return 0;
}


/*-----------------------------------------------------------------------------
 *
 *
 *
 *---------------------------------------------------------------------------*/
int
MSGQ_Wait(
	MSGQ_T* hndl,
	void * msg
	)
{
	if (!hndl) return -1;
	
	SDL_SemWait(hndl->signal);
	SDL_mutexP(hndl->guard);

	if (hndl->nmsg <= 0)
	{
		SDL_mutexV(hndl->guard);
		return -2;
	}

	memcpy(
		msg, (Uint8 *)&hndl->queue[hndl->msg_size * hndl->read], hndl->msg_size
	);
	hndl->nmsg--;

	if (++hndl->read >= hndl->max_msg)
	{
		hndl->read = 0;
	}

	SDL_mutexV(hndl->guard);

	return 0;
}


/*-----------------------------------------------------------------------------
 *
 *
 *
 *---------------------------------------------------------------------------*/
int
MSGQ_WaitTimeout(
             MSGQ_T* hndl,
             void * msg,
            int to
             )
{
	if (!hndl) return -1;
	
	if (SDL_MUTEX_TIMEDOUT == SDL_SemWaitTimeout(hndl->signal,to))
	{
		return -3;
	}
    
	SDL_mutexP(hndl->guard);
    
	if (hndl->nmsg <= 0)
	{
		SDL_mutexV(hndl->guard);
		return -2;
	}
    
	memcpy(
           msg, (Uint8 *)&hndl->queue[hndl->msg_size * hndl->read], hndl->msg_size
           );
	hndl->nmsg--;
    
	if (++hndl->read >= hndl->max_msg)
	{
		hndl->read = 0;
	}
    
	SDL_mutexV(hndl->guard);
    
	return 0;
}

/*-----------------------------------------------------------------------------
 *
 *
 *
 *---------------------------------------------------------------------------*/
int
MSGQ_TryWait(
	MSGQ_T* hndl,
	void * msg
	)
{
	if (!hndl) return -1;
	
	if (SDL_MUTEX_TIMEDOUT == SDL_SemTryWait(hndl->signal))
	{
		return -3;
	}

	SDL_mutexP(hndl->guard);

	if (hndl->nmsg <= 0)
	{
		SDL_mutexV(hndl->guard);
		return -2;
	}

	memcpy(
		msg, (Uint8 *)&hndl->queue[hndl->msg_size * hndl->read], hndl->msg_size
	);
	hndl->nmsg--;

	if (++hndl->read >= hndl->max_msg)
	{
		hndl->read = 0;
	}

	SDL_mutexV(hndl->guard);

	return 0;
}
/*-----------------------------------------------------------------------------
 *
 *
 *
 *---------------------------------------------------------------------------*/
int
MSGQ_Flush(
	MSGQ_T *hndl
	)
{
	if (!hndl) return -1;
	
	SDL_mutexP(hndl->guard);
	
	hndl->write = 0;
	hndl->read = 0;
	hndl->nmsg = 0;

	SDL_mutexV(hndl->guard);

	return 0;
}
/*-----------------------------------------------------------------------------
 *
 *
 *
 *---------------------------------------------------------------------------*/
int
MSGQ_Peep(
	MSGQ_T* hndl,
	void * msg
	)
{
	if (!hndl) return -1;
	
	SDL_mutexP(hndl->guard);

	if (hndl->nmsg <= 0)
	{
		SDL_mutexV(hndl->guard);
		return -2;
	}

	memcpy(
		msg, (Uint8 *)&hndl->queue[hndl->msg_size * hndl->read], hndl->msg_size
	);

	//Just peep, so that we should modify nmsg and read pointer
	//hndl->nmsg--;
	//if (++hndl->read >= hndl->max_msg)
	//{
	//	hndl->read = 0;
	//}

	SDL_mutexV(hndl->guard);

	return 0;
}
