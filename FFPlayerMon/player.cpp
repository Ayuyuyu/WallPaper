#include "player.h"
#include <algorithm>
#include <chrono>
#include <iostream>
extern "C" {
	#include <libavutil/time.h>
	#include <libavutil/imgutils.h>
	#include <libswresample\swresample.h>
}
/*
#define MAX_AUDIO_FRAME_SIZE 192000
#define SDL_AUDIO_BUFFER_SIZE 1024
typedef struct AudioParams {
	int freq;
	int channels;
	int64_t channel_layout;
	enum AVSampleFormat fmt;
	int frame_size;
	int bytes_per_sec;
} FF_AudioParams;

static FF_AudioParams s_audio_param_src;
static FF_AudioParams s_audio_param_tgt;
*/

const size_t Player::queue_size_{6};


Player::Player(const std::string& file_name) :
	demuxer_{ std::make_unique<Demuxer>(file_name) },
	video_decoder_{ std::make_unique<VideoDecoder>(
		demuxer_->video_codec_parameters()) },
	//audio_decoder_{ std::make_unique<AudioDecoder>(demuxer_->audio_codec_parameters()) },
	format_converter_{ std::make_unique<FormatConverter>(
		video_decoder_->width(), video_decoder_->height(),
		video_decoder_->pixel_format(), AV_PIX_FMT_YUV420P) },
	display_{ std::make_unique<Display>(
		video_decoder_->width(), video_decoder_->height()) },
	timer_{ std::make_unique<Timer>() },
	packet_queue_{ std::make_unique<PacketQueue>(queue_size_) },		//解封装 解格式
	frame_queue_{ std::make_unique<FrameQueue>(queue_size_) }			//解码
	// yuc add
	//audio_packet_queue_{ std::make_unique<PacketQueue>(queue_size_) },
	//audio_frame_queue_{ std::make_unique<FrameQueue>(queue_size_) }
{
	/*audio_buff = new uint8_t[MAX_AUDIO_FRAME_SIZE];
	audio_buff_size = 0;
	audio_buff_index = 0;
	video_first = 1;*/
}	

Player::~Player(){
	//delete[] audio_buff;
}
void Player::operator()() {
	stages_.emplace_back(&Player::demultiplex, this);			//demultiplex线程
	stages_.emplace_back(&Player::decode_video, this);		//decode_video线程
	//stages_.emplace_back(&Player::prepare_to_play, this);
	//audio();
	video();
	//video_forever();
	//stages_.emplace_back(&Player::video, this);

	// 启动
	for (auto &stage : stages_) {		
		stage.join();
	}

	if (exception_) {
		std::rethrow_exception(exception_);
	}
}

//解封装  解协议
void Player::demultiplex() {
	try {
		for (;;) {
			// Create AVPacketni
			std::unique_ptr<AVPacket, std::function<void(AVPacket*)>> packet{				//new avpacket
				new AVPacket,
				[](AVPacket* p){ av_packet_unref(p); delete p; }};  
			av_init_packet(packet.get());
			packet->data = nullptr;

			// Read frame into AVPacket
			if (!(*demuxer_)(*packet)) {			//读取到avpacket   //av_read_frame
				//packet_queue_->finished();
				//break;
				std::cout << "demultiplex run over" << std::endl;
				int ret = demuxer_->reset_video_stream();
			}
			if (!packet->buf)
			{
				continue;
			}
			// Move into queue if first video stream
			// 读入到queue 后面可以进行其他解码
			if (packet->stream_index == demuxer_->video_stream_index()) {
				if (!packet_queue_->push(move(packet))) {			//推入 packet queue
					break;
				}
			}
			// audio 类型 index 
			/*
			else if (packet->stream_index == demuxer_->audio_stream_index()) {
				if (!audio_packet_queue_->push(move(packet))) {			//推入 packet queue
					break;
				}
			}
			*/
		}
	} catch (...) {
		exception_ = std::current_exception();
		frame_queue_->quit();
		packet_queue_->quit();
		//audio_packet_queue_->quit();
	}
}

