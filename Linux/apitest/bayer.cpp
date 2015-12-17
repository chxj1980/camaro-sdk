#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <memory.h>
#ifdef _WIN32
#include <omp.h>
#endif
////////////////////////////////////////////////////////

int PIX_HEIGHT = 0;
int PIX_WIDTH = 0;

#define R(x,y)  rgb[0+ 3 * ((x) + PIX_WIDTH *(y))]
#define G(x,y)  rgb[1+ 3 * ((x) + PIX_WIDTH *(y))]
#define B(x,y)  rgb[2+ 3 * ((x) + PIX_WIDTH *(y))]

#define GREY(x,y)   grey[(x)+PIX_WIDTH*(y)]

#if 0 //OV 4682
/*
B IR  B  IR
G  R  G  R
B IR  B  IR
G  R  G  R
*/
#define Bay(x,y)  ((unsigned short)(raw_16[(x) + PIX_WIDTH *(y)] >> 2))
void bayer_copy(unsigned short * raw_16, unsigned char *rgb, int x, int y)
{

    //G(x+1,y) = Bay(x+1,y) ;
    //G(x,y+1) = Bay(x,y+1);
    //G(x,y) = G(x+1,y+1) = (Bay(x+1,y) + Bay(x,y+1)) / 2;
    G(x,y)  = G(x,y+1) = G(x+1,y)  = G(x+1,y+1) = Bay(x,y+1);
    B(x,y)=B(x+1,y)=B(x,y+1)=B(x+1,y+1) = Bay(x,y);
    R(x,y)=R(x+1,y)=R(x,y+1)=R(x+1,y+1) = Bay(x+1,y+1);
}

void bayer_bilinear(unsigned short * raw_16, unsigned char *rgb,int x, int y)
{
    R(x,y) = (Bay(x-1,y-1) + Bay(x+1, y+1) + Bay(x+1,y-1) + Bay(x-1,y+1)) / 4;
    //G(x,y) = (Bay(x-1,y) + Bay(x+1,y) + Bay(x,y-1) + Bay(x,y+1)) / 4;
    G(x,y) = ( Bay(x,y-1) + Bay(x,y+1)) / 2;
    B(x,y) = Bay(x,y);

    R(x+1,y) = (Bay(x+1,y-1) + Bay(x+1, y+1)) / 2;
    //G(x+1,y) = Bay(x+1,y);
    G(x+1,y) = (Bay(x,y-1) + Bay(x+2, y+1) + Bay(x+2,y-1) + Bay(x,y+1)) / 4;
    B(x+1,y) = (Bay(x,y) + Bay(x+2,y)) / 2 ;

    R(x,y+1) = (Bay(x-1,y+1) + Bay(x+1, y+1)) /2;
    G(x,y+1) = Bay(x,y+1);
    B(x,y+1) = (Bay(x,y) + Bay(x, y+2)) / 2;

    R(x+1,y+1) = Bay(x+1,y+1);
    //G(x+1,y+1) = (Bay(x+1,y) + Bay(x,y+1) + Bay(x+2,y+1) + Bay(x+1,y+2)) / 4;
    G(x+1,y+1) = (Bay(x,y+1) + Bay(x+2,y+1)) / 2;
    B(x+1,y+1) = (Bay(x,y) + Bay(x+2, y+2) + Bay(x+2,y) + Bay(x,y+2)) / 4;

}

#else
//AR 0134
#define Bay(x,y)  ((unsigned short)(raw_16[(x) + PIX_WIDTH *(y)] >> 4) & 0x00FF)

/*
G  R  G R
B  G  B G
G  R  G R
B  G  B G
*/

// for grey
void grey_copy(unsigned short * raw_16, unsigned char *grey, int x, int y)
{
    GREY(x,y) = Bay(x,y);
}

