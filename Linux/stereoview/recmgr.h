#ifndef _RECMGR_H
#define _RECMGR_H


# ifdef __cplusplus
extern "C" {
# endif

int REC_Open(char *filename);
void REC_Close();
int REC_Push(unsigned char *rawleft, unsigned char *rawright, int sizel, int sizer, int i);

# ifdef __cplusplus
}
# endif

#endif