//解码video

void Player::decode_video() {
	try {
		const AVRational microseconds = {1, 1000000};

		for (;;) {
			// Create AVFrame and AVQueue
			std::unique_ptr<AVFrame, std::function<void(AVFrame*)>>
				frame_decoded{av_frame_alloc(), [](AVFrame* f){ av_frame_free(&f); }};
			std::unique_ptr<AVPacket, std::function<void(AVPacket*)>> packet{
				nullptr, [](AVPacket* p){ av_packet_unref(p); delete p; }};

			// Read packet from queue	读出
			if (!packet_queue_->pop(packet)) {
				frame_queue_->finished();
				break;
			}

			// video读入decode
			// If the packet didn't send, receive more frames and try again  
			bool sent = false;
			while (!sent) {
				/******************video**************************/
				sent = video_decoder_->send(packet.get());

				// If a whole frame has been decoded,
				// adjust time stamps and add to queue‘
				while (video_decoder_->receive(frame_decoded.get())) {		//get frame获取解析 
					frame_decoded->pts = av_rescale_q(
						frame_decoded->pkt_dts,
						demuxer_->time_base(),
						microseconds);

					std::unique_ptr<AVFrame, std::function<void(AVFrame*)>>			//converted frame结果
						frame_converted{
							av_frame_alloc(),
							[](AVFrame* f){ av_free(f->data[0]); }};
					if (av_frame_copy_props(frame_converted.get(),
						frame_decoded.get()) < 0) {
						throw std::runtime_error("Copying frame properties");
					}
					if (av_image_alloc(
						frame_converted->data, frame_converted->linesize,
						video_decoder_->width(), video_decoder_->height(),
						video_decoder_->pixel_format(), 1) < 0) {
						throw std::runtime_error("Allocating picture");
					}
					(*format_converter_)(
						frame_decoded.get(), frame_converted.get());		//sws_scale
					//避免错误问题
					if (frame_converted->pts <= 0) {
						continue;
					}
					if (!frame_queue_->push(move(frame_converted))) {
						break;
					}
				}
			}
		}
	} catch (...) {
		exception_ = std::current_exception();
		frame_queue_->quit();
		packet_queue_->quit();
	}
	std::cout << "decode_video" << std::endl;

}

//sdl部分播放
void Player::video() {
	try {
		int64_t last_pts = 0;

		for (uint64_t frame_number = 0;; ++frame_number) {
			//display_->input();		//sdl 键盘输入获取
			if (display_->get_quit()) {		//获取状态
				break;
			} else if (display_->get_play()) {		
				std::unique_ptr<AVFrame, std::function<void(AVFrame*)>> frame{
					nullptr, [](AVFrame* f){ av_frame_free(&f); }};
				if (!frame_queue_->pop(frame)) {		//pop出一个frame
					break;
					//continue;
				}

				if (frame_number) {
					if (last_pts > frame->pts){
						last_pts = frame->pts;
						timer_->update();
					}
					const int64_t frame_delay = frame->pts - last_pts;
					//std::cout << "frame_number" << " " << frame_delay << " " << frame->pts << " "<<last_pts <<std::endl;
					last_pts = frame->pts;
					timer_->wait(frame_delay);	//刷新率跟相关
				} else {
					last_pts = frame->pts;
					timer_->update();
				}
			
				display_->refresh(
					{frame->data[0], frame->data[1], frame->data[2]},
					{static_cast<size_t>(frame->linesize[0]),
					 static_cast<size_t>(frame->linesize[1]),
					 static_cast<size_t>(frame->linesize[2])});

			} else {
				std::chrono::milliseconds sleep(10);
				std::this_thread::sleep_for(sleep);
				timer_->update();
			}
		}

	} catch (...) {
		exception_ = std::current_exception();
	}

	frame_queue_->quit();
	packet_queue_->quit();
}

