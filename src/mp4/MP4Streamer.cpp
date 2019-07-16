#include "MP4Streamer.h"
#include "MP4Atom.h"

MP4Streamer::MP4Streamer(std::shared_ptr<H264Source> source, MP4Sink* target)
: m_source(source), m_target(target)
{
	
}

void MP4Streamer::start()
{
	// https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap1/qtff1.html

	// Structure: http://ramugedia.com/mp4-container

	{
		MP4Atom ftyp('ftyp');
		// major brand, minor version
		ftyp.append("isom").append(0x200u); 
		// compatible brands
		ftyp.append("isomiso2avc1iso6mp41");

		m_target->write(ftyp);
		m_bytesWritten += ftyp.length();
	}

	{
		MP4Atom moov('moov');

		{
			MP4Atom mvhd('mvhd');
			mvhd.append(0u); // version + flags
			mvhd.append(0u); // creation time
			mvhd.append(0u); // mod time
			mvhd.append(1000u); // time scale 1/1000
			mvhd.append(0u); // mod time
			mvhd.append(0x10000u); // preffered rate 1.00
			mvhd.append(uint16_t(0x100)); // audio volume 1.00
			mvhd.append(10, 0); // reserved

			// transformation matrix
			mvhd.append(0x10000u);
			mvhd.append(12, 0);
			mvhd.append(0x10000u);
			mvhd.append(12, 0);
			mvhd.append(0x40000000u);
			// times and durations
			mvhd.append(24, 0);
			mvhd.append(2u); // next track

			moov += mvhd;
		}

		{
			MP4Atom trak('trak');
			{
				MP4Atom tkhd('tkhd');

				// version + flags: track enabled | track in movie
				tkhd.append(3u);
				// creation + mod time
				tkhd.append(8, 0);
				// track ID
				tkhd.append(1u);
				// tons of stuff
				tkhd.append(24, 0);
				// transformation matrix
				tkhd.append(0x10000u);
				tkhd.append(12, 0);
				tkhd.append(0x10000u);
				tkhd.append(12, 0);
				tkhd.append(0x40000000u);

				tkhd.append(m_source->width());
				tkhd.append(m_source->height());

				trak += tkhd;
			}
			{
				MP4Atom mdia('mdia');
				{
					{
						MP4Atom mdhd('mdhd');

						// version + flags
						mdhd.append(0u);
						// creation + mod time
						mdhd.append(8, 0);
						// time scale
						mdhd.append(1200000u);
						// duration
						mdhd.append(0u);
						// lang code
						mdhd.append(uint16_t(0x55C4));
						// quality
						mdhd.append(uint16_t(0));

						mdia += mdhd;
					}

					{
						MP4Atom hdlr('hdlr');
						// version + flags
						hdlr.append(0u);
						// component type, ffmpeg uses 0
						hdlr.append("mhlr");
						// component subtype
						hdlr.append("vide");
						// manufacturer + flags + flags mask
						hdlr.append(12, 0);
						hdlr.appendWithNul("VideoHandler");

						mdia += hdlr;
					}
					{
						MP4Atom minf('minf');
						{
							MP4Atom vmhd('vmhd');

							// version + flags
							vmhd.append(1u);
							// gfx mode + color
							vmhd.append(0u);

							minf += vmhd;
						}
						{
							MP4Atom dinf('dinf');
							{
								MP4Atom dref('dref');

								// version + flags
								dref.append(0u);
								// # of entries
								dref.append(1u);

								// ref size
								dref.append(12u);
								// type
								dref.append("url ");
								// version + flags
								dref.append(1u);

								dinf += dref;
							}
							minf += dinf;
						}
						{
							MP4Atom stbl('stbl');
							{
								MP4Atom stsd('stsd');
								// version + flags
								stsd.append(0u);
								// num of entries
								stsd.append(1u);
								{
									MP4Atom avc1('avc1');
									
									avc1.append(6, 0); // reserved
									avc1.append(uint16_t(1)); // data ref idx
									avc1.append(0u); // version + revision
									avc1.append(0u); // vendor
									avc1.append(0u); // temporal qty
									avc1.append(0u); // spacial qty
									avc1.append(uint16_t(m_source->width()));
									avc1.append(uint16_t(m_source->height()));

									avc1.append(0x480000u); // horizontal res
									avc1.append(0x480000u); // vertical res
									avc1.append(0u); // entry data size
									avc1.append(uint16_t(1)); // frames per sample
									avc1.append(32, 0); // compressor name
									avc1.append(uint16_t(24)); // bit depth
									avc1.append(uint16_t(0xffff)); // color table index

									{
										MP4Atom avcc('avcC');
										// http://aviadr1.blogspot.com/2010/05/h264-extradata-partially-explained-for.html

										auto sps = m_source->spsUnit();
										avcc.append(1, 1); // version
										avcc.append(sps.substr(1, 3)); // profile, compatibility, level
										avcc.append(1, char(0xfc | 3));
										avcc.append(1, char(0xe0 | 1));

										avcc.append(uint16_t(sps.length()));
										avcc.append(sps);

										auto pps = m_source->ppsUnit();
										avcc.append(1, 1); // PPS count
										avcc.append(uint16_t(pps.length()));
										avcc.append(pps);

										avc1 += avcc;
									}
									stsd += avc1;
								}
								stbl += stsd;
							}
							{
								MP4Atom stts('stts');
								stts.append(8, 0);
								stbl += stts;
							}
							{
								MP4Atom stsc('stsc');
								stsc.append(8, 0);
								stbl += stsc;
							}
							{
								MP4Atom stsz('stsz');
								stsz.append(12, 0);
								stbl += stsz;
							}
							{
								MP4Atom stco('stco');
								stco.append(8, 0);
								stbl += stco;
							}

							minf += stbl;
						}
						mdia += minf;
					}
				}

				trak += mdia;
			}
			moov += trak;
		}
		
		{
			MP4Atom mvex('mvex');
			{
				MP4Atom trex('trex');
				trex.append(0u);
				trex.append(1u);
				trex.append(1u);
				trex.append(12, 0);
				mvex += trex;
			}

			moov += mvex;
		}
		/*
		{
			MP4Atom udta('udta');
			// TODO
			moov += udta;
		}
		*/

		m_target->write(moov);
		m_bytesWritten += moov.length();
	}

	// TODO: Loop of moof+mdat
	// moof
	{
		MP4Atom moof('moof');

		// mfhd
		{
			MP4Atom mfhd('mfhd');
			mfhd.append(0u);
			mfhd.append(1u); // fragment number
			moof += mfhd;
		}

		// traf
		{
			MP4Atom traf('traf');

			// tfhd
			{
				MP4Atom tfhd('tfhd');
				// base data offset present
				// def sample duration present
				// def sample size present
				// def sample flags present
				tfhd.append(0x39u); // flags
				tfhd.append(1u); // track ID
				tfhd.append(m_bytesWritten); // base data offset - amt of data previously written
				tfhd.append(48000u); // sample duration
				tfhd.append(0x0b00u); // sample size
				tfhd.append(0x01010000u); // sample flags
				traf += tfhd;
			}
			{
				MP4Atom tfdt('tfdt');
				tfdt.append(12, 0);
				traf += tfdt;
			}
			{
				MP4Atom trun('trun');
				// TODO: track run box
				traf += trun;
			}
			moof += traf;
		}

		m_target->write(moof);
	}
	// TODO: mdat...
}
