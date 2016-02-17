/*******************************************************************************
Vendor: Xilinx 
Associated Filename: opencl_sw_maxscore_systolic.cpp
Purpose: SWAN-HLS Sample - Smithwaterman Algorithm HLS kernel / systolic version
Revision History: February 12, 2016 - initial release
                                                
*******************************************************************************
Copyright (C) 2016 XILINX, Inc.

This file contains confidential and proprietary information of Xilinx, Inc. and 
is protected under U.S. and international copyright and other intellectual 
property laws.

DISCLAIMER
This disclaimer is not a license and does not grant any rights to the materials 
distributed herewith. Except as otherwise provided in a valid license issued to 
you by Xilinx, and to the maximum extent permitted by applicable law: 
(1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, AND XILINX 
HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, 
INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, OR 
FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether 
in contract or tort, including negligence, or under any other theory of 
liability) for any loss or damage of any kind or nature related to, arising under 
or in connection with these materials, including for any direct, or any indirect, 
special, incidental, or consequential loss or damage (including loss of data, 
profits, goodwill, or any type of loss or damage suffered as a result of any 
action brought by a third party) even if such damage or loss was reasonably 
foreseeable or Xilinx had been advised of the possibility of the same.

CRITICAL APPLICATIONS
Xilinx products are not designed or intended to be fail-safe, or for use in any 
application requiring fail-safe performance, such as life-support or safety 
devices or systems, Class III medical devices, nuclear facilities, applications 
related to the deployment of airbags, or any other applications that could lead 
to death, personal injury, or severe property or environmental damage 
(individually and collectively, "Critical Applications"). Customer assumes the 
sole risk and liability of any use of Xilinx products in Critical Applications, 
subject only to applicable laws and regulations governing limitations on product 
liability. 

THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT 
ALL TIMES.

*******************************************************************************/




/*
 * Systolic implementation of smith-waterman
 */

#include <stdio.h>
#include <string.h>
#include "sw.h"
#include <ap_int.h>
#include "assert.h"

typedef ap_uint<2> uint2_t;
typedef ap_uint<1> uint1_t;

typedef struct _pe{
    short d;
    short p;
}pe;

void initPE(pe *pex){
#pragma HLS PIPELINE
	for(int i = 0; i < MAXPE; i++){
		pex[i].d = 0;
		pex[i].p = 0;
	}
}

#ifdef _COMPUTE_FULL_MATRIX
short **localMat;
static short  colIter = 0;
#endif


void updatePE(pe *pex, uint2_t d, uint2_t q, short n, short nw, short w, short r, short c){
#pragma HLS PIPELINE
    short max = 0;
    short match = (d == q) ? MATCH : MISS_MATCH;
    short x1 = nw+match;
    short t1 = (x1 > max) ? x1 : max;
    short x2 = w+GAP;
    short t2 = (x2 > t1 ) ? x2 : t1;
    short x3 = n+GAP;
    max = (x3 > t2) ? x3 : t2;
    pex->p = max;
    pex->d = n;
#ifdef _COMPUTE_FULL_MATRIX
    localMat[r][colIter*MAXPE + c] = max;
#endif

}


void executePE(short r,short c,pe *pex, pe*ppex, uint2_t *d, uint2_t *q){
#pragma HLS PIPELINE
    short nw, w, n;

    if (r == 0){
        n = 0;
        nw = 0;
    }else{
        n = pex->p;
        nw = ppex->d;
    }
    w = ppex->p;
    uint2_t d1 = d[c];
    uint2_t q1 = q[r];
    updatePE(pex, d1, q1, n, nw, w, r, c);
}

void executeFirstPE(short r,short c,pe *p, uint2_t *d, uint2_t *q, short nw, short w){
#pragma HLS PIPELINE
    short  n;
    if (r == 0){
        n = 0;
    }else{
        n = p->p;

    }
    uint2_t d1 = d[c];
    uint2_t q1 = q[r];
    updatePE(p, d1, q1, n, nw, w, r, c);
}

template <int FACTOR>
void swCoreB(uint2_t *d, uint2_t *q, short *maxr, short *maxc, short *maxv, short *iterB, pe *myPE, short stripe, short rows){
#pragma HLS inline
#pragma HLS array partition variable=d cyclic factor=FACTOR
	int i, loop;
    short w = 0; // Initial condition at the start of a row
    d+= stripe*MAXPE;
	initPE(myPE);
    for(loop = 0; loop < rows; ++loop){
#pragma HLS PIPELINE
        short rowmaxv = MINVAL;
        short rowmaxpe = 0;
        for(i = 0; i < MAXPE; i++){
#pragma HLS inline region recursive
            if(i == 0){
                short nw = w;
                w = (stripe == 0) ? 0 : iterB[loop];
                executeFirstPE(loop,i,&myPE[i], d, q, nw, w);
            }else{
                executePE(loop,i,&myPE[i], &myPE[i-1], d, q);
            }
			if(i == MAXPE-1){
                iterB[loop] = myPE[i].p;
			}
            if (myPE[i].p > rowmaxv){
                rowmaxv = myPE[i].p;
                rowmaxpe = i;
            }
        }

        if (rowmaxv > *maxv){
            *maxv = rowmaxv;
            *maxc = rowmaxpe + stripe*MAXPE; // log2(MAXPE);
            *maxr = loop;
        }
    }
}