void Player::prepare_to_play()
{
	try {
		const AVRational microseconds = { 1, 1000000 };
		for (;;) {
			// Create AVPacket
			std::unique_ptr<AVPacket, std::function<void(AVPacket*)>> packet{				//new avpacket
				new AVPacket,
				[](AVPacket* p) { av_packet_unref(p); delete p; } };
			av_init_packet(packet.get());
			packet->data = nullptr;

			// Read frame into AVPacket
			if (!(*demuxer_)(*packet)) {			//读取到avpacket   //av_read_frame
				std::cout << "demultiplex run over" << std::endl;
				//frame_queue_->finished();
				//break;
				int ret = demuxer_->reset_video_stream();
			}
			if (!packet->buf ||packet->pts <0 )
			{
				continue;
			}
			// Move into queue if first video stream
			// 读入到queue 后面可以进行其他解码
			if (packet->stream_index == demuxer_->video_stream_index()) {
				std::unique_ptr<AVFrame, std::function<void(AVFrame*)>>
					frame_decoded{ av_frame_alloc(), [](AVFrame* f) { av_frame_free(&f); } };
				bool sent = false;
				while (!sent) {
					/******************video**************************/
					sent = video_decoder_->send(packet.get());
					// If a whole frame has been decoded,
					// adjust time stamps and add to queue‘
					while (video_decoder_->receive(frame_decoded.get())) {		//get frame获取解析 
						frame_decoded->pts = av_rescale_q(
							frame_decoded->pkt_dts,
							demuxer_->time_base(),
							microseconds);

						std::unique_ptr<AVFrame, std::function<void(AVFrame*)>>			//converted frame结果
							frame_converted{
								av_frame_alloc(),
								[](AVFrame* f) { av_free(f->data[0]); } };
						if (av_frame_copy_props(frame_converted.get(),
							frame_decoded.get()) < 0) {
							throw std::runtime_error("Copying frame properties");
						}
						if (av_image_alloc(
							frame_converted->data, frame_converted->linesize,
							video_decoder_->width(), video_decoder_->height(),
							video_decoder_->pixel_format(), 1) < 0) {
							throw std::runtime_error("Allocating picture");
						}
						(*format_converter_)(frame_decoded.get(), frame_converted.get());		//sws_scale
						if (frame_converted->pts <=0){
							continue;
						}
						if (!frame_queue_->push(move(frame_converted))) {
							break;
						}
					}
				}

			}
		}
	}
	catch (...) {
		exception_ = std::current_exception();
		frame_queue_->quit();
		//packet_queue_->quit();
	}
}



