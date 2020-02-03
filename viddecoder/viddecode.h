#ifndef VIDDECODE_H__
#define VIDDECODE_H__

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <stdio.h>

int init(void* dec, char path[]);
void* mallocDecoderData();
uint8_t** nextFrame(void* dec);
void releaseResources(void* dec);
int convertToRGB(void* dec, uint8_t* dest);
int getWidth(void* dec);
int getHeight(void* dec);


struct decoderData {
    int status;
    AVFormatContext *pFormatContext;
    AVCodec *pCodec;
    AVCodecParameters *pCodecParameters;
    AVCodecContext *pCodecContext;
    AVFrame *pFrame;

    struct SwsContext *pSwsContext;

    int video_stream_index;

    int width;
    int height;
    int framerateden;
    int frameratequo;
};

#endif