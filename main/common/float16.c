#include "float16.h"
#include <stdint.h>

float float16_to_float32(_float16_shape_type f16_val)
{
	//printf("32to16. Input: 0x%x\n", f16_val.bits);

	int32_t sign = (f16_val.bits >> 15) & 0x00000001;
	uint32_t exponent = (f16_val.bits >> 10) & 0x0000001f;
    uint32_t mantissa =  f16_val.bits & 0x000003ff;

	//printf("float16:\n  f16_val.number.sign: %d\n  f16_val.number.exponent %d\n  f16_val.number.mantissa %d\n", sign, exponent, mantissa);

	union { float f; uint32_t i; } v;

	if(exponent == 0)
	{
		if(mantissa == 0) //zero!
		{
			//printf("zero value!\n");
			v.i = sign << 31;
			return v.f;
		}
		else	//sub-normal number, renormalize it (!?)
		{
			//printf("subnormal value!\n");
			//printf("   0x%x, 0x%x\n", mantissa, exponent);
			while (!(mantissa & 0x00000400))
			{
				mantissa <<= 1;
				exponent -=  1;
				//printf("   0x%x, 0x%x\n", mantissa, exponent);
			}
			exponent += 1;
			mantissa &= ~0x00000400;
			//printf("   0x%x, 0x%x\n", mantissa, exponent);

		}
	}

	else if(exponent == 0x1F) //NaN! treat as zero for our purposes
	{
		if (mantissa == 0) // Inf
		{
			//printf("Infinite\n");
			v.i = (sign << 31) | 0x7f800000;
			return v.f;
		}
		else // NaN
		{
			//printf("NaN\n");
			v.i = (sign << 31) | 0x7f800000 | (mantissa << 13);
			return v.f;
		}
	}

	//printf("Normal number\n");
	//else a normal number; fall through from sub-normal case.
	exponent = exponent + (127 - 15);
    mantissa = mantissa << 13;
	//printf("   0x%x, 0x%x, 0x%x\n", sign, exponent, mantissa);
    
    v.i = (sign << 31) | (exponent << 23) | mantissa;
    //printf("  0x%x\n", v.i);
    return v.f;
}