/*
int audio_decode_frame(Player *audio_state, uint8_t *audio_buf, int buf_size)
{
	AVFrame *frame = av_frame_alloc();
	int data_size = 0;

	SwrContext *swr_ctx = nullptr;
	static double clock = 0;

	std::unique_ptr<AVFrame, std::function<void(AVFrame*)>>
	frame_decoded{ av_frame_alloc(), [](AVFrame* f) { av_frame_free(&f); } };
	std::unique_ptr<AVPacket, std::function<void(AVPacket*)>> packet{
	nullptr, [](AVPacket* p) { av_packet_unref(p); delete p; } };

	if (!audio_state->audio_packet_queue_->pop(packet)) {
		audio_state->frame_queue_->finished();
		return -1;
	}

	audio_state->audio_decoder_->send(packet.get());
	audio_state->audio_decoder_->receive(frame_decoded.get());

	// 设置通道数或channel_layout
	if (frame_decoded->channels > 0 && frame_decoded->channel_layout == 0)
		frame_decoded->channel_layout = av_get_default_channel_layout(frame_decoded->channels);
	else if (frame_decoded->channels == 0 && frame_decoded->channel_layout > 0)
		frame_decoded->channels = av_get_channel_layout_nb_channels(frame_decoded->channel_layout);

	AVSampleFormat dst_format = AV_SAMPLE_FMT_S16;//av_get_packed_sample_fmt((AVSampleFormat)frame_decoded->format);
	Uint64 dst_layout = av_get_default_channel_layout(frame_decoded->channels);
	// 设置转换参数
	swr_ctx = swr_alloc_set_opts(nullptr, dst_layout, dst_format, frame_decoded->sample_rate,
		frame_decoded->channel_layout, (AVSampleFormat)frame_decoded->format, frame_decoded->sample_rate, 0, nullptr);
	if (!swr_ctx || swr_init(swr_ctx) < 0)
		return -1;

	// 计算转换后的sample个数 a * b / c
	uint64_t dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, frame_decoded->sample_rate) + frame_decoded->nb_samples, frame_decoded->sample_rate, frame_decoded->sample_rate, AVRounding(1));
	// 转换，返回值为转换后的sample个数
	int nb = swr_convert(swr_ctx, &audio_buf, static_cast<int>(dst_nb_samples), (const uint8_t**)frame_decoded->data, frame_decoded->nb_samples);
	data_size = frame_decoded->channels * nb * av_get_bytes_per_sample(dst_format);


	av_frame_free(&frame);
	swr_free(&swr_ctx);

	return data_size;
}

void sdl_audio_callback(void *userdata, uint8_t *stream, int len) {

	Player *audio_state = (Player*)userdata;

	SDL_memset(stream, 0, len);

	int audio_size = 0;
	int len1 = 0;
	while (len > 0)// 向设备发送长度为len的数据
	{
		if (audio_state->audio_buff_index >= audio_state->audio_buff_size) // 缓冲区中无数据
		{
			// 从packet中解码数据
			audio_size = audio_decode_frame(audio_state, audio_state->audio_buff, sizeof(audio_state->audio_buff));
			if (audio_size < 0) // 没有解码到数据或出错，填充0
			{
				audio_state->audio_buff_size = 0;
				memset(audio_state->audio_buff, 0, audio_state->audio_buff_size);
			}
			else
				audio_state->audio_buff_size = audio_size;

			audio_state->audio_buff_index = 0;
		}
		len1 = audio_state->audio_buff_size - audio_state->audio_buff_index; // 缓冲区中剩下的数据长度
		if (len1 > len) // 向设备发送的数据长度为len
			len1 = len;

		SDL_MixAudio(stream, audio_state->audio_buff + audio_state->audio_buff_index, len, SDL_MIX_MAXVOLUME);

		len -= len1;
		stream += len1;
		audio_state->audio_buff_index += len1;
	}
}


void Player::audio() {

		SDL_AudioSpec wanted_spec_;

		// 2) 音频设备打开后播放静音，不启动回调，调用SDL_PauseAudio(0)后启动回调，开始正常播放音频
		wanted_spec_.freq = audio_decoder_->codec_context_->sample_rate;    // 采样率
		wanted_spec_.format = AUDIO_S16SYS;              // S表带符号，16是采样深度，SYS表采用系统字节序
		wanted_spec_.channels = audio_decoder_->codec_context_->channels;   // 声音通道数
		wanted_spec_.silence = 0;                        // 静音值
		wanted_spec_.samples = SDL_AUDIO_BUFFER_SIZE;    // SDL声音缓冲区尺寸，单位是单声道采样点尺寸x通道数
		wanted_spec_.callback = sdl_audio_callback;      // 回调函数，若为NULL，则应使用SDL_QueueAudio()机制
		wanted_spec_.userdata = this;             // 提供给回调函数的参数
		if (SDL_OpenAudio(&wanted_spec_, nullptr) < 0)
		{
			printf("SDL_OpenAudio() failed: %s\n", SDL_GetError());
			return ;
		}

		SDL_PauseAudio(0);

}
*/