#include "converttask.h"
#include <iostream>
#include <QThread>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/bsf.h>
}

ConvertTask::ConvertTask(QObject *parent)
    : QThread{parent}
{}


void ConvertTask::setInFilePath(QString inFilePath) {
    this->inFilePath = inFilePath;
}


void ConvertTask::setOutFilePath(QString outFilePath) {
    this->outFilePath = outFilePath;
}

void ConvertTask::run() {
    std::cout << "start to convert" << std::endl;
    AVFormatContext* inFormatContext = nullptr;
    AVFormatContext* outFormatContext = nullptr;
    int inStreamNum = 0;
    int* streamIndexMapping = nullptr;
    int validSteamIndex = 0;
    AVPacket* pkt = nullptr;
    int ret = -1;
    int videoStreamIndex = -1;

    const AVBitStreamFilter* bitStreamFilter = av_bsf_get_by_name("h264_mp4toannexb");
    AVBSFContext* bsfContext = nullptr;
    if (bitStreamFilter) {
        ret = av_bsf_alloc(bitStreamFilter, &bsfContext);
        if (ret != 0) {
            std::cout << "Failed to invoke av_bsf_alloc method, ret=" << ret << std::endl;
        }
    }

    ret = avformat_open_input(&inFormatContext, inFilePath.toStdString().c_str(), nullptr, nullptr);
    if (ret != 0) {
        std::cout << "Failed to invoke avformat_open_input method, ret=" << ret << std::endl;
        goto end;
    }
    ret = avformat_find_stream_info(inFormatContext, nullptr);
    if (ret < 0) {
        std::cout << "Failed to invoke avformat_find_stream_info method, ret=" << ret << std::endl;
    }

    std::cout << "inFormatContext->iformat->name=" << inFormatContext->iformat->name << std::endl;
    std::cout << "convertTask(), totalDurationUS=" <<  inFormatContext->duration / AV_TIME_BASE << std::endl;

    inStreamNum = inFormatContext->nb_streams;
    streamIndexMapping = new int[inStreamNum];

    ret = avformat_alloc_output_context2(&outFormatContext, nullptr, nullptr, outFilePath.toStdString().c_str());
    if (ret < 0) {
        std::cout << "Failed to invoke avformat_alloc_output_context2 method, ret=" << ret << std::endl;
        goto end;
    }

    for (int index = 0; index < inStreamNum; index++) {
        AVStream* inStream = inFormatContext->streams[index];
        int mediaType = inStream->codecpar->codec_type;
        if (mediaType == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = index;
        }
        if (mediaType != AVMEDIA_TYPE_AUDIO
            && mediaType != AVMEDIA_TYPE_VIDEO
            && mediaType != AVMEDIA_TYPE_SUBTITLE) {
            streamIndexMapping[index] = -1;
            std::cout << "Skip invalid mediaType =" << mediaType << std::endl;
            continue;
        }

        streamIndexMapping[index] = validSteamIndex++;
        AVStream* outStream = avformat_new_stream(outFormatContext, nullptr);
        ret = avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
        if (ret < 0) {
            std::cout << "Failed to invoke avcodec_parameters_copy method, ret=" << ret << std::endl;
            goto end;
        }

        if (inStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO
                                        && outFilePath.endsWith("avi")) {

            avcodec_parameters_copy(bsfContext->par_in, inStream->codecpar);
            av_bsf_init(bsfContext);
        }
        outStream->codecpar->codec_tag = 0;
    }

    if (!(outFormatContext->flags & AVFMT_NOFILE)) {
        ret = avio_open(&outFormatContext->pb, outFilePath.toStdString().c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            std::cout << "Failed to invoke avio_open method" << std::endl;
        }
    }


    ret = avformat_write_header(outFormatContext, nullptr);
    if (ret < 0) {
        std::cout << "Failed to invoke avformat_write_header method, ret=" << ret << std::endl;
        goto end;
    }

    emit notifyProgress(10);

    pkt = av_packet_alloc();
    while(true) {
        ret = av_read_frame(inFormatContext, pkt);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                std::cout << "Read frame reache the end of the file, ret=" << ret << std::endl;
                break;
            }
            std::cout << "Failed to invoke av_read_frame method, ret=" << ret << std::endl;
            goto end;
        }
        if (pkt->stream_index >= inStreamNum || streamIndexMapping[pkt->stream_index] < 0) {
            av_packet_unref(pkt);
            continue;
        }

        if (pkt->stream_index==videoStreamIndex && outFilePath.endsWith("avi")) {
            //Use av_bsf_send_packet() and av_bsf_receive_packet()
            ret = av_bsf_send_packet(bsfContext, pkt);
            if (ret < 0)
            {
                std::cout << "av_bsf_send_packet failed!" << std::endl;
                av_packet_unref(pkt);
                continue;
            }
            av_packet_unref(pkt);
            ret = av_bsf_receive_packet(bsfContext, pkt);
            if (ret < 0)
            {
                std::cout << "av_bsf_receive_packet failed!" << std::endl;
                av_packet_unref(pkt);
                continue;
            }
        }

        int outStreamIndex = streamIndexMapping[pkt->stream_index];
        AVStream* inStream = inFormatContext->streams[pkt->stream_index];
        AVStream* outStream = outFormatContext->streams[outStreamIndex];
        pkt->stream_index = outStreamIndex;
        av_packet_rescale_ts(pkt, inStream->time_base, outStream->time_base);
        ret = av_interleaved_write_frame(outFormatContext, pkt);
        if (ret < 0) {
            break;
        }


        double progress = 10 + (double)avio_tell(inFormatContext->pb) / avio_size(inFormatContext->pb) * 80;

        std::cout << "ConvertTask#run(), progress=" << progress <<", current thread id = "<< QThread::currentThreadId() << std::endl;

        QThread::currentThread()->usleep(2);

        emit notifyProgress(progress);
    }

    emit notifyProgress(90);
    av_write_trailer(outFormatContext);
    emit notifyProgress(100);


end:
    std::cout << "Finish to convert video formatter" << std::endl;
    av_bsf_free(&bsfContext);
    avformat_close_input(&inFormatContext);
    if (!(outFormatContext->flags & AVFMT_NOFILE)) {
        avio_close(outFormatContext->pb);
    }
    avformat_free_context(outFormatContext);
    delete[] streamIndexMapping;
}
