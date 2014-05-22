// drumkv1_sample.h
//
/****************************************************************************
   Copyright (C) 2012-2014, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __drumkv1_sample_h
#define __drumkv1_sample_h

#include <stdint.h>

#include <stdlib.h>
#include <string.h>

// forward decls.
class drumkv1_reverse_sched;


//-------------------------------------------------------------------------
// drumkv1_sample - sampler wave table.
//

class drumkv1_sample
{
public:

	// ctor.
	drumkv1_sample(float srate = 44100.0f);

	// dtor.
	~drumkv1_sample();

	// nominal sample-rate.
	void setSampleRate(float srate)
		{ m_srate = srate; }
	float sampleRate() const
		{ return m_srate; }

	// reverse mode.
	void setReverse (bool reverse)
	{
		if (( m_reverse && !reverse) ||
			(!m_reverse &&  reverse)) {
			m_reverse = reverse;
			reverse_sample();
		}
	}

	bool isReverse() const
		{ return m_reverse; }

	// schedule sample reverse.
	void reverse_test(bool reverse)
	{
		if (( m_reverse && !reverse) ||
			(!m_reverse &&  reverse)) {
			reverse_sched(reverse);
		}
	}

	void reverse_sched(bool reverse);

	// init.
	bool open(const char *filename, float freq0 = 1.0f);
	void close();

	// accessors.
	const char *filename() const
		{ return m_filename; }
	uint16_t channels() const
		{ return m_nchannels; }
	float rate() const
		{ return m_rate0; }
	float freq() const
		{ return m_freq0; }
	uint32_t length() const
		{ return m_nframes; }

	// resampler ratio
	float ratio() const
		{ return m_ratio; }

	// reset.
	void reset(float freq0)
	{
		m_freq0 = freq0;
		m_ratio = m_rate0 / (m_freq0 * m_srate);
	}

	// frame value.
	float *frames(uint16_t k) const
		{ return m_pframes[k]; }

	// predicate.
	bool isOver(uint32_t frame) const
		{ return !m_pframes || (frame >= m_nframes); }

protected:

	// reverse sample buffer.
	void reverse_sample();

private:

	float    m_srate;
	char    *m_filename;
	uint16_t m_nchannels;
	float    m_rate0;
	float    m_freq0;
	float    m_ratio;
	uint32_t m_nframes;
	float  **m_pframes;
	bool     m_reverse;

	drumkv1_reverse_sched *m_reverse_sched;
};


//-------------------------------------------------------------------------
// drumkv1_generator - sampler oscillator (sort of:)

class drumkv1_generator
{
public:

	// ctor.
	drumkv1_generator(drumkv1_sample *sample = 0) { reset(sample); }

	// sample accessor.
	drumkv1_sample *sample() const
		{ return m_sample; }

	// reset.
	void reset(drumkv1_sample *sample)
	{
		m_sample = sample;

		start();
	}

	// begin.
	void start(void)
	{
		m_phase = 0.0f;
		m_index = 0;
		m_alpha = 0.0f;
		m_frame = 0;
	}

	// iterate.
	void next(float freq)
	{
		const float delta = freq * m_sample->ratio();

		m_index  = int(m_phase);
		m_alpha  = m_phase - float(m_index);
		m_phase += delta;

		if (m_frame < m_index)
			m_frame = m_index;
	}

	// sample.
	float value(uint16_t k) const
	{
		if (isOver())
			return 0.0f;

		const float *frames = m_sample->frames(k);

		const float x0 = frames[m_index];
		const float x1 = frames[m_index + 1];
		const float x2 = frames[m_index + 2];
		const float x3 = frames[m_index + 3];

		const float c1 = (x2 - x0) * 0.5f;
		const float b1 = (x1 - x2);
		const float b2 = (c1 + b1);
		const float c3 = (x3 - x1) * 0.5f + b2 + b1;
		const float c2 = (c3 + b2);

		return (((c3 * m_alpha) - c2) * m_alpha + c1) * m_alpha + x1;
	}

	// predicate.
	bool isOver() const
		{ return m_sample->isOver(m_frame); }

private:

	// iterator variables.
	drumkv1_sample *m_sample;

	float    m_phase;
	uint32_t m_index;
	float    m_alpha;
	uint32_t m_frame;
};


#endif	// __drumkv1_sample_h

// end of drumkv1_sample.h
