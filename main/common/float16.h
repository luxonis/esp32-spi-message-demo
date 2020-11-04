#ifndef __FLOAT_16__
#define __FLOAT_16__

#include <stdint.h>

/* data type for storing float16s */
typedef union
{
	struct
	{
		uint16_t sign : 1;
		uint16_t exponent : 5;
		uint16_t mantissa: 10;
	} number;

	struct
	{
		uint8_t msw;
		uint8_t lsw;
	} words;

	uint16_t bits;
} _float16_shape_type;

#ifdef __cplusplus
extern "C" {
#endif

float float16_to_float32(_float16_shape_type f16_val);

#ifdef __cplusplus
}
#endif

#endif