void bayer_copy(unsigned short * raw_16, unsigned char *rgb, int x, int y)
{

    G(x,y) = Bay(x,y) ;
    G(x+1,y+1) = Bay(x+1,y+1);
    G(x+1,y) = G(x,y+1) = (Bay(x,y) + Bay(x+1,y+1)) / 2;

    B(x,y)=B(x+1,y)=B(x,y+1)=B(x+1,y+1) = Bay(x,y+1);
    R(x,y)=R(x+1,y)=R(x,y+1)=R(x+1,y+1) = Bay(x+1,y);
}

void bayer_bilinear(unsigned short * raw_16, unsigned char *rgb,int x, int y)
{
    B(x,y) = (Bay(x,y-1) + Bay(x, y+1)) / 2;
    G(x,y) = Bay(x,y);
    R(x,y) = (Bay(x-1,y) + Bay(x+1, y)) / 2;

    B(x+1,y) = (Bay(x,y-1) + Bay(x+2, y+1) + Bay(x+2,y-1) + Bay(x,y+1)) / 4;
    G(x+1,y) = (Bay(x,y) + Bay(x+2, y) + Bay(x,y) + Bay(x+2,y)) / 4;
    R(x+1,y) = Bay(x+1,y);

    B(x,y+1) = Bay(x,y+1);
    G(x,y+1) = (Bay(x,y) + Bay(x+1, y+1) + Bay(x-1,y+1) + Bay(x,y+2)) / 4;
    R(x,y+1) = (Bay(x-1,y) + Bay(x+1,y) + Bay(x-1,y+2) + Bay(x+1,y+2)) / 4;

    B(x+1,y+1) = (Bay(x,y+1) + Bay(x+2,y+1)) / 2;
    G(x+1,y+1) = Bay(x+1,y+1);
    R(x+1,y+1) = (Bay(x+1,y) + Bay(x+1, y+2) ) / 2;
}


// the following bayer_xxx functions are for "CFA Demosaicing with "directional weighted gradient based interpolation""
//
void bayer_copy_G(unsigned short * raw_16, unsigned char *rgb, int x, int y)
{
    G(x,y) = Bay(x,y) ;
}

void bayer_copy_B(unsigned short * raw_16, unsigned char *rgb, int x, int y)
{
    B(x,y) = Bay(x,y) ;
}

void bayer_copy_R(unsigned short * raw_16, unsigned char *rgb, int x, int y)
{
    R(x,y) = Bay(x,y) ;
}

void bayer_inter_G_at_BR(unsigned short * raw_16, unsigned char *rgb,int x, int y)
{
    float Wn, We, Ws, Ww, Gn, Ge, Gs, Gw, temp;

    Wn = 1.0 / (1 + abs(Bay(x,y+1) - Bay(x,y-1)) + abs(Bay(x,y) - Bay(x,y-2)));
    We = 1.0 / (1 + abs(Bay(x-1,y) - Bay(x+1,y)) + abs(Bay(x,y) - Bay(x+2,y)));
    Ws = 1.0 / (1 + abs(Bay(x,y-1) - Bay(x,y+1)) + abs(Bay(x,y) - Bay(x,y+2)));
    Ww = 1.0 / (1 + abs(Bay(x+1,y) - Bay(x-1,y)) + abs(Bay(x,y) - Bay(x-2,y)));

    Gn = Bay(x,y-1) + 1.0 * (Bay(x,y) - Bay(x,y-2)) / 2;
    Ge = Bay(x+1,y) + 1.0 * (Bay(x,y) - Bay(x+2,y)) / 2;
    Gs = Bay(x,y+1) + 1.0 * (Bay(x,y) - Bay(x,y+2)) / 2;
    Gw = Bay(x-1,y) + 1.0 * (Bay(x,y) - Bay(x-2,y)) / 2;

    temp = (Wn * Gn + We * Ge + Ws * Gs + Ww * Gw) / (Wn + We + Ws + Ww);
    if (temp >= 254.5)
        G(x,y) = 255;
    else if (temp <= 0.49)
        G(x,y) = 0;
    else
        G(x,y) = temp + 0.5;
}

