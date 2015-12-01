#ifndef MSGQ_H
#define MSGQ_H

#include "SDL.h"

#if defined __cplusplus
extern "C" {
#endif



typedef struct MSGQ_S {
	Uint8 * queue;
	Uint32 msg_size;
	int max_msg;
	int write;
	int read;
	int nmsg;
	SDL_sem * signal;
	SDL_mutex * guard;
}MSGQ_T;

MSGQ_T *MSGQ_Create(Uint32 msg_size,Uint32 max_msg	);
void MSGQ_Destroy(	MSGQ_T * handle	);
int MSGQ_Post(	MSGQ_T * hndl,	void * msg	);
int MSGQ_Wait(	MSGQ_T* hndl,	void * msg	);
int MSGQ_Flush(	MSGQ_T *hndl	);
int MSGQ_WaitTimeout(	MSGQ_T* hndl,	void * msg, int to);
int MSGQ_TryWait(	MSGQ_T* hndl,	void * msg);
int MSGQ_Peep(	MSGQ_T* hndl,	void * msg	);


#if defined __cplusplus
}
#endif

#endif
