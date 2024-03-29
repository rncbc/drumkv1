// drumkv1_wave.h
//
/****************************************************************************
   Copyright (C) 2012-2023, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __drumkv1_wave_h
#define __drumkv1_wave_h

#include <cstdint>



//-------------------------------------------------------------------------
// drumkv1_wave - smoothed (integrating oversampled) wave table.
//

class drumkv1_wave
{
public:

	// shape.
	enum Shape { Pulse = 0, Saw, Sine, Rand, Noise };

	// ctor.
	drumkv1_wave(uint32_t nsize = 4096, uint16_t nover = 24);

	// dtor.
	~drumkv1_wave();

	// properties.
	Shape shape() const
		{ return m_shape; }
	float width() const
		{ return m_width; }

	// sample rate.
	void setSampleRate(float srate)
		{ m_srate = srate; }
	float sampleRate() const
		{ return m_srate; }

	// table size (in frames)
	uint32_t size() const
		{ return m_nsize; }

	// init.
	void reset(Shape shape, float width);

	// init.test
	void reset_test(Shape shape, float width)
	{
		if (shape != m_shape || width != m_width)
			reset(shape, width);
	}

	// begin.
	float start(float& phase, float pshift = 0.0f, float freq = 0.0f) const
	{
		phase = m_phase0 + pshift;
		if (phase >= 1.0f)
			phase -= 1.0f;

		return sample(phase, freq);
	}

	// iterate.
	float sample(float& phase, float freq) const
	{
		const float index = phase * float(m_nsize);
		const uint32_t i = uint32_t(index);
		const float alpha = index - float(i);

		phase += freq / m_srate;
		if (phase >= 1.0f)
			phase -= 1.0f;

		// cubic interpolation...
		const float x0 = m_table[i];
		const float x1 = m_table[i + 1];
#if 0	// cubic interp.
		const float x2 = m_table[i + 2];
		const float x3 = m_table[i + 3];

		const float c1 = (x2 - x0) * 0.5f;
		const float b1 = (x1 - x2);
		const float b2 = (c1 + b1);
		const float c3 = (x3 - x1) * 0.5f + b2 + b1;
		const float c2 = (c3 + b2);

		return (((c3 * alpha) - c2) * alpha + c1) * alpha + x1;
#else	// linear interp.
		return x0 + alpha * (x1 - x0);
#endif
	}

	// absolute value.
	float value(float phase) const
	{
		phase += m_phase0;
		if (phase >= 1.0f)
			phase -= 1.0f;

		return m_table[uint32_t(phase * float(m_nsize))];
	}

protected:

	// init pulse table.
	void reset_pulse();

	// init saw table.
	void reset_saw();

	// init sine table.
	void reset_sine();

	// init random table.
	void reset_rand();

	// init noise table.
	void reset_noise();

	// post-processors
	void reset_filter();
	void reset_normalize();
	void reset_interp();

	// Hal Chamberlain's pseudo-random linear congruential method.
	float pseudo_randf ()
	{
		m_srand = (m_srand * 196314165) + 907633515;
		return m_srand / float(INT32_MAX) - 1.0f;
	}

private:

	uint32_t m_nsize;
	uint16_t m_nover;

	Shape    m_shape;
	float    m_width;

	float    m_srate;
	float   *m_table;
	float    m_phase0;

	uint32_t m_srand;
};


//-------------------------------------------------------------------------
// drumkv1_wave_lf - hard/non-smoothed wave table (eg. LFO).
//

class drumkv1_wave_lf : public drumkv1_wave
{
public:

	// ctor.
	drumkv1_wave_lf(uint32_t nsize = 1024)
		: drumkv1_wave(nsize, 0) {}
};


//-------------------------------------------------------------------------
// drumkv1_oscillator - wave table oscillator
//

class drumkv1_oscillator
{
public:

	// ctor.
	drumkv1_oscillator(drumkv1_wave *wave = 0) { reset(wave); }

	// wave and phase accessors.
	void reset(drumkv1_wave *wave)
		{ m_wave = wave; m_phase = 0.0f; }

	drumkv1_wave *wave() const
		{ return m_wave; }

	// begin.
	float start(float pshift = 0.0f, float freq = 0.0f)
		{ return m_wave->start(m_phase, pshift, freq); }

	// iterate.
	float sample(float freq)
		{ return m_wave->sample(m_phase, freq); }

private:

	drumkv1_wave *m_wave;

	float m_phase;
};


#endif	// __drumkv1_wave_h

// end of drumkv1_wave.h