void bayer_inter_B_at_R(unsigned short * raw_16, unsigned char *rgb,int x, int y)
{
    float Wne, Wse, Wsw, Wnw, Bne, Bse, Bsw, Bnw, temp;

    Wne = 1.0 / (1 + abs(Bay(x-1,y+1) - Bay(x+1,y-1)) + abs(G(x,y) - G(x+1,y-1)));
    Wse = 1.0 / (1 + abs(Bay(x-1,y-1) - Bay(x+1,y+1)) + abs(G(x,y) - G(x+1,y+1)));
    Wsw = 1.0 / (1 + abs(Bay(x+1,y-1) - Bay(x-1,y+1)) + abs(G(x,y) - G(x-1,y+1)));
    Wnw = 1.0 / (1 + abs(Bay(x+1,y+1) - Bay(x-1,y-1)) + abs(G(x,y) - G(x-1,y-1)));

    Bne = Bay(x+1,y-1) + 1.0 * (G(x,y) - G(x+1,y-1)) / 2;
    Bse = Bay(x+1,y+1) + 1.0 * (G(x,y) - G(x+1,y+1)) / 2;
    Bsw = Bay(x-1,y+1) + 1.0 * (G(x,y) - G(x-1,y+1)) / 2;
    Bnw = Bay(x-1,y-1) + 1.0 * (G(x,y) - G(x-1,y-1)) / 2;

    temp = (Wne * Bne + Wse * Bse + Wsw * Bsw + Wnw * Bnw) / (Wne + Wse + Wsw + Wnw);

    if (temp >= 254.5)
        B(x,y) = 255;
    else if (temp <= 0.49)
        B(x,y) = 0;
    else
        B(x,y) = temp + 0.5;
}

void bayer_inter_R_at_B(unsigned short * raw_16, unsigned char *rgb,int x, int y)
{
    float Wne, Wse, Wsw, Wnw, Rne, Rse, Rsw, Rnw, temp;

    Wne = 1.0 / (1 + abs(Bay(x-1,y+1) - Bay(x+1,y-1)) + abs(G(x,y) - G(x+1,y-1)));
    Wse = 1.0 / (1 + abs(Bay(x-1,y-1) - Bay(x+1,y+1)) + abs(G(x,y) - G(x+1,y+1)));
    Wsw = 1.0 / (1 + abs(Bay(x+1,y-1) - Bay(x-1,y+1)) + abs(G(x,y) - G(x-1,y+1)));
    Wnw = 1.0 / (1 + abs(Bay(x+1,y+1) - Bay(x-1,y-1)) + abs(G(x,y) - G(x-1,y-1)));

    Rne = Bay(x+1,y-1) + (G(x,y) - G(x+1,y-1)) / 2;
    Rse = Bay(x+1,y+1) + (G(x,y) - G(x+1,y+1)) / 2;
    Rsw = Bay(x-1,y+1) + (G(x,y) - G(x-1,y+1)) / 2;
    Rnw = Bay(x-1,y-1) + (G(x,y) - G(x-1,y-1)) / 2;

    temp = (Wne * Rne + Wse * Rse + Wsw * Rsw + Wnw * Rnw) / (Wne + Wse + Wsw + Wnw);

    if (temp >= 254.5)
        R(x,y) = 255;
    else if (temp <= 0.49)
        R(x,y) = 0;
    else
        R(x,y) = temp + 0.5;
}

