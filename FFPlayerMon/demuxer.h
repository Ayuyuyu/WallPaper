#pragma once
#include <string>
extern "C" {
	#include "libavformat/avformat.h"
}

class Demuxer {
public:
	Demuxer(const std::string &file_name);
	~Demuxer();
	AVCodecParameters* video_codec_parameters();
	int video_stream_index() const;
	AVRational time_base() const;
	int reset_video_stream() const;  //����
	//��Ƶ
	AVCodecParameters* audio_codec_parameters();
	int audio_stream_index() const;
	AVRational audio_time_base() const;

	bool operator()(AVPacket &packet);

private:
	AVFormatContext* format_context_{};
	int video_stream_index_{};
	int audio_stream_index_{};		//��Ƶ
};