/*Only columns*/
void swSystolicBlocking(uint2_t d[MAXCOL], uint2_t q[MAXROW], short *maxr, short *maxc, short *maxv, short rows, short cols){
pe  myPE[MAXPE];
short iterB[MAXROW];
#pragma HLS inline 
#pragma HLS RESOURCE variable=iterB core=RAM_S2P_LUTRAM
#pragma HLS RESOURCE variable=q core=RAM_S2P_LUTRAM
	*maxc = MINVAL;
	*maxv = MINVAL;
	*maxr = MINVAL;
    short stripes = MAXCOL / MAXPE;
    assert(stripes <= (MAXCOL+MAXPE-1)/MAXPE);
    assert(rows <= MAXROW);
#pragma HLS array partition variable=myPE
	for(short stripe = 0; stripe < stripes; stripe = stripe + 1){
#ifdef _COMPUTE_FULL_MATRIX
		colIter = stripe;
#endif
        swCoreB<MAXPE>(d, q, maxr, maxc, maxv, iterB, myPE, stripe, rows);
	}

}


void simpleSW(uint2_t refSeq[MAXCOL], uint2_t readSeq[MAXROW], short *maxr, short *maxc, short *maxv){
#pragma HLS inline region off
	*maxv = MINVAL;
    int row, col;
    short mat[MAXROW][MAXCOL];
	for(col = 0; col < MAXCOL; col++){
		short d = refSeq[col];
		for(row = 0; row < MAXROW; ++row){
			short n, nw, w;
			 if (row == 0){
				 n = 0;
			 }else{
			     n = mat[row-1][col];
			 }
			 if(col == 0){
				 w = 0;
			 }else{
				 w = mat[row][col-1];
			 }

			 if(row > 0 && col > 0){
				 nw = mat[row-1][col-1];
			 }else{
				 nw = 0;
			 }

			 short q = readSeq[row];
			 short max = 0;
			 short match = (d == q) ? MATCH : MISS_MATCH;
			 short t1 = (nw + match > max) ? nw + match : max;
			 short t2 = (n + GAP > w + GAP) ? n + GAP : w + GAP;
			 max = t1 > t2 ? t1 : t2;
			 mat[row][col] = max;
			 if(max > *maxv){
				 *maxv = max;
				 *maxr = row;
				 *maxc = col;
			 }
		}
	}


}

void sw(uint2_t d[MAXCOL], uint2_t q[MAXROW], short *maxr, short *maxc, short *maxv){
#pragma HLS inline region off
	swSystolicBlocking(d, q, maxr, maxc, maxv, MAXROW, MAXCOL);
}

template <int BUFFERSZ>
void intTo2bit(unsigned int *buffer, uint2_t *buffer2b){
 int i, j;
#pragma HLS PIPELINE
 for(i = 0; i < BUFFERSZ; ++i){
        for(j = 0; j < 16; ++j){
            buffer2b[16*i+j] = (buffer[i] & (3 << (2*j)))>>(2*j);
        }
    }
}

template <int FACTOR>
void swInt(unsigned int *readRefPacked, short *maxr, short *maxc, short *maxv){
#pragma HLS function_instantiate variable=maxv
    uint2_t d2bit[MAXCOL];
    uint2_t q2bit[MAXROW];
#pragma HLS array partition variable=d2bit,q2bit cyclic factor=FACTOR

    intTo2bit<MAXCOL/16>((readRefPacked + MAXROW/16), d2bit);
    intTo2bit<MAXROW/16>(readRefPacked, q2bit);
    sw(d2bit, q2bit, maxr, maxc, maxv);
}

void swMaxScore(unsigned int readRefPacked[NUMPACKED][PACKEDSZ], short out[NUMPACKED][3]){
	/*instantiate NUMPACKED PE*/
	for(int i = 0; i < NUMPACKED;++i){
	#pragma HLS UNROLL
		swInt<MAXPE>(readRefPacked[i], &out[i][0], &out[i][1], &out[i][2]);
	}
}
//#ifndef HLS_COMPILE
extern "C" {
//#endif
    void opencl_sw_maxscore(unsigned int *input, unsigned int  *output, int *size) {
#pragma HLS inline region off
#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem 
#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem 
#pragma HLS INTERFACE m_axi port=size offset=slave bundle=gmem 
#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE s_axilite port=size bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control
        unsigned int inbuf[PACKEDSZ*NUMPACKED];
        unsigned int outbuf[3*NUMPACKED];
        unsigned int readRefPacked[NUMPACKED][PACKEDSZ];
        short out[NUMPACKED][3];
        int numIter;
#pragma HLS array partition variable=readRefPacked  dim=1
#pragma HLS array partition variable=out dim=0
        numIter = *size;
        int loop = 0;
        for(loop = 0; loop < numIter; loop++){
            /*read from device memory to BRAM*/
            memcpy(readRefPacked, 
                (unsigned int *)(input + loop*PACKEDSZ*NUMPACKED),
                UINTSZ*PACKEDSZ*NUMPACKED);
            swMaxScore(readRefPacked, out);
            /*PE OUT to outbuf*/
            for(int i = 0; i < NUMPACKED; ++i){
#pragma HLS PIPELINE
                outbuf[3*i] = out[i][0];
                outbuf[3*i+1] = out[i][1];
                outbuf[3*i+2] = out[i][2];
            }
            /*outbuf to device memory*/
            memcpy((unsigned int *)(output + 3*NUMPACKED*loop), 
                outbuf, sizeof(unsigned int)*3*NUMPACKED);
        }
        return;
    }
}