void bayer_inter_B_at_G(unsigned short * raw_16, unsigned char *rgb,int x, int y)
{
    float Wn, We, Ws, Ww, Bn, Be, Bs, Bw, temp;

    Wn = 1.0 / (1 + abs(B(x,y+1) - B(x,y-1)) + abs(G(x,y) - G(x,y-2)));
    We = 1.0 / (1 + abs(B(x-1,y) - B(x+1,y)) + abs(G(x,y) - G(x+2,y)));
    Ws = 1.0 / (1 + abs(B(x,y-1) - B(x,y+1)) + abs(G(x,y) - G(x,y+2)));
    Ww = 1.0 / (1 + abs(B(x+1,y) - B(x-1,y)) + abs(G(x,y) - G(x-2,y)));

    Bn = B(x,y-1) + (G(x,y) - G(x,y-2)) / 2;
    Be = B(x+1,y) + (G(x,y) - G(x+2,y)) / 2;
    Bs = B(x,y+1) + (G(x,y) - G(x,y+2)) / 2;
    Bw = B(x-1,y) + (G(x,y) - G(x-2,y)) / 2;

    temp = (Wn * Bn + We * Be + Ws * Bs + Ww * Bw) / (Wn + We + Ws + Ww);

    if (temp >= 254.5)
        B(x,y) = 255;
    else if (temp <= 0.49)
        B(x,y) = 0;
    else
        B(x,y) = temp + 0.5;
}

void bayer_inter_R_at_G(unsigned short * raw_16, unsigned char *rgb,int x, int y)
{
    float Wn, We, Ws, Ww, Rn, Re, Rs, Rw, temp;

    Wn = 1.0 / (1 + abs(R(x,y+1) - R(x,y-1)) + abs(G(x,y) - G(x,y-2)));
    We = 1.0 / (1 + abs(R(x-1,y) - R(x+1,y)) + abs(G(x,y) - G(x+2,y)));
    Ws = 1.0 / (1 + abs(R(x,y-1) - R(x,y+1)) + abs(G(x,y) - G(x,y+2)));
    Ww = 1.0 / (1 + abs(R(x+1,y) - R(x-1,y)) + abs(G(x,y) - G(x-2,y)));

    Rn = R(x,y-1) + (G(x,y) - G(x,y-2)) / 2;
    Re = R(x+1,y) + (G(x,y) - G(x+2,y)) / 2;
    Rs = R(x,y+1) + (G(x,y) - G(x,y+2)) / 2;
    Rw = R(x-1,y) + (G(x,y) - G(x-2,y)) / 2;

    temp = (Wn * Rn + We * Re + Ws * Rs + Ww * Rw) / (Wn + We + Ws + Ww);

    if (temp >= 254.5)
        R(x,y) = 255;
    else if (temp <= 0.49)
        R(x,y) = 0;
    else
        R(x,y) = temp + 0.5;
}
#endif


#include <omp.h>

int convert_raw_to_grey_buffer(unsigned char *raw,
                               unsigned char *grey,
                               int w,int h)
{
    PIX_WIDTH = w;
    PIX_HEIGHT = h;
    int i, j;
    for (j = 0; j < PIX_HEIGHT; j++)
    {
        for (i = 0; i < PIX_WIDTH; i++)
        {
            grey_copy((unsigned short *)raw,grey,i,j);
        }
    }
    return 0;
}

int convert_raw_to_rgb_buffer_with_lumi_calc(unsigned char *raw, unsigned char *rgb, int *lumi, int lumi_x, int lumi_y, int lumi_width, int lumi_height)
{
    int i,j;
    int u, v;
    //unsigned short *raw_16 = (unsigned short *)raw;
    *lumi = 0;
    //int sum_lumi = 0;
    //#pragma omp parallel for shared(raw,rgb) private(i,j)
	auto x = (lumi_x % 2) ? (lumi_x + 1) : lumi_x;
	auto y = (lumi_y % 2) ? (lumi_y + 1) : lumi_y;

	auto width = (lumi_width % 2) ? (lumi_width - 1) : lumi_width;
	auto height = (lumi_height % 2) ? (lumi_height - 1) : lumi_height;
    for (j=0;j < PIX_HEIGHT; j+=2)
    {
        for (i=0;i < PIX_WIDTH; i+=2)
        {

            if (i==0||j==0||i== PIX_WIDTH -2|| j== PIX_HEIGHT-2)
            {
                bayer_copy(reinterpret_cast<unsigned short *>(raw),rgb, i,j);

            }
            else
            {
                bayer_bilinear(reinterpret_cast<unsigned short *>(raw),rgb, i,j);
                // bayer_copy((unsigned short *)raw,rgb, i,j);
            }

            if (i >= x && i < x + width && j >= y && j < y + height)
            {
                for (u = i; u <= i+1; u++)
                    for (v = j; v <= j+1; v++)
                    {
                        *lumi += (R(u,v) * 299 + G(u,v) * 587 + B(u,v) * 114 + 500) / 1000 - 128;
                    }
            }


        }
    }

    *lumi /= (width * height);
    *lumi += 128;
    return 0;
}

