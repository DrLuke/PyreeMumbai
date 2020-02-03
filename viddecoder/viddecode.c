/*
 * http://ffmpeg.org/doxygen/trunk/index.html
 *
 * Main components
 *
 * Format (Container) - a wrapper, providing sync, metadata and muxing for the streams.
 * Stream - a continuous stream (audio or video) of data over time.
 * Codec - defines how data are enCOded (from Frame to Packet)
 *        and DECoded (from Packet to Frame).
 * Packet - are the data (kind of slices of the stream data) to be decoded as raw frames.
 * Frame - a decoded raw frame (to be encoded or filtered).
 */

#include "viddecode.h"

// print out the steps and errors
static void logging(const char *fmt, ...);
// decode packets into frames
static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame);
// save a frame into a .pgm file
static void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename);



void* mallocDecoderData() {
    void* retval = malloc(sizeof(struct decoderData));
    return retval;
}

int init(void* dec, char path[]) {
    struct decoderData* decoderdata;

    /*for(int i = 0; i <1000; i++) {
        printf("%c", path[i]);
        if(path[i] == 0){break;}
    }*/


    decoderdata = (struct decoderData*)dec;
    decoderdata->status = 0;

    //logging("initializing all the containers, codecs and protocols.");

    av_register_all();

    decoderdata->pFormatContext = avformat_alloc_context();
    if (!decoderdata->pFormatContext) {
        //logging("ERROR could not allocate memory for Format Context");
        decoderdata->status = -1;
        return -1;
    }

    //logging("opening the input file (%s) and loading format (container) header", path);
    if (avformat_open_input(&decoderdata->pFormatContext, path, NULL, NULL) != 0) {
        //logging("ERROR could not open the file");
        decoderdata->status = -1;
        return -1;
    }

    // now we have access to some information about our file
    // since we read its header we can say what format (container) it's
    // and some other information related to the format itself.
    //logging("format %s, duration %lld us, bit_rate %lld", decoderdata->pFormatContext->iformat->long_name, decoderdata->pFormatContext->duration, decoderdata->pFormatContext->bit_rate);

    //logging("finding stream info from format");
    // read Packets from the Format to get stream information
    // this function populates pFormatContext->streams
    // (of size equals to pFormatContext->nb_streams)
    // the arguments are:
    // the AVFormatContext
    // and options contains options for codec corresponding to i-th stream.
    // On return each dictionary will be filled with options that were not found.
    // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb
    if (avformat_find_stream_info(decoderdata->pFormatContext,  NULL) < 0) {
        //logging("ERROR could not get the stream info");
        decoderdata->status = -1;
        return -1;
    }

    // the component that knows how to enCOde and DECode the stream
    // it's the codec (audio or video)
    // http://ffmpeg.org/doxygen/trunk/structAVCodec.html
    decoderdata->pCodec = NULL;
    // this component describes the properties of a codec used by the stream i
    // https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html
    decoderdata->pCodecParameters =  NULL;
    decoderdata->video_stream_index  = -1;

    // loop though all the streams and print its main information
    for (int i = 0; i < decoderdata->pFormatContext->nb_streams; i++)
    {
        AVCodecParameters *pLocalCodecParameters =  NULL;
        pLocalCodecParameters = decoderdata->pFormatContext->streams[i]->codecpar;
        //logging("AVStream->time_base before open coded %d/%d", decoderdata->pFormatContext->streams[i]->time_base.num, decoderdata->pFormatContext->streams[i]->time_base.den);
        //logging("AVStream->r_frame_rate before open coded %d/%d", decoderdata->pFormatContext->streams[i]->r_frame_rate.num, decoderdata->pFormatContext->streams[i]->r_frame_rate.den);
        //logging("AVStream->start_time %" PRId64, decoderdata->pFormatContext->streams[i]->start_time);
        //logging("AVStream->duration %" PRId64, decoderdata->pFormatContext->streams[i]->duration);

        //logging("finding the proper decoder (CODEC)");

        AVCodec *pLocalCodec = NULL;

        // finds the registered decoder for a codec ID
        // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
        pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

        if (pLocalCodec==NULL) {
            //logging("ERROR unsupported codec!");
            decoderdata->status = -1;
            return -1;
        }

        // when the stream is a video we store its index, codec parameters and codec
        if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            decoderdata->video_stream_index = i;
            decoderdata->pCodec = pLocalCodec;
            decoderdata->pCodecParameters = pLocalCodecParameters;
            decoderdata->width = pLocalCodecParameters->width;
            decoderdata->height = pLocalCodecParameters->height;

            //logging("Video Codec: resolution %d x %d", pLocalCodecParameters->width, pLocalCodecParameters->height);
        }

        // print its name, id and bitrate
        //logging("\tCodec %s ID %d bit_rate %lld", pLocalCodec->long_name, pLocalCodec->id, decoderdata->pCodecParameters->bit_rate);
    }
    // https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
    decoderdata->pCodecContext = avcodec_alloc_context3(decoderdata->pCodec);
    if (!decoderdata->pCodecContext)
    {
        //logging("failed to allocated memory for AVCodecContext");
        decoderdata->status = -1;
        return -1;
    }

    // Fill the codec context based on the values from the supplied codec parameters
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
    if (avcodec_parameters_to_context(decoderdata->pCodecContext, decoderdata->pCodecParameters) < 0)
    {
        //logging("failed to copy codec params to codec context");
        decoderdata->status = -1;
        return -1;
    }

    // Initialize the AVCodecContext to use the given AVCodec.
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d
    if (avcodec_open2(decoderdata->pCodecContext, decoderdata->pCodec, NULL) < 0)
    {
        //logging("failed to open codec through avcodec_open2");
        decoderdata->status = -1;
        return -1;
    }

    // https://ffmpeg.org/doxygen/trunk/structAVFrame.html
    decoderdata->pFrame = av_frame_alloc();
    if (!decoderdata->pFrame)
    {
        //logging("failed to allocated memory for AVFrame");
        decoderdata->status = -1;
        return -1;
    }

    // Create SwsContext for converting frames to RGB24 (unless they already are of course)
    enum AVPixelFormat src_fmt = decoderdata->pCodecContext->pix_fmt;
    enum AVPixelFormat dst_fmt = AV_PIX_FMT_RGB24;

    if(src_fmt == dst_fmt) {
        decoderdata->pSwsContext = NULL;    // Don't need to convert
    } else {
        decoderdata->pSwsContext = sws_getContext(decoderdata->width, decoderdata->height, src_fmt,
                                                  decoderdata->width, decoderdata->height, dst_fmt,
                                                  SWS_POINT, NULL, NULL, NULL);
        if(!decoderdata->pSwsContext) {
            printf("ERROR failed to create SwsContext");
        }
    }

    return 0;
}

