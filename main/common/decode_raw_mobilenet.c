#include "decode_raw_mobilenet.h"

float f16Tof32(half f16){
    _float16_shape_type test_float16;
    test_float16.words.msw = ((char*)&f16)[0];
    test_float16.words.lsw = ((char*)&f16)[1];

    return float16_to_float32(test_float16);
}

int decode_raw_mobilenet(Detection dets[], half *result, float confidence_thr, int max_detections)
{
    int i = 0;
    int detections_nr = 0;

    while (1)
    {
        raw_Detection temp;
        memcpy(&temp, &result[i*7], sizeof(raw_Detection));
        if(f16Tof32(result[i*7]) == -1.0f || i+1 > max_detections)
        {
            break;
        }
        i++;

        float current_confidence = f16Tof32(temp.confidence);
        if(current_confidence >= confidence_thr)
        {
            dets[detections_nr].label = f16Tof32(temp.label);

            dets[detections_nr].confidence = current_confidence; CLAMP_MIN_MAX(dets[detections_nr].confidence, 0., 1.);

            float x_min = f16Tof32(temp.x_min); CLAMP_MIN_MAX(x_min, 0., 1.);
            float y_min = f16Tof32(temp.y_min); CLAMP_MIN_MAX(y_min, 0., 1.);
            float x_max = f16Tof32(temp.x_max); CLAMP_MIN_MAX(x_max, 0., 1.);
            float y_max = f16Tof32(temp.y_max); CLAMP_MIN_MAX(y_max, 0., 1.);

            dets[detections_nr].x_min = x_min;
            dets[detections_nr].x_max = x_max;
            dets[detections_nr].y_min = y_min;
            dets[detections_nr].y_max = y_max;

            detections_nr++;
        }

    }
    return detections_nr;
}