int convert_raw_to_rgb_buffer_with_histogram_calc(unsigned char *raw,
                                                  unsigned char *rgb,
                                                  unsigned int *histogram,
                                                  int histogram_buckets,
                                                  int w,int h)
{
    PIX_WIDTH = w;
    PIX_HEIGHT = h;
        int i,j;
        int u, v;
        unsigned char lumi;
        unsigned int index;
    //unsigned short *raw_16 = (unsigned short *)raw;
    memset((void*)histogram, 0, histogram_buckets * sizeof(unsigned int));
    //#pragma omp parallel for shared(raw,rgb) private(i,j, u, v)
    for (j=0;j < PIX_HEIGHT; j+=2)
    {
        for (i=0;i < PIX_WIDTH; i+=2)
        {

            if (i==0||j==0||i== PIX_WIDTH -2|| j== PIX_HEIGHT-2)
            {
                bayer_copy((unsigned short *)raw,rgb, i,j);

            }
            else
            {
                bayer_bilinear((unsigned short *)raw,rgb, i,j);
                // bayer_copy((unsigned short *)raw,rgb, i,j);
            }

#if 1
            // calculate histogram
            for (u = i; u <= i+1; u++)
                for (v = j; v <= j+1; v++)
                {
                    lumi = (R(u,v) * 299 + G(u,v) * 587 + B(u,v) * 114 + 500) / 1000;
                    index = lumi * histogram_buckets / 256;
                    histogram[index]++;
                }
#endif

        }
    }
    return 0;
}