#define CHECKSTATUS(msg) if(status<0) {/*printf(msg);*/av_packet_free(&pPacket);return NULL;}
uint8_t** nextFrame(void* dec) {
    struct decoderData* decoderdata = (struct decoderData*)dec;

    AVPacket *pPacket = av_packet_alloc();

    av_frame_unref(decoderdata->pFrame);


    int status;
    int frameGenerated = 0;

    while(!frameGenerated)
    {
        status = av_read_frame(decoderdata->pFormatContext, pPacket);
        if(status < 0) {    // File is finished or error
            CHECKSTATUS("ERROR on av_read_frame")
        }
        if (pPacket->stream_index == decoderdata->video_stream_index) { // Only take packets belonging to video stream
            status = avcodec_send_packet(decoderdata->pCodecContext, pPacket);
            CHECKSTATUS("ERROR on avcodec_send_packet")

            status = avcodec_receive_frame(decoderdata->pCodecContext, decoderdata->pFrame);
            if (status == AVERROR(EAGAIN)) {
                //printf("ERROR avcodec_receive_frame AVERROR(EAGAIN)\n");
                break;
            } else if(status == AVERROR_EOF) {
                //printf("ERROR avcodec_receive_frame EOF\n");
                break;
            } else CHECKSTATUS("ERROR on avcodec_receive_frame")
            if(status == 0) {
                frameGenerated = 1;
            }
        }
    }

    if(frameGenerated == 1) {
        return decoderdata->pFrame->data;
    }

    av_packet_free(&pPacket);   // Free packet

    return NULL;
}

int convertToRGB(void* dec, uint8_t* dest) {
    struct decoderData* decoderdata = (struct decoderData*)dec;

    uint8_t* dest_data[8];
    dest_data[0] = dest;
    int dstStride = decoderdata->width*3;
    if(decoderdata->pSwsContext) {
        sws_scale(decoderdata->pSwsContext, (const uint8_t * const*)decoderdata->pFrame->data, decoderdata->pFrame->linesize, 0, decoderdata->height,
                  dest_data, (const int*)&dstStride);

        return 0;
    }
    return -1;
}

void releaseResources(void* dec) {
    struct decoderData* decoderdata = (struct decoderData*)dec;

    avformat_close_input(&decoderdata->pFormatContext);
    avformat_free_context(decoderdata->pFormatContext);

    av_frame_free(&decoderdata->pFrame);
    avcodec_free_context(&decoderdata->pCodecContext);
    sws_freeContext(decoderdata->pSwsContext);
}

int getWidth(void* dec) {
    struct decoderData* decoderdata = (struct decoderData*)dec;
    return decoderdata->pCodecContext->width;
}

int getHeight(void* dec) {
    struct decoderData* decoderdata = (struct decoderData*)dec;
    return decoderdata->pCodecContext->height;
}

static void logging(const char *fmt, ...)
{
    /*va_list args;
    printf("LOG: " );
    va_start( args, fmt );
    vprintf( fmt, args );
    va_end( args );
    printf( "\n" );*/
}

/*static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame)
{
    // Supply raw packet data as input to a decoder
    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3
    int response = avcodec_send_packet(pCodecContext, pPacket);

    if (response < 0) {
        logging("Error while sending a packet to the decoder: %s", av_err2str(response));
        return response;
    }

    while (response >= 0)
    {
        // Return decoded output data (into a frame) from a decoder
        // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c
        response = avcodec_receive_frame(pCodecContext, pFrame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            logging("Error while receiving a frame from the decoder: %s", av_err2str(response));
            return response;
        }

        if (response >= 0) {
            logging(
                    "Frame %d (type=%c, size=%d bytes) pts %d key_frame %d [DTS %d]",
                    pCodecContext->frame_number,
                    av_get_picture_type_char(pFrame->pict_type),
                    pFrame->pkt_size,
                    pFrame->pts,
                    pFrame->key_frame,
                    pFrame->coded_picture_number
            );

            char frame_filename[1024];
            snprintf(frame_filename, sizeof(frame_filename), "%s-%d.pgm", "frame", pCodecContext->frame_number);
            // save a grayscale frame into a .pgm file
            save_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height, frame_filename);

            av_frame_unref(pFrame);
        }
    }
    return 0;
} */