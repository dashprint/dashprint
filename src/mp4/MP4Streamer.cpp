#include "MP4Streamer.h"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <stdexcept>

// http://www.ffmpeg.org/doxygen/trunk/doc_2examples_2remuxing_8c-example.html

MP4Streamer::MP4Streamer(std::shared_ptr<H264Source> source, MP4Sink* target)
: m_source(source), m_target(target)
{
	const int bufferSize = 8192*12;

	unsigned char* buffer = (unsigned char*) ::av_malloc(bufferSize);
	m_avioContextIn = ::avio_alloc_context(buffer, bufferSize, 0, this, readPacket, nullptr, nullptr);

	m_inputFormatContext = ::avformat_alloc_context();
	m_inputFormatContext->pb = m_avioContextIn;
	
	buffer = (unsigned char*) ::av_malloc(bufferSize);
	m_avioContextOut = ::avio_alloc_context(buffer, bufferSize, 1, this, nullptr, writePacket, nullptr);

	int status = avformat_alloc_output_context2(&m_outputFormatContext, nullptr, "mp4", "stream.mp4");
	m_outputFormatContext->pb = m_avioContextOut;
}

MP4Streamer::~MP4Streamer()
{
	if (m_avioContextIn)
	{
		::av_free(m_avioContextIn->buffer);
		::av_free(m_avioContextIn);
	}
	if (m_avioContextOut)
	{
		::av_free(m_avioContextOut->buffer);
		::av_free(m_avioContextOut);
	}
	if (m_inputFormatContext)
		::avformat_free_context(m_inputFormatContext);
	if (m_outputFormatContext)
		::avformat_free_context(m_outputFormatContext);
}

void MP4Streamer::run()
{
	m_stop = false;
	
	int status = ::avformat_open_input(&m_inputFormatContext, "/tmp/working.264", nullptr, nullptr);
	if (status != 0)
		throwAverror("avformat_open_input() failed:", status);

	status = ::avformat_find_stream_info(m_inputFormatContext, nullptr);
	if (status != 0)
		throwAverror("avformat_find_stream_info() failed:", status);

	AVStream* inStream = m_inputFormatContext->streams[0];
	AVStream* outStream = ::avformat_new_stream(m_outputFormatContext, nullptr);

	if (!outStream)
		throw std::runtime_error("avformat_new_stream() failed");

	status = ::avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
	if (status != 0)
		throwAverror("avcodec_copy_context() failed:", status);

	outStream->avg_frame_rate = inStream->avg_frame_rate;
	outStream->r_frame_rate = inStream->r_frame_rate;
	outStream->time_base = inStream->time_base;
	outStream->sample_aspect_ratio = inStream->sample_aspect_ratio;

	status = ::avformat_transfer_internal_stream_timing_info(m_outputFormatContext->oformat, outStream, inStream, AVFMT_TBCF_AUTO);

	AVDictionary* opts = nullptr;
	av_dict_set(&opts, "movflags", "frag_keyframe+empty_moov", 0);

	status = ::avformat_write_header(m_outputFormatContext, &opts);
	if (opts != nullptr)
		av_dict_free(&opts);
	if (status != 0)
		throwAverror("avformat_write_header() failed:", status);

	int64_t dts = 0;
	while (!m_stop)
	{
		AVPacket packet;

		status = ::av_read_frame(m_inputFormatContext, &packet);
		if (status != 0)
			throwAverror("av_read_frame() failed:", status);
		
		if (packet.stream_index != 0)
		{
			::av_packet_unref(&packet);
			continue;
		}

		// This is absolutely critical for correct playback
		if (packet.dts == AV_NOPTS_VALUE)
		{
			packet.dts = dts;
			dts += packet.duration;
		}

		//packet.pts = ::av_rescale_q_rnd(packet.pts, inStream->time_base, outStream->time_base, AVRounding(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		//packet.dts = ::av_rescale_q_rnd(packet.dts, inStream->time_base, outStream->time_base, AVRounding(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		//packet.duration = ::av_rescale_q(packet.duration, inStream->time_base, outStream->time_base);
		//packet.pos = -1;

		status = ::av_interleaved_write_frame(m_outputFormatContext, &packet);
		::av_packet_unref(&packet);

		if (status != 0)
			throwAverror("av_interleaved_write_frame() failed:", status);
	}
}

int MP4Streamer::readPacket(void* opaque, uint8_t* buf, int bufSize)
{
	MP4Streamer* This = static_cast<MP4Streamer*>(opaque);
	return This->m_source->read(buf, bufSize);
}

int MP4Streamer::writePacket(void* opaque, uint8_t* buf, int bufSize)
{
	MP4Streamer* This = static_cast<MP4Streamer*>(opaque);
	return This->m_target->write(buf, bufSize);
}

int MP4Streamer::throwAverror(const char* descr, int err)
{
	std::stringstream ss;
	char buf[128];

	ss << descr;
	if (::av_strerror(err, buf, sizeof(buf)) < 0)
		std::strcpy(buf, "Unknown error");
	ss << buf;
	
	throw std::runtime_error(ss.str());
}
