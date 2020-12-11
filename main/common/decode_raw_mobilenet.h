#ifndef __DECODE_MOBILENET__
#define __DECODE_MOBILENET__


#include <stdio.h>
#include <string.h>
#include <math.h>

#include "float16.h"


#define CLAMP_MIN_MAX(val, min, max)  \
    do                                \
    {                                 \
        if (val < min)                \
            val = min;                \
        else if (val > max)           \
            val = max;                \
        else if (isnan(val))          \
            val = 0;                  \
    } while (0)


typedef short half;
typedef struct {
    half header;
    half label;
    half confidence;
    half x_min;
    half y_min;
    half x_max;
    half y_max;
} raw_Detection;

typedef struct {
    float header;
    float label;
    float confidence;
    float x_min;
    float y_min;
    float x_max;
    float y_max;
} Detection;

#ifdef __cplusplus
extern "C" {
#endif

float f16Tof32(half f16);
int decode_raw_mobilenet(Detection dets[], half *result, float confidence_thr, int max_detections);

#ifdef __cplusplus
}
#endif

#endif
