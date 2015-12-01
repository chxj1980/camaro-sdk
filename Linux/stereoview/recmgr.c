#include <stdio.h>
#include "msgq.h"
#include "recmgr.h"
#include <time.h>

typedef struct 
{
    unsigned char *dataleft;
    unsigned char *dataright;
    int index;
	int len;
}msg_t;

static MSGQ_T *msgq;
static char gfilename[256] = {0};
static SDL_Thread *hWrThrd = NULL;
static int hWrThrdRunning = 0;
static int frame_count_written = 0;
#define REC_QBUF_LEN        10
#define FRAMECOUNT_PER_FILE 1000
static int write_taskfunc(void *data)
{
	msg_t msg;

    FILE *fp;
    time_t  t_now;
    struct tm *ptm_now;

    data = data;
    //sprintf(gfilename, "/home/xinghua/tmp/RAW/all.raw");

    char *filename_ren_index = strstr(gfilename, ".raw");
    //int filename_no = 0;
    frame_count_written = 0;
    time(&t_now);
    ptm_now = localtime(&t_now);
    sprintf(filename_ren_index, "_%04d%02d%02d_%02d%02d%02d.raw", ptm_now->tm_year+1900, ptm_now->tm_mon, ptm_now->tm_mday, ptm_now->tm_hour, ptm_now->tm_min, ptm_now->tm_sec);
    fp = fopen(gfilename,"wb");
    while (hWrThrdRunning)
	{

		if (MSGQ_TryWait(msgq, &msg) < 0)
		{
            SDL_Delay(10);
			continue;
        }

        if (frame_count_written >= FRAMECOUNT_PER_FILE)
        {
            //filename_no++;
            fclose(fp);
            time(&t_now);
            ptm_now = localtime(&t_now);
            sprintf(filename_ren_index, "_%04d%02d%02d_%02d%02d%02d.raw", ptm_now->tm_year+1900, ptm_now->tm_mon, ptm_now->tm_mday, ptm_now->tm_hour, ptm_now->tm_min, ptm_now->tm_sec);
            fp = fopen(gfilename,"wb");
            frame_count_written = 0;
        }


        fwrite(msg.dataleft,msg.len , 1, fp);
        fwrite(msg.dataright,msg.len , 1, fp);
        frame_count_written++;

        free(msg.dataleft);
        free(msg.dataright);
    }
    fclose(fp);

	return 0;
}


int REC_Open(char *filename)
{
    strcpy(gfilename, filename);
    if (hWrThrdRunning) return -1;
    msgq = MSGQ_Create(sizeof(msg_t), REC_QBUF_LEN);
    hWrThrdRunning = 1;
	hWrThrd= SDL_CreateThread(write_taskfunc, NULL);
	return 0;
}

void REC_Close()
{
	msg_t msg;
    if (!hWrThrdRunning) return;

    if (msgq->nmsg > 0) SDL_Delay(10);//waiting...

	hWrThrdRunning = 0;
	SDL_WaitThread(hWrThrd, NULL);

    while(MSGQ_TryWait(msgq, &msg) ==0)
	{
        if (msg.dataleft)
            free(msg.dataleft);
        if (msg.dataright)
            free(msg.dataright);
    }

    MSGQ_Destroy(msgq);
}

int REC_Push(unsigned char *rawleft, unsigned char *rawright, int size, int index)
{
	msg_t msg;
    msg.dataleft = (unsigned char *)malloc(size);
    msg.dataright = (unsigned char *)malloc(size);
    msg.len = size;
    msg.index = index;

    memcpy(msg.dataleft, rawleft, size);
    memcpy(msg.dataright, rawright, size);

	if(	MSGQ_Post(msgq, &msg) <0)
	{
        //printf("-RECORD drop(%d)-\n", index);
        free(msg.dataleft);
        free(msg.dataright);
        return -1;
	}

	return 0;
}

