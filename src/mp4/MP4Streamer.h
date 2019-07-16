#ifndef _MP4STREAMER_H
#define _MP4STREAMER_H
#include <string_view>
#include <memory>
#include <stdint.h>
#include "MP4Atom.h"

// Annex-B H.264 source
class H264Source
{
public:
	virtual ~H264Source() {}
	// Sequence Parameter Set
	virtual std::string_view spsUnit() const = 0;
	virtual std::string_view ppsUnit() const = 0;
	virtual std::string_view readNAL() const = 0;

	// Video dimensions parsing:
	// https://stackoverflow.com/questions/12018535/get-the-width-height-of-the-video-from-h-264-nalu
	virtual uint32_t width() const = 0;
	virtual uint32_t height() const = 0;
};

class MP4Sink
{
public:
	virtual void write(std::string_view data) = 0;
	void write(const MP4Atom& atom) { write(atom.data()); }
};

class MP4Streamer
{
public:
	MP4Streamer(std::shared_ptr<H264Source> source, MP4Sink* target);
	void start();
private:
	std::shared_ptr<H264Source> m_source;
	MP4Sink* m_target;
	uint64_t m_bytesWritten = 0;
};

#endif
