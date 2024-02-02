#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include<stdint.h>

#define FRAC_BITS 14

uint8_t getBit(int32_t a, uint8_t bitNo) { return (a & ( 1 << bitNo )) >> bitNo; }
int32_t setBit(int32_t a, uint8_t bitNo, uint8_t bitVal) { int32_t mask = 1<<bitNo; return ((a & ~mask) | (bitVal << bitNo));}

int32_t float_2_fxp(float a) { 
    int32_t c;
    if (a > 1.9999) c = (int32_t) 0x00007FFF;            
    else if (a < -1.9999) c = (int32_t) 0xFFFF8000;    
    else {
        int8_t signBit = getBit(a,31);
        if (signBit == 0) c = (int32_t)(round(a * (1 << FRAC_BITS)));  
        else              c = (int32_t)(ceil(a * (1 << FRAC_BITS)));
    }
    return c;
}

unsigned int excise(uint32_t no, uint8_t bitNoStart, uint8_t bitNoStop) {
    unsigned int mask = 0;
    for (unsigned i=bitNoStart; i<=bitNoStop; i++) mask |= 1 << i;
    return (mask & no) >> bitNoStart;
}

/* ----------------------------------------------
* coeffs 
* coefficients generation for unity gain notch filtering 
* @param Fnotch   - notch frequency [MHz]
* @param r        - pole radius
* ---------------------------------------------- */
void coeffs(float Fnotch, float r,unsigned int *values) {
	float c1, c2, c3, c4;
	float c5, c6, c7, c8, c9; 
	float Fsample = 500;
	int16_t c1_hex, c2_hex, c3_hex, c4_hex;
	int16_t c5_hex, c7_hex, c9_hex;
	int32_t c6_hex, c8_hex;
	int32_t cc=0;

	c1 =  (1+r*r)/2;
	c2 = -(1+r*r) * cos(2 * M_PI * Fnotch / Fsample);
	c3 = -c2;
	c4 = -(r*r);
	c5 = c2 * (1-c1);
	c6 = c1 + c3 * c2;
	c7 = c1 * c3;
	c8 = c2 * c2 + c4;
	c9 = c3 * c4;

	c1_hex = float_2_fxp(c1);
	c2_hex = float_2_fxp(c2);
	c3_hex = float_2_fxp(c3);
	c4_hex = float_2_fxp(c4);
	c5_hex = float_2_fxp(c5);
	c7_hex = float_2_fxp(c7);
	c9_hex = float_2_fxp(c9);
	c6_hex = float_2_fxp(c6);
	c8_hex = float_2_fxp(c8);

	cc = setBit(cc,30,getBit(c6_hex,16));
	cc = setBit(cc,31,getBit(c8_hex,16));

/*	printf("Register CH1NF1P1: %4x%4x\n",  excise(c2_hex,0,15),excise(c1_hex,0,15));
	printf("Register CH1NF1P2: %4x%4x\n",  excise(c4_hex,0,15),excise(c3_hex,0,15));
	printf("Register CH1NF1P3: %4x%4x\n",  excise(c7_hex,0,15),excise(c5_hex,0,15));
	printf("Register CH1NF1P4: %4x%4x\n",  excise(c8_hex,0,15),excise(c6_hex,0,15));
	printf("Register CH1NF1P5: %4x%4x\n",  excise(cc,16,31),   excise(c9_hex,0,15));*/
  values[0] = (excise(c2_hex,0,15)<<16)+(excise(c1_hex,0,15)&0xffff);
  values[1] = (excise(c4_hex,0,15)<<16)+(excise(c3_hex,0,15)&0xffff);
  values[2] = (excise(c7_hex,0,15)<<16)+(excise(c5_hex,0,15)&0xffff);
  values[3] = (excise(c8_hex,0,15)<<16)+(excise(c6_hex,0,15)&0xffff);
  values[4] = (excise(cc,16,31)<<16)   +(excise(c9_hex,0,15)&0xffff);
}
/*// ----------------------------------------------
// main
// ----------------------------------------------
int main(int argc, char *argv[]) {	

	coeffs(70, 0.999);

	return 0;

}*/


