#pragma once
#include "demuxer.h"
#include "display.h"
#include "format_converter.h"
#include "queue.h"
#include "timer.h"
#include "video_decoder.h"
#include "audio_decoder.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
extern "C" {
	#include <libavcodec/avcodec.h>
}

class Player {
public:
	Player(const std::string &file_name);
	~Player();
	void operator()();
private:
	void demultiplex();
	void decode_video();
	void video();
	//yuc add
	void prepare_to_play();

	//void audio();
	//void sdl_audio_callback(void *userdata, uint8_t *stream, int len);
public:
	std::unique_ptr<Demuxer> demuxer_;
	std::unique_ptr<VideoDecoder> video_decoder_;
	//std::unique_ptr<AudioDecoder> audio_decoder_;
	std::unique_ptr<FormatConverter> format_converter_;
	std::unique_ptr<Display> display_;
	std::unique_ptr<Timer> timer_;
	std::unique_ptr<PacketQueue> packet_queue_;
	std::unique_ptr<FrameQueue> frame_queue_;
	//std::unique_ptr<PacketQueue> audio_packet_queue_;
	//std::unique_ptr<FrameQueue> audio_frame_queue_;		//audio frame queue

	std::vector<std::thread> stages_;
	static const size_t queue_size_;
	std::exception_ptr exception_{};

	//std::vector<AVFrame*> frame_vector_;
	//uint32_t audio_buff_size;			// 新取得的音频数据大小
	//uint32_t audio_buff_index  ;       // 已发送给设备的数据量
	//uint8_t *audio_buff;
};