int convert_raw_to_rgb_buffer(unsigned char *raw, unsigned char *rgb, bool isGradientBasedInter, int w, int h)
{
    int i,j;
    PIX_WIDTH = w;
    PIX_HEIGHT = h;
#if 1

    if (!isGradientBasedInter)
    {
        #pragma omp parallel for shared(raw,rgb) private(i,j)
        for (j=0;j < PIX_HEIGHT; j+=2)
         {
            for (i=0;i < PIX_WIDTH; i+=2)
             {

                 if (i==0||j==0||i== PIX_WIDTH -2|| j== PIX_HEIGHT-2)
                 {
                    bayer_copy((unsigned short *)raw,rgb, i,j);

                 }
                 else
                 {
                    bayer_bilinear((unsigned short *)raw,rgb, i,j);
                    // bayer_copy((unsigned short *)raw,rgb, i,j);
                 }

             }
         }
    }
    else
    {
        //0. fill the 4 sides first
        #pragma omp parallel for shared(raw,rgb) private(i,j)
        for (j=0;j < PIX_HEIGHT; j+=2)
        {
            for (i=0;i < PIX_WIDTH; i+=2)
             {

                 if (i==0||j==0||i== PIX_WIDTH -2|| j== PIX_HEIGHT-2)
                 {
                    bayer_copy((unsigned short *)raw,rgb, i,j);
                 }

            }
         }

         //1. fill G at G && interpolate G at B&R
        #pragma omp parallel for shared(raw,rgb) private(i,j)
         for (j = 2; j < PIX_HEIGHT-2; j++)
         {
             for (i = 2; i < PIX_WIDTH-2; i++)
             {
                if ((i + j) % 2 == 0)
                    bayer_copy_G((unsigned short *)raw, rgb, i, j);
                else
                    bayer_inter_G_at_BR((unsigned short *)raw, rgb, i, j);

             }
         }

         //2. interpolate B/R at R/B
         #pragma omp parallel for shared(raw,rgb) private(i,j)
         for (j = 2; j < PIX_HEIGHT-2; j++)
         {
             for (i = 2; i < PIX_WIDTH-2; i++)
             {
                if (i % 2 == 1 && j % 2 == 0)       // at R
                {
                    bayer_copy_R((unsigned short *)raw, rgb, i, j);
                    bayer_inter_B_at_R((unsigned short *)raw, rgb, i, j);

                }
                else if (i % 2 == 0 && j % 2 == 1)   // at B
                {
                    bayer_copy_B((unsigned short *)raw, rgb, i, j);
                    bayer_inter_R_at_B((unsigned short *)raw, rgb, i, j);
                }
             }
         }

         //3. interpolate B R at G
         #pragma omp parallel for shared(raw,rgb) private(i,j)
         for (j = 2; j < PIX_HEIGHT-2; j++)
         {
             for (i = 2; i < PIX_WIDTH-2; i++)
             {
                 if ((i + j) % 2 == 0) //at G
                 {
                     bayer_inter_B_at_G((unsigned short *)raw, rgb, i, j);
                     bayer_inter_R_at_G((unsigned short *)raw, rgb, i, j);
                 }
             }
         }
    }

#else
    //to show IR only
    unsigned short * raw_16 = (unsigned short *)raw;

    for (i=0;i < PIX_WIDTH; i+=2)
    {
        for (j=0;j < PIX_HEIGHT; j+=2)
        {
            unsigned char ir = Bay(i,j+1);
            unsigned char r = Bay(i+1,j+1);
            //unsigned char g = Bay(i+1,j+1);
            unsigned char b = Bay(i,j);
            R(i,j) = r;
            G(i,j) = ir;
            B(i,j) = b;

            R(i,j+1) = r;
            G(i,j+1) = ir;
            B(i,j+1) = b;

            R(i+1,j) = r;
            G(i+1,j) = ir;
            B(i+1,j) = b;

            R(i+1,j+1) = r;
            G(i+1,j+1) = ir;
            B(i+1,j+1) = b;
        }
    }

#endif
     return 0;
}

