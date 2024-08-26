// drumkv1_sample.cpp
//
/****************************************************************************
   Copyright (C) 2012-2024, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#include "drumkv1_sample.h"

#include "drumkv1_resampler.h"

#include <sndfile.h>


//-------------------------------------------------------------------------
// drumkv1_sample - sampler wave table.
//

// ctor.
drumkv1_sample::drumkv1_sample ( float srate )
	: m_srate(srate), m_filename(nullptr), m_nchannels(0),
		m_rate0(0.0f), m_freq0(1.0f), m_ratio(0.0f),
		m_nframes(0), m_pframes(nullptr), m_reverse(false),
		m_offset(false), m_offset_start(0), m_offset_end(0),
		m_offset_phase0(0.0f), m_offset_end2(0)
{
}


// copy-ctor.
drumkv1_sample::drumkv1_sample ( const drumkv1_sample& sample )
	: drumkv1_sample(sample.m_srate)
{
	m_offset  = sample.m_offset;
	m_reverse = sample.m_reverse;
}


// dtor.
drumkv1_sample::~drumkv1_sample (void)
{
	close();
}


// init.
bool drumkv1_sample::open ( const char *filename, float freq0 )
{
	if (filename == nullptr)
		return false;

	const bool same_filename
		= (m_filename && ::strcmp(m_filename, filename) == 0);

	char *filename2 = ::strdup(filename);

	close();

	if (!same_filename)
		setOffsetRange(0, 0);

	m_filename = filename2;

	SF_INFO info;
	::memset(&info, 0, sizeof(info));
	
	SNDFILE *file = ::sf_open(m_filename, SFM_READ, &info);
	if (file == nullptr)
		return false;

	m_nchannels = info.channels;
	m_rate0     = float(info.samplerate);
	m_nframes   = info.frames;

	float *buffer = new float [m_nchannels * m_nframes];

	const int nread = ::sf_readf_float(file, buffer, m_nframes);
	if (nread > 0) {
		// resample start...
		const uint32_t ninp = uint32_t(nread);
		const uint32_t rinp = uint32_t(m_rate0);
		const uint32_t rout = uint32_t(m_srate);
		if (rinp != rout) {
			drumkv1_resampler resampler;
			const uint32_t nout = uint32_t(float(ninp) * m_srate / m_rate0);
			const uint32_t FILTSIZE = 32; // resample medium quality
			if (resampler.setup(rinp, rout, m_nchannels, FILTSIZE)) {
				float *inpb = buffer;
				float *outb = new float [m_nchannels * nout];
				resampler.inp_count = ninp;
				resampler.inp_data  = inpb;
				resampler.out_count = nout;
				resampler.out_data  = outb;
				resampler.process();
				buffer = outb;
				delete [] inpb;
				// identical rates now...
				m_rate0 = float(rout);
				m_nframes = (nout - resampler.out_count);
			}
		}
		else m_nframes = ninp;
		// resample end.
	}

	const uint32_t nsize = m_nframes + 4;
	m_pframes = new float * [m_nchannels];
	for (uint16_t k = 0; k < m_nchannels; ++k) {
		m_pframes[k] = new float [nsize];
		::memset(m_pframes[k], 0, nsize * sizeof(float));
	}

	uint32_t i = 0;
	for (uint32_t j = 0; j < m_nframes; ++j) {
		for (uint16_t k = 0; k < m_nchannels; ++k)
			m_pframes[k][j] = buffer[i++];
	}

	delete [] buffer;
	::sf_close(file);

	if (m_reverse)
		reverse_sync();

	reset(freq0);

	updateOffset();
	return true;
}


void drumkv1_sample::close (void)
{
	if (m_pframes) {
		for (uint16_t k = 0; k < m_nchannels; ++k)
			delete [] m_pframes[k];
		delete [] m_pframes;
		m_pframes = nullptr;
	}

	m_nframes   = 0;
	m_ratio     = 0.0f;
	m_freq0     = 1.0f;
	m_rate0     = 0.0f;
	m_nchannels = 0;

//	setOffsetRange(0, 0);

	if (m_filename) {
		::free(m_filename);
		m_filename = nullptr;
	}
}


// reverse sample buffer.
void drumkv1_sample::reverse_sync (void)
{
	if (m_nframes > 0 && m_pframes) {
		const uint32_t nsize1 = (m_nframes - 1);
		const uint32_t nsize2 = (m_nframes >> 1);
		for (uint16_t k = 0; k < m_nchannels; ++k) {
			float *frames = m_pframes[k];
			for (uint32_t i = 0; i < nsize2; ++i) {
				const uint32_t j = nsize1 - i;
				const float sample = frames[i];
				frames[i] = frames[j];
				frames[j] = sample;
			}
		}
	}
}


// offset range.
void drumkv1_sample::setOffsetRange ( uint32_t start, uint32_t end )
{
	if (start > m_nframes)
		start = m_nframes;

	if (end > m_nframes || start >= end)
		end = m_nframes;

	if (start < end) {
		m_offset_start = start;
		m_offset_end = end;
	} else {
		m_offset_start = 0;
		m_offset_end = m_nframes;
	}

	if (m_offset && m_offset_start < m_offset_end) {
		m_offset_phase0 = float(zero_crossing(m_offset_start, nullptr));
		m_offset_end2 = zero_crossing(m_offset_end, nullptr);
	} else {
		m_offset_phase0 = 0.0f;
		m_offset_end2 = m_nframes;
	}
}


// offset updater.
void drumkv1_sample::updateOffset (void)
{
	setOffsetRange(m_offset_start, m_offset_end);
}


// zero-crossing aliasing (all channels).
uint32_t drumkv1_sample::zero_crossing ( uint32_t i, int *slope ) const
{
	const int s0 = (slope ? *slope : 0);

	if (i > 0) --i;
	float v0 = zero_crossing_k(i);
	for (++i; i < m_nframes; ++i) {
		const float v1 = zero_crossing_k(i);
		if ((0 >= s0 && v0 >= 0.0f && 0.0f >= v1) ||
			(s0 >= 0 && v1 >= 0.0f && 0.0f >= v0)) {
			if (slope && s0 == 0) *slope = (v1 < v0 ? -1 : +1);
			return i;
		}
		v0 = v1;
	}

	return m_nframes;
}


// zero-crossing aliasing (median).
float drumkv1_sample::zero_crossing_k ( uint32_t i ) const
{
	float sum = 0.0f;
	for (uint16_t k = 0; k < m_nchannels; ++k)
		sum += m_pframes[k][i];
	return (sum / float(m_nchannels));
}


// end of drumkv1_sample.cpp
