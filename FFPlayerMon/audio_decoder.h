#pragma once
extern "C" {
#include "libavcodec/avcodec.h"
}
#include "SDL.h"

class AudioDecoder {
public:
	AudioDecoder(AVCodecParameters* codec_parameters);
	~AudioDecoder();
	bool send(AVPacket* packet);
	bool receive(AVFrame* frame);


public:
	AVCodecContext* codec_context_{};


};


