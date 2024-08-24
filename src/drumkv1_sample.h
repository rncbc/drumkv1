// drumkv1_sample.h
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

#ifndef __drumkv1_sample_h
#define __drumkv1_sample_h

#include <cstdint>

#include <cstdlib>
#include <cstring>


// forward decls.
class drumkv1;


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
			reverse_sync();
		}
	}

	bool isReverse() const
		{ return m_reverse; }

	// offset mode.
	void setOffset(bool offset)
	{
		m_offset = offset;

		updateOffset();
	}

	bool isOffset() const
		{ return m_offset; }

	// sample start/end points (offsets)
	void setOffsetRange(uint32_t start, uint32_t end);

	uint32_t offsetStart() const
		{ return m_offset_start; }
	uint32_t offsetEnd() const
		{ return m_offset_end; }

	float offsetPhase0() const
		{ return (m_offset ? m_offset_phase0 : 0.0f); }

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
	bool isOver(uint32_t index) const
		{ return !m_pframes || (index >= m_offset_end2); }

protected:

	// reverse sample buffer.
	void reverse_sync();

	// zero-crossing aliasing .
	uint32_t zero_crossing(uint32_t i, int *slope) const;
	float zero_crossing_k(uint32_t i) const;

	// offset updater.
	void updateOffset();

private:

	// instance variables.
	float    m_srate;
	char    *m_filename;
	uint16_t m_nchannels;
	float    m_rate0;
	float    m_freq0;
	float    m_ratio;
	uint32_t m_nframes;
	float  **m_pframes;
	bool     m_reverse;

	bool     m_offset;
	uint32_t m_offset_start;
	uint32_t m_offset_end;
	float    m_offset_phase0;
	uint32_t m_offset_end2;
};


//-------------------------------------------------------------------------
// drumkv1_generator - sampler oscillator (sort of:)

class drumkv1_generator
{
public:

	// ctor.
	drumkv1_generator(drumkv1_sample *sample = nullptr) { reset(sample); }

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
		m_phase = (m_sample ? m_sample->offsetPhase0() : 0.0f);
		m_index = 0;
		m_alpha = 0.0f;
	}

	// iterate.
	void next(float freq)
	{
		const float delta = freq * (m_sample ? m_sample->ratio() : 1.0f);

		m_index  = uint32_t(m_phase);
		m_alpha  = m_phase - float(m_index);
		m_phase += delta;
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
		{ return (m_sample ? m_sample->isOver(m_index) : true); }

private:

	// iterator variables.
	drumkv1_sample *m_sample;

	float    m_phase;
	uint32_t m_index;
	float    m_alpha;
};


//-------------------------------------------------------------------------
// drumkv1_sample_ref - PADsynth wave table (sample reference lists).
//
#include "drumkv1_list.h"

class drumkv1_sample_ref
{
public:

	// dtor.
	~drumkv1_sample_ref()
		{ clear_refs(true); }

	// methods.
	void append(drumkv1_sample *sample)
		{ m_play.append(new sample_ref(sample)); }

	drumkv1_sample *next() const
		{ return m_play.next()->refp; }
	drumkv1_sample *prev() const
		{ return m_play.prev()->refp; }

	void acquire()
		{ ++(m_play.next()->refc); }
	void release()
		{ --(m_play.next()->refc); free_refs(); }

	void free_refs()
	{
		sample_ref *ref = m_play.next();
		while (ref && ref->refc == 0 && ref != m_play.prev()) {
			m_play.remove(ref);
			m_free.append(ref);
			ref = m_play.next();
		}
	}

	void clear_refs(bool force = false)
	{
		sample_ref *ref;
		if (force) {
			ref = m_play.next();
			while (ref) {
				m_play.remove(ref);
				m_free.append(ref);
				ref = m_play.next();
			}
		}
		ref = m_free.next();
		while (ref) {
			m_free.remove(ref);
			delete ref->refp;
			delete ref;
			ref = m_free.next();
		}
	}

private:

	struct sample_ref : public drumkv1_list<sample_ref>
	{
		sample_ref(drumkv1_sample *sample) : refp(sample), refc(0) {}

		drumkv1_sample *refp;
		uint32_t        refc;
	};

	drumkv1_list<sample_ref> m_play;
	drumkv1_list<sample_ref> m_free;
};


#endif	// __drumkv1_sample_h

// end of drumkv1_sample.h
