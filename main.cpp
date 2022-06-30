#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
}

int WriteFile(const char *filename, void *buf, int size, const char *mode) {
    FILE *fp = fopen(filename, mode);
    if(fp == NULL) {
        printf("fopen %s failed\n", filename);
        return -1;
    }
    if(buf != NULL) {
        fwrite(buf, 1, size, fp);
    }
    fclose(fp);
    return 0;
}

int main(int argc, char* argv[]) {
    int ret;
    char errstr[256];
    int video_index = -1, audio_index = -1;
    AVFormatContext* ifmt_ctx = NULL;

    if(argc < 2) {
        printf("usage: ./test rtmp_url\n");
        return 0;
    }
    const char* url = argv[1];

    avcodec_register_all();
    av_register_all();
    av_log_set_level(AV_LOG_FATAL);

    if((ret = avformat_open_input(&ifmt_ctx, url, NULL, NULL)) < 0) {
        av_strerror(ret, errstr, sizeof(errstr));
        printf("Could not open input file, %s\n", errstr);
        return -1;
    }
    if((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        printf("Failed to retrieve input stream information\n");
        return -1;
    }
    for(int i = 0; i < ifmt_ctx->nb_streams; i++) {
        if(ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            video_index = i;
        }
        else if(ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_index = i;
        }
        else{
            printf("streams:%d, unsupport type : %d\n",
                    ifmt_ctx->nb_streams, ifmt_ctx->streams[i]->codec->codec_type);
            return -1;
        }
    }
    AVBSFContext* bsf_ctx = NULL; 
    const AVBitStreamFilter* pfilter = av_bsf_get_by_name("h264_mp4toannexb");
    if(pfilter == NULL) {
        printf("get bsf failed\n");
        return -1;
    }
    if((ret = av_bsf_alloc(pfilter, &bsf_ctx)) != 0) {
        printf("alloc bsf failed\n");
        return -1;
    }
    ret = avcodec_parameters_from_context(bsf_ctx->par_in, ifmt_ctx->streams[video_index]->codec);
    if(ret < 0) {
        printf("set codec failed\n");
        return -1;
    }
    bsf_ctx->time_base_in = ifmt_ctx->streams[video_index]->codec->time_base;
    ret = av_bsf_init(bsf_ctx);
    if(ret < 0) {
        printf("init bsf failed\n");
        return -1;
    }

    //int frame_id = 0;
    AVPacket pkt;
    while(av_read_frame(ifmt_ctx, &pkt) >= 0) {
        if(pkt.stream_index != video_index) {
            printf("warning, not support stream index %d, video:%d,audio:%d\n", 
                    pkt.stream_index, video_index, audio_index);
            av_packet_unref(&pkt);
            continue;
        }
        ret = av_bsf_send_packet(bsf_ctx, &pkt);
        if(ret != 0) {
            printf("warning, send pkt failed, ret:%d\n", ret);
            break;
        }
        ret = av_bsf_receive_packet(bsf_ctx, &pkt);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            printf("warning, recv pkt EAGAIN/EOF, ret:%d\n", ret);
            break;
        }
        else if (ret < 0) {
            printf("warning, recv pkt failed, ret:%d\n", ret);
            break;
        }
        //printf("##test, frameid:%d, pkt size:%d, %02x:%02x:%02x:%02x:%02x\n", 
        //        frame_id++, pkt.size, pkt.data[0], pkt.data[1], pkt.data[2], pkt.data[3], pkt.data[4]);
        //WriteFile("test.264", pkt.data, pkt.size, "a+");
        av_packet_unref(&pkt);
    }
    
    av_bsf_free(&bsf_ctx);
    avformat_close_input(&ifmt_ctx);
    printf("run ok\n");

    return 0;
}