// addition for lens focusing calibration
// X_START, Y_START, WIDTH, HEIGHT:
//      specify the area for gradient calculation
// fThreshold:
//      the threshold for excluding the little values
// magnitude:
//      magnitude gradient value of the area
//int convert_raw_to_rgb_buffer_with_gradient_calc(unsigned char *raw, unsigned char *rgb, bool isGradientBasedInter, int M_START_X, int M_START_Y, int M_WIDTH, int M_HEIGHT, float M_Threshold, float &magnitude)
//{
//    int i,j;
//
//    unsigned short *raw_16 = (unsigned short *)raw;
//    float M[PIX_WIDTH][PIX_HEIGHT];
//
//    if (!isGradientBasedInter)
//    {
//        #pragma omp parallel for shared(raw,rgb) private(i,j)
//        for (j=0;j < PIX_HEIGHT; j+=2)
//         {
//            for (i=0;i < PIX_WIDTH; i+=2)
//             {
//
//                 if (i==0||j==0||i== PIX_WIDTH -2|| j== PIX_HEIGHT-2)
//                 {
//                    bayer_copy((unsigned short *)raw,rgb, i,j);
//
//                 }
//                 else
//                 {
//                    bayer_bilinear((unsigned short *)raw,rgb, i,j);
//                    // bayer_copy((unsigned short *)raw,rgb, i,j);
//                 }
//
//                 if (i >=M_START_X && i < M_START_X + M_WIDTH && j >= M_START_Y && j < M_START_Y + M_HEIGHT)
//                 {
//                    int m, n, u, v;
//                    for(m = 0; m <= 1; m++)
//                        for (n = 0; n <= 1; n++)
//                        {
//                            u = i + m; v = j + n;
//                            M[u-M_START_X][v-M_START_Y] = sqrt( (Bay(u+2,v) - Bay(u,v)) * (Bay(u+2,v)-Bay(u,v)) + (Bay(u,v+2) - Bay(u,v)) * (Bay(u,v+2)-Bay(u,v)) );
//                        }
//                 }
//
//             }
//         }
//
//        magnitude = 0;
//        for (i = 0; i < M_WIDTH;i++)
//            for (j = 0; j < M_HEIGHT; j++)
//            {
//                if (M[i][j] > M_Threshold)
//                    magnitude += M[i][j] / 1000;
//            }
//    }
//    else
//    {
//        //0. fill the 4 sides first
//        #pragma omp parallel for shared(raw,rgb) private(i,j)
//        for (j=0;j < PIX_HEIGHT; j+=2)
//        {
//            for (i=0;i < PIX_WIDTH; i+=2)
//             {
//
//                 if (i==0||j==0||i== PIX_WIDTH -2|| j== PIX_HEIGHT-2)
//                 {
//                    bayer_copy((unsigned short *)raw,rgb, i,j);
//                 }
//
//                 if (i >=M_START_X && i < M_START_X + M_WIDTH && j >= M_START_Y && j < M_START_Y + M_HEIGHT)
//                 {
//                    int m, n, u, v;
//                    for(m = 0; m <= 1; m++)
//                        for (n = 0; n <= 1; n++)
//                        {
//                            u = i + m; v = j + n;
//                            M[u-M_START_X][v-M_START_Y] = sqrt( (Bay(u+2,v) - Bay(u,v)) * (Bay(u+2,v)-Bay(u,v)) + (Bay(u,v+2) - Bay(u,v)) * (Bay(u,v+2)-Bay(u,v)) );
//                        }
//                 }
//
//            }
//         }
//
//        magnitude = 0;
//        for (i = 0; i < M_WIDTH;i++)
//            for (j = 0; j < M_HEIGHT; j++)
//            {
//                if (M[i][j] > M_Threshold)
//                    magnitude += M[i][j] / 1000;
//            }
//
//         //1. fill G at G && interpolate G at B&R
//        #pragma omp parallel for shared(raw,rgb) private(i,j)
//         for (j = 2; j < PIX_HEIGHT-2; j++)
//         {
//             for (i = 2; i < PIX_WIDTH-2; i++)
//             {
//                if ((i + j) % 2 == 0)
//                    bayer_copy_G((unsigned short *)raw, rgb, i, j);
//                else
//                    bayer_inter_G_at_BR((unsigned short *)raw, rgb, i, j);
//
//             }
//         }
//
//         //2. interpolate B/R at R/B
//         #pragma omp parallel for shared(raw,rgb) private(i,j)
//         for (j = 2; j < PIX_HEIGHT-2; j++)
//         {
//             for (i = 2; i < PIX_WIDTH-2; i++)
//             {
//                if (i % 2 == 1 && j % 2 == 0)       // at R
//                {
//                    bayer_copy_R((unsigned short *)raw, rgb, i, j);
//                    bayer_inter_B_at_R((unsigned short *)raw, rgb, i, j);
//
//                }
//                else if (i % 2 == 0 && j % 2 == 1)   // at B
//                {
//                    bayer_copy_B((unsigned short *)raw, rgb, i, j);
//                    bayer_inter_R_at_B((unsigned short *)raw, rgb, i, j);
//                }
//             }
//         }
//
//         //3. interpolate B R at G
//         #pragma omp parallel for shared(raw,rgb) private(i,j)
//         for (j = 2; j < PIX_HEIGHT-2; j++)
//         {
//             for (i = 2; i < PIX_WIDTH-2; i++)
//             {
//                 if ((i + j) % 2 == 0) //at G
//                 {
//                     bayer_inter_B_at_G((unsigned short *)raw, rgb, i, j);
//                     bayer_inter_R_at_G((unsigned short *)raw, rgb, i, j);
//                 }
//             }
//         }
//    }
//
//     return 0;
//}
