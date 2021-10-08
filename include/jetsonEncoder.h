#ifndef _JETSON_ENCODER_H_
#define _JETSON_ENCODER_H_

#include <stdint.h>
#include <iostream>
#include <fstream>
#include "NvVideoEncoder.h"

using namespace std;

#define TEST_ERROR(cond, str) if(cond) { \
                                        cerr << str << endl; \
                                        }

typedef struct
{
    NvVideoEncoder *enc;
    uint32_t encoder_pixfmt;

    char *in_file_path;
    std::ifstream *in_file;

    uint32_t width;
    uint32_t height;

    char *out_file_path;
    std::ofstream *out_file;

    uint32_t bitrate;
    uint32_t fps_n;
    uint32_t fps_d;

    bool got_error;
} context_t;


typedef struct
{
    int target_header;
    int target_id[20];
    int target_x[20];
    int target_y[20];
    int target_w[20];
    int target_h[20];
    int target_class[20];
    int target_prob[20];
    int target_num;
    float target_velocity[20];
} targetInfo;

typedef struct
{
    bool use_hdr;
    bool use_flip;
} controlData;

class jetsonEncoder
{
    public:
    jetsonEncoder();
    ~jetsonEncoder();
    int encodeFrame(uint8_t *yuv_bytes);
    int pubTargetData(targetInfo  target_data);
    controlData getControlData();
    
    private:
    void set_defaults(context_t * ctx);
    
    void copyYuvToBuffer(uint8_t *yuv_bytes, NvBuffer &buffer);

    context_t ctx;
    int frame_count;
};

#endif