﻿// drumkv1.cpp
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

#include "drumkv1.h"

#include "drumkv1_sample.h"

#include "drumkv1_wave.h"
#include "drumkv1_ramp.h"

#include "drumkv1_list.h"

#include "drumkv1_filter.h"
#include "drumkv1_formant.h"

#include "drumkv1_fx.h"
#include "drumkv1_reverb.h"

#include "drumkv1_config.h"
#include "drumkv1_controls.h"
#include "drumkv1_programs.h"
#include "drumkv1_tuning.h"

#include "drumkv1_sched.h"


#ifdef CONFIG_DEBUG_0
#include <cstdio>
#endif

#include <cstring>


//-------------------------------------------------------------------------
// drumkv1_impl
//
// -- borrowed and revamped from synth.h of synth4
//    Copyright (C) 2007 jorgen, linux-vst.com
//

const uint8_t MAX_VOICES  = 64;			// max polyphony
const uint8_t MAX_NOTES   = 128;
const uint8_t MAX_GROUP   = 128;

const float MIN_ENV_MSECS = 0.5f;		// min 500 usec per stage
const float MAX_ENV_MSECS = 2000.0f;	// max 2 sec per stage (default)

const float COARSE_SCALE  = 12.0f;
const float FINE_SCALE    = 1.0f;
const float SWEEP_SCALE   = 0.5f;
const float PITCH_SCALE   = 0.5f;

const uint8_t MAX_DIRECT_NOTES = (MAX_VOICES >> 2);


// maximum helper

inline float drumkv1_max ( float a, float b )
{
	return (a > b ? a : b);
}


// hyperbolic-tangent fast approximation

inline float drumkv1_tanhf ( const float x )
{
	const float x2 = x * x;
	return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}


// sigmoids

inline float drumkv1_sigmoid ( const float x )
{
//	return 2.0f / (1.0f + ::expf(-5.0f * x)) - 1.0f;
	return drumkv1_tanhf(2.0f * x);
}

inline float drumkv1_sigmoid_0 ( const float x, const float t0 )
{
	const float t1 = 1.0f - t0;
#if 0
	if (x > +t1)
		return +t1 + t0 * drumkv1_tanhf(+(x - t1) / t0);
	else
	if (x < -t1)
		return -t1 - t0 * drumkv1_tanhf(-(x + t1) / t0);
	else
		return x;
#else
	return (x < -1.0f ? -t1 : (x > +1.0f ? t1 : t1 * x * (1.5f - 0.5f * x * x)));
#endif
}

inline float drumkv1_sigmoid_1 ( const float x, const float t0 = 0.01f )
{
	return 0.5f * (1.0f + drumkv1_sigmoid_0(2.0f * x - 1.0f, t0));
}


// velocity hard-split curve

inline float drumkv1_velocity ( const float x, const float p = 0.2f )
{
	return ::powf(x, (1.0f - p));
}


// pitchbend curve

inline float drumkv1_pow2f ( const float x )
{
// simplest power-of-2 straight linearization
// -- x argument valid in [-1, 1] interval
//	return 1.0f + (x < 0.0f ? 0.5f : 1.0f) * x;
	return ::powf(2.0f, x);
}


// convert note to frequency (hertz)

inline float drumkv1_freq2 ( float delta )
{
	return ::powf(2.0f, delta / 12.0f);
}

inline float drumkv1_freq ( int note )
{
	return (440.0f / 32.0f) * drumkv1_freq2(float(note - 9));
}


// parameter port (basic)

class drumkv1_port
{
public:

	drumkv1_port() : m_port(nullptr), m_value(0.0f), m_vport(0.0f) {}

	virtual ~drumkv1_port() {}

	void set_port(float *port)
		{ m_port = port; }
	float *port() const
		{ return m_port; }

	virtual void set_value(float value)
		{ m_value = value; if (m_port) m_vport = *m_port; }

	float value() const
		{ return m_value; }
	float *value_ptr()
		{ tick(1); return &m_value; }

	virtual float tick(uint32_t /*nstep*/)
	{
		if (m_port && ::fabsf(*m_port - m_vport) > 0.001f)
			set_value(*m_port);

		return m_value;
	}

	float operator *()
		{ return tick(1); }

private:

	float *m_port;
	float  m_value;
	float  m_vport;
};


// parameter port (smoothed)

class drumkv1_port2 : public drumkv1_port
{
public:

	drumkv1_port2() : m_vtick(0.0f), m_vstep(0.0f), m_nstep(0) {}

	static const uint32_t NSTEP = 32;

	void set_value(float value)
	{
		m_vtick = drumkv1_port::value();

		m_nstep = NSTEP;
		m_vstep = (value - m_vtick) / float(m_nstep);

		drumkv1_port::set_value(value);
	}

	float tick(uint32_t nstep)
	{
		if (m_nstep == 0)
			return drumkv1_port::tick(nstep);

		if (m_nstep >= nstep) {
			m_vtick += m_vstep * float(nstep);
			m_nstep -= nstep;
		} else {
			m_vtick += m_vstep * float(m_nstep);
			m_nstep  = 0;
		}

		return m_vtick;
	}

private:

	float    m_vtick;
	float    m_vstep;
	uint32_t m_nstep;
};


// parameter port (scheduled/detached)

class drumkv1_port3_sched : public drumkv1_sched
{
public:

	// ctor.
	drumkv1_port3_sched(drumkv1 *pDrumk, int key)
		: drumkv1_sched(pDrumk, drumkv1_sched::Controller), m_key(key) {}

	// (pure) virtual prober.
	virtual float probe(int sid) const = 0;

	// key element accessor.
	int key() const { return m_key; }

protected:

	// instance variables.
	int m_key;
};


class drumkv1_port3 : public drumkv1_port
{
public:

	drumkv1_port3(drumkv1_port3_sched *sched, drumkv1::ParamIndex index)
		: m_sched(sched), m_index(index) {}

	void set_value(float value)
	{
		const float v0 = m_sched->probe(m_index);
		const float d0 = ::fabsf(value - v0);

		drumkv1_port::set_value(value);

		if (d0 > 0.001f)
			m_sched->schedule(m_index);
	}

	void set_value_sync(float value)
	{
		drumkv1_port::set_value(value);
	}

private:

	drumkv1_port3_sched *m_sched;
	drumkv1::ParamIndex  m_index;
};


// envelope

struct drumkv1_env
{
	// envelope stages

	enum Stage { Idle = 0, Attack, Decay1, Decay2, End };

	// per voice

	struct State
	{
		// ctor.
		State() : running(false), stage(Idle),
			phase(0.0f), delta(0.0f), value(0.0f),
			c1(1.0f), c0(0.0f), frames(0) {}

		// process
		float tick()
		{
			if (running && frames > 0) {
				phase += delta;
				value = c1 * phase * (2.0f - phase) + c0;
				--frames;
			}
			return value;
		}

		// state
		bool running;
		Stage stage;
		float phase;
		float delta;
		float value;
		float c1, c0;
		uint32_t frames;
	};

	void start(State *p)
	{
		p->running = true;
		p->stage = Attack;
		p->frames = uint32_t(*attack * *attack * max_frames);
		if (p->frames < min_frames1) // prevent click on too fast attack
			p->frames = min_frames1;
		p->phase = 0.0f;
		p->delta = 1.0f / float(p->frames);
		p->value = 0.0f;
		p->c1 = 1.0f;
		p->c0 = 0.0f;
	}

	void next(State *p)
	{
		if (p->stage == Attack) {
			p->stage = Decay1;
			p->frames = uint32_t(*decay1 * *decay1 * max_frames);
			if (p->frames < min_frames2) // prevent click on too fast decay1
				p->frames = min_frames2;
			p->phase = 0.0f;
			p->delta = 1.0f / float(p->frames);
			p->c1 = *level2 - 1.0f;
			p->c0 = p->value;
		}
		else if (p->stage == Decay1) {
			p->stage = Decay2;
			p->frames = uint32_t(*decay2 * *decay2 * max_frames);
			if (p->frames < min_frames2) // prevent click on too fast decay2
				p->frames = min_frames2;
			p->phase = 0.0f;
			p->delta = 1.0f / float(p->frames);
			p->c1 = -(p->value);
			p->c0 = p->value;
		}
		else if (p->stage == Decay2) {
			p->running = false;
			p->stage = End;
			p->frames = 0;
			p->phase = 0.0f;
			p->delta = 0.0f;
			p->value = 0.0f;
			p->c1 = 0.0f;
			p->c0 = 0.0f;
		}
	}

	void note_off(State *p)
	{
		p->running = true;
		p->stage = Decay2;
		p->frames = uint32_t(*decay2 * *decay2 * max_frames);
		if (p->frames < min_frames2) // prevent click on too fast release
			p->frames = min_frames2;
		p->phase = 0.0f;
		p->delta = 1.0f / float(p->frames);
		p->c1 = -(p->value);
		p->c0 = p->value;
	}

	void note_off_fast(State *p)
	{
		p->running = true;
		p->stage = Decay2;
		p->frames = min_frames2;
		p->phase = 0.0f;
		p->delta = 1.0f / float(p->frames);
		p->c1 = -(p->value);
		p->c0 = p->value;
	}

	void idle(State *p)
	{
		p->running = false;
		p->stage = Idle;
		p->frames = 0;
		p->phase = 0.0f;
		p->delta = 0.0f;
		p->value = 1.0f;
		p->c1 = 0.0f;
		p->c0 = 0.0f;
	}

	// parameters

	drumkv1_port attack;
	drumkv1_port decay1;
	drumkv1_port level2;
	drumkv1_port decay2;

	uint32_t min_frames1;
	uint32_t min_frames2;
	uint32_t max_frames;
};


// midi control

struct drumkv1_ctl
{
	drumkv1_ctl() { reset(); }

	void reset()
	{
		pressure = 0.0f;
		pitchbend = 1.0f;
		modwheel = 0.0f;
		panning = 0.0f;
		volume = 1.0f;
		sustain = false;
	}

	float pressure;
	float pitchbend;
	float modwheel;
	float panning;
	float volume;
	bool  sustain;
};


// dco

class drumkv1_gen : public drumkv1_port3_sched
{
public:

	drumkv1_gen(drumkv1 *pDrumk, int key)
		: drumkv1_port3_sched(pDrumk, key),
			reverse(this, drumkv1::GEN1_REVERSE),
			offset(this, drumkv1::GEN1_OFFSET),
			offset_1(this, drumkv1::GEN1_OFFSET_1),
			offset_2(this, drumkv1::GEN1_OFFSET_2) {}

	drumkv1_port  sample;
	drumkv1_port3 reverse;
	drumkv1_port3 offset;
	drumkv1_port3 offset_1;
	drumkv1_port3 offset_2;
	drumkv1_port  group;
	drumkv1_port  coarse;
	drumkv1_port  fine;
	drumkv1_port  envtime;

	float sample0, envtime0;

protected:

	float probe(int sid) const
	{
		float ret = 0.0f;
		const int key = drumkv1_port3_sched::key();
		drumkv1 *pDrumk = drumkv1_port3_sched::instance();
		drumkv1_element *element = pDrumk->element(key);
		if (element)
		switch (drumkv1::ParamIndex(sid)) {
		case drumkv1::GEN1_REVERSE:
			ret = (element->isReverse() ? 1.0f : 0.0f);
			break;
		case drumkv1::GEN1_OFFSET:
			ret = (element->isOffset() ? 1.0f : 0.0f);
			break;
		case drumkv1::GEN1_OFFSET_1: {
			const uint32_t iSampleLength
				= element->sample()->length();
			const uint32_t iOffsetStart
				= element->offsetStart();
			ret = (iSampleLength > 0
				? float(iOffsetStart) / float(iSampleLength)
				: 0.0f);
			break;
		}
		case drumkv1::GEN1_OFFSET_2: {
			const uint32_t iSampleLength
				= element->sample()->length();
			const uint32_t iOffsetEnd
				= element->offsetEnd();
			ret = (iSampleLength > 0
				? float(iOffsetEnd) / float(iSampleLength)
				: 1.0f);
			break;
		}
		default:
			break;
		}

		return ret;
	}

	void process(int sid)
	{
		const int key = drumkv1_port3_sched::key();
		drumkv1 *pDrumk = drumkv1_port3_sched::instance();
		drumkv1_element *element = pDrumk->element(key);
		if (element)
		switch (drumkv1::ParamIndex(sid)) {
		case drumkv1::GEN1_REVERSE:
			element->setReverse(reverse.value() > 0.5f);
			element->sampleReverseSync();
			break;
		case drumkv1::GEN1_OFFSET:
			element->setOffset(offset.value() > 0.5f);
			element->sampleOffsetSync();
			break;
		case drumkv1::GEN1_OFFSET_1:
			if (element->isOffset()) {
				const uint32_t iSampleLength
					= element->sample()->length();
				const uint32_t iOffsetEnd
					= element->offsetEnd();
				uint32_t iOffsetStart
					= uint32_t(offset_1.value() * float(iSampleLength));
				if (iOffsetStart >= iOffsetEnd)
					iOffsetStart  = iOffsetEnd - 1;
				element->setOffsetRange(iOffsetStart, iOffsetEnd);
				element->sampleOffsetRangeSync();
				element->updateEnvTimes();
			}
			break;
		case drumkv1::GEN1_OFFSET_2:
			if (element->isOffset()) {
				const uint32_t iSampleLength
					= element->sample()->length();
				const uint32_t iOffsetStart
					= element->offsetStart();
				uint32_t iOffsetEnd
					= uint32_t(offset_2.value() * float(iSampleLength));
				if (iOffsetStart >= iOffsetEnd)
					iOffsetEnd = iOffsetStart + 1;
				element->setOffsetRange(iOffsetStart, iOffsetEnd);
				element->sampleOffsetRangeSync();
				element->updateEnvTimes();
			}
			break;
		default:
			break;
		}
		// Sync current sample...
		if (pDrumk->currentElement() == key)
			pDrumk->updateSample();
	}
};


// dcf

struct drumkv1_dcf
{
	drumkv1_port  enabled;
	drumkv1_port2 cutoff;
	drumkv1_port2 reso;
	drumkv1_port  type;
	drumkv1_port  slope;
	drumkv1_port2 envelope;

	drumkv1_env   env;
};


// lfo

struct drumkv1_lfo
{
	drumkv1_port  enabled;
	drumkv1_port  shape;
	drumkv1_port  width;
	drumkv1_port2 bpm;
	drumkv1_port2 rate;
	drumkv1_port2 sweep;
	drumkv1_port2 pitch;
	drumkv1_port2 cutoff;
	drumkv1_port2 reso;
	drumkv1_port2 panning;
	drumkv1_port2 volume;

	drumkv1_env   env;
};


// dca

struct drumkv1_dca
{
	drumkv1_port enabled;
	drumkv1_port volume;

	drumkv1_env  env;
};



// def (ranges)

struct drumkv1_def
{
	drumkv1_port pitchbend;
	drumkv1_port modwheel;
	drumkv1_port pressure;
	drumkv1_port velocity;
	drumkv1_port channel;
	drumkv1_port noteoff;
};


// out (mix)

struct drumkv1_out
{
	drumkv1_port width;
	drumkv1_port panning;
	drumkv1_port fxsend;
	drumkv1_port volume;
};


// chorus (fx)

struct drumkv1_cho
{
	drumkv1_port wet;
	drumkv1_port delay;
	drumkv1_port feedb;
	drumkv1_port rate;
	drumkv1_port mod;
};


// flanger (fx)

struct drumkv1_fla
{
	drumkv1_port wet;
	drumkv1_port delay;
	drumkv1_port feedb;
	drumkv1_port daft;
};


// phaser (fx)

struct drumkv1_pha
{
	drumkv1_port wet;
	drumkv1_port rate;
	drumkv1_port feedb;
	drumkv1_port depth;
	drumkv1_port daft;
};


// delay (fx)

struct drumkv1_del
{
	drumkv1_port wet;
	drumkv1_port delay;
	drumkv1_port feedb;
	drumkv1_port bpm;
};


// reverb

struct drumkv1_rev
{
	drumkv1_port wet;
	drumkv1_port room;
	drumkv1_port damp;
	drumkv1_port feedb;
	drumkv1_port width;
};


// dynamic(compressor/limiter)

struct drumkv1_dyn
{
	drumkv1_port compress;
	drumkv1_port limiter;
};


// balance smoother (1 parameters)

class drumkv1_bal1 : public drumkv1_ramp1
{
public:

	drumkv1_bal1() : drumkv1_ramp1(2) {}

protected:

	float evaluate(uint16_t i)
	{
		drumkv1_ramp1::update();

		const float wbal = 0.25f * M_PI
			* (1.0f + m_param1_v);

		return M_SQRT2 * (i & 1 ? ::sinf(wbal) : ::cosf(wbal));
	}
};


// balance smoother (2 parameters)

class drumkv1_bal2 : public drumkv1_ramp2
{
public:

	drumkv1_bal2() : drumkv1_ramp2(2) {}

protected:

	float evaluate(uint16_t i)
	{
		drumkv1_ramp2::update();

		const float wbal = 0.25f * M_PI
			* (1.0f + m_param1_v)
			* (1.0f + m_param2_v);

		return M_SQRT2 * (i & 1 ? ::sinf(wbal) : ::cosf(wbal));
	}
};



// pressure smoother (3 parameters)

class drumkv1_pre : public drumkv1_ramp3
{
public:

	drumkv1_pre() : drumkv1_ramp3() {}

protected:

	float evaluate(uint16_t)
	{
		drumkv1_ramp3::update();

		return m_param1_v * drumkv1_max(m_param2_v, m_param3_v);
	}
};


// common phasor (LFO sync)

class drumkv1_phasor
{
public:

	drumkv1_phasor(uint32_t nsize = 1024)
		: m_nsize(nsize), m_nframes(0) {}

	void process(uint32_t nframes)
	{
		m_nframes += nframes;
		while (m_nframes >= m_nsize)
			m_nframes -= m_nsize;
	}

	float pshift() const
		{ return float(m_nframes) / float(m_nsize); }

private:

	uint32_t m_nsize;
	uint32_t m_nframes;
};


// synth element

class drumkv1_elem : public drumkv1_list<drumkv1_elem>
{
public:

	drumkv1_elem(drumkv1 *pDrumk, float srate, int key);

	drumkv1_element element;

	void midiInEnabled(bool on);
	uint32_t midiInCount();

	drumkv1_sample  gen1_sample;
	drumkv1_wave_lf lfo1_wave;

	drumkv1_formant::Impl dcf1_formant;

	drumkv1_gen    gen1;
	drumkv1_dcf    dcf1;
	drumkv1_lfo    lfo1;
	drumkv1_dca    dca1;
	drumkv1_out    out1;

	drumkv1_ramp1  wid1;
	drumkv1_bal2   pan1;
	drumkv1_ramp3  vol1;

	float params[3][drumkv1::NUM_ELEMENT_PARAMS];

	void updateEnvTimes(float srate);
};


// synth element

drumkv1_elem::drumkv1_elem ( drumkv1 *pDrumk, float srate, int key )
	: element(this), gen1_sample(srate), gen1(pDrumk, key)
{
	// element parameter port/value set
	for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i) {
		drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
		for (int j = 0; j < 3; ++j)
			params[j][i] = drumkv1_param::paramDefaultValue(index);
		element.setParamPort(index, &(params[1][i]));
	}

	// element key (sample note)
	gen1.sample0 = float(key);

	// max env. stage length (default)
	gen1.envtime0 = 0.0001f * MAX_ENV_MSECS;

	for (int j = 0; j < 3; ++j) {
		params[j][drumkv1::GEN1_SAMPLE]  = gen1.sample0;
		params[j][drumkv1::GEN1_ENVTIME] = gen1.envtime0;
	}

	// element sample rate
	gen1_sample.setSampleRate(srate);
	lfo1_wave.setSampleRate(srate);

	updateEnvTimes(srate);

	dcf1_formant.setSampleRate(srate);
}


void drumkv1_elem::updateEnvTimes ( float srate )
{
	// element envelope range times in frames
	const float srate_ms = 0.001f * srate;

	float envtime_msecs = 10000.0f * gen1.envtime0;
	if (envtime_msecs < MIN_ENV_MSECS) {
		const uint32_t envtime_frames
			= (gen1_sample.offsetEnd() - gen1_sample.offsetStart()) >> 1;
		envtime_msecs = envtime_frames / srate_ms;
	}
	if (envtime_msecs < MIN_ENV_MSECS)
		envtime_msecs = MIN_ENV_MSECS * 4.0f;

	const uint32_t min_frames1 = uint32_t(srate_ms * MIN_ENV_MSECS);
	const uint32_t min_frames2 = (min_frames1 << 2);
	const uint32_t max_frames  = uint32_t(srate_ms * envtime_msecs);

	dcf1.env.min_frames1 = min_frames1;
	dcf1.env.min_frames2 = min_frames2;
	dcf1.env.max_frames  = max_frames;

	lfo1.env.min_frames1 = min_frames1;
	lfo1.env.min_frames2 = min_frames2;
	lfo1.env.max_frames  = max_frames;

	dca1.env.min_frames1 = min_frames1;
	dca1.env.min_frames2 = min_frames2;
	dca1.env.max_frames  = max_frames;
}


// voice

struct drumkv1_voice : public drumkv1_list<drumkv1_voice>
{
	drumkv1_voice(drumkv1_elem *pElem = nullptr);

	void reset(drumkv1_elem *pElem)
	{
		elem = pElem;

		gen1.reset(pElem ? &pElem->gen1_sample : nullptr);
		lfo1.reset(pElem ? &pElem->lfo1_wave : nullptr);

		dcf17.reset(pElem ? &pElem->dcf1_formant : nullptr);
		dcf18.reset(pElem ? &pElem->dcf1_formant : nullptr);
	}

	drumkv1_elem *elem;

	int note;									// voice note
	int group;									// voice group

	float vel;									// key velocity
	float pre;									// key pressure/after-touch

	drumkv1_generator  gen1;
	drumkv1_oscillator lfo1;

	float gen1_freq;							// frequency and phase

	float lfo1_sample;

	drumkv1_filter1 dcf11, dcf12;				// filters
	drumkv1_filter2 dcf13, dcf14;
	drumkv1_filter3 dcf15, dcf16;
	drumkv1_formant dcf17, dcf18;

	drumkv1_env::State dca1_env;				// envelope states
	drumkv1_env::State dcf1_env;
	drumkv1_env::State lfo1_env;

	drumkv1_pre dca1_pre;

	float out1_panning;
	float out1_volume;

	drumkv1_bal1  out1_pan;						// output panning
	drumkv1_ramp1 out1_vol;						// output volume

	bool sustain;
};


// MIDI input asynchronous status notification

class drumkv1_midi_in : public drumkv1_sched
{
public:

	drumkv1_midi_in (drumkv1 *pDrumk)
		: drumkv1_sched(pDrumk, MidiIn),
			m_enabled(false), m_count(0) {}

	void schedule_event()
		{ if (m_enabled && ++m_count < 2) schedule(-1); }
	void schedule_note(int key, int vel)
		{ if (m_enabled) schedule((vel << 7) | key); }

	void process(int) {}

	void enabled(bool on)
		{ m_enabled = on; m_count = 0; }

	uint32_t count()
	{
		const uint32_t ret = m_count;
		m_count = 0;
		return ret;
	}

private:

	bool     m_enabled;
	uint32_t m_count;
};


// micro-tuning/instance implementation

class drumkv1_tun
{
public:

	drumkv1_tun() : enabled(false), refPitch(440.0f), refNote(69) {}

	bool    enabled;
	float   refPitch;
	int     refNote;
	QString scaleFile;
	QString keyMapFile;
};


// drum-kit sampler implementation

class drumkv1_impl
{
public:

	drumkv1_impl(drumkv1 *pDrumk, uint16_t nchannels, float srate, uint32_t nsize);

	~drumkv1_impl();

	void setChannels(uint16_t nchannels);
	uint16_t channels() const;

	void setSampleRate(float srate);
	float sampleRate() const;

	void setBufferSize(uint32_t nsize);
	uint32_t bufferSize() const;

	drumkv1_element *addElement(int key);
	drumkv1_element *element(int key) const;
	void removeElement(int key);

	void setCurrentElement(int key);
	int currentElement() const;

	void setCurrentElementTest(int key);
	int currentElementTest();

	void clearElements();

	void setSampleFile(const char *pszSampleFile);
	const char *sampleFile() const;

	drumkv1_sample *sample() const;

	void setReverse(bool bReverse);
	bool isReverse() const;

	void setOffset(bool bOffset);
	bool isOffset() const;

	void setOffsetRange(uint32_t iOffsetStart, uint32_t iOffsetEnd);
	uint32_t offsetStart() const;
	uint32_t offsetEnd() const;

	void setTempo(float bpm);
	float tempo() const;

	void setParamPort(drumkv1::ParamIndex index, float *pfParam);
	drumkv1_port *paramPort(drumkv1::ParamIndex index);

	void setParamValue(drumkv1::ParamIndex index, float fValue);
	float paramValue(drumkv1::ParamIndex index);

	drumkv1_controls *controls();
	drumkv1_programs *programs();

	void setTuningEnabled(bool enabled);
	bool isTuningEnabled() const;

	void setTuningRefPitch(float refPitch);
	float tuningRefPitch() const;

	void setTuningRefNote(int refNote);
	int tuningRefNote() const;

	void setTuningScaleFile(const char *pszScaleFile);
	const char *tuningScaleFile() const;

	void setTuningKeyMapFile(const char *pszKeyMapFile);
	const char *tuningKeyMapFile() const;

	void resetTuning();

	void process_midi(uint8_t *data, uint32_t size);
	void process(float **ins, float **outs, uint32_t nframes);

	void resetParamValues(bool bSwap);

	void stabilize();
	void reset();

	void sampleReverseTest();
	void sampleReverseSync();

	void sampleOffsetTest();
	void sampleOffsetSync();
	void sampleOffsetRangeSync();

	void updateEnvTimes();

	void midiInEnabled(bool on);
	uint32_t midiInCount();

	void directNoteOn(int note, int vel);

	bool running(bool on);

protected:

	void allSoundOff();
	void allControllersOff();
	void allNotesOff();
	void allSustainOff();
	void allSustainOn();

	void resetElement(drumkv1_elem *elem);

	float get_bpm ( float bpm ) const
		{ return (bpm > 0.0f ? bpm : m_bpm); }

	drumkv1_voice *alloc_voice ( int key )
	{
		drumkv1_voice *pv = nullptr;
		drumkv1_elem *elem = m_elems[key];
		if (elem) {
			pv = m_free_list.next();
			if (pv) {
				pv->reset(elem);
				m_free_list.remove(pv);
				m_play_list.append(pv);
				++m_nvoices;
			}
		}
		return pv;
	}

	void free_voice ( drumkv1_voice *pv )
	{
		m_play_list.remove(pv);
		m_free_list.append(pv);
		pv->reset(0);
		--m_nvoices;
	}

	void alloc_sfxs(uint32_t nsize);

private:

	drumkv1 *m_pDrumk;

	drumkv1_config   m_config;
	drumkv1_controls m_controls;
	drumkv1_programs m_programs;
	drumkv1_midi_in  m_midi_in;
	drumkv1_tun      m_tun;

	uint16_t m_nchannels;
	float    m_srate;
	float    m_bpm;

	float    m_freqs[MAX_NOTES];

	drumkv1_ctl m_ctl;

	drumkv1_def m_def;

	drumkv1_cho m_cho;
	drumkv1_fla m_fla;
	drumkv1_pha m_pha;
	drumkv1_del m_del;
	drumkv1_rev m_rev;
	drumkv1_dyn m_dyn;

	drumkv1_voice **m_voices;
	drumkv1_voice  *m_notes[MAX_NOTES];
	drumkv1_voice  *m_group[MAX_GROUP];

	drumkv1_elem   *m_elems[MAX_NOTES];

	drumkv1_elem   *m_elem;

	float *m_params[drumkv1::NUM_ELEMENT_PARAMS];

	drumkv1_port *m_key;

	int m_key0, m_key1;

	drumkv1_list<drumkv1_voice> m_free_list;
	drumkv1_list<drumkv1_voice> m_play_list;

	drumkv1_list<drumkv1_elem>  m_elem_list;

	float  **m_sfxs;
	uint32_t m_nsize;

	drumkv1_fx_chorus   m_chorus;
	drumkv1_fx_flanger *m_flanger;
	drumkv1_fx_phaser  *m_phaser;
	drumkv1_fx_delay   *m_delay;
	drumkv1_fx_comp    *m_comp;

	drumkv1_reverb m_reverb;

	// process direct note on/off...
	volatile uint16_t m_direct_note;

	struct direct_note {
		uint8_t status, note, vel;
	} m_direct_notes[MAX_DIRECT_NOTES];

	volatile int  m_nvoices;

	volatile bool m_running;
};


// voice constructor

drumkv1_voice::drumkv1_voice ( drumkv1_elem *pElem ) :
	note(-1),
	group(-1),
	vel(0.0f),
	pre(0.0f),
	gen1_freq(0.0f),
	lfo1_sample(0.0f),
	out1_panning(0.0f),
	out1_volume(1.0f),
	sustain(false)
{
	reset(pElem);
}


// synth engine constructor

drumkv1_impl::drumkv1_impl (
	drumkv1 *pDrumk, uint16_t nchannels, float srate, uint32_t nsize )
	: m_pDrumk(pDrumk),	m_controls(pDrumk), m_programs(pDrumk),
		m_midi_in(pDrumk), m_bpm(180.0f), m_nvoices(0), m_running(false)
{
	// allocate voice pool.
	m_voices = new drumkv1_voice * [MAX_VOICES];

	for (int i = 0; i < MAX_VOICES; ++i) {
		m_voices[i] = new drumkv1_voice();
		m_free_list.append(m_voices[i]);
	}

	for (int note = 0; note < MAX_NOTES; ++note)
		m_notes[note] = nullptr;

	for (int group = 0; group < MAX_GROUP; ++group)
		m_group[group] = nullptr;

	// reset all current param ports
	for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i)
		m_params[i] = nullptr;

	// special case for sample element switching
	m_key = new drumkv1_port();

	// local buffers none yet
	m_sfxs = nullptr;
	m_nsize = 0;

	// flangers none yet
	m_flanger = nullptr;

	// phasers none yet
	m_phaser = nullptr;

	// delays none yet
	m_delay = nullptr;

	// compressors none yet
	m_comp = nullptr;

	// Micro-tuning support, if any...
	resetTuning();

	// load controllers & programs database...
	m_config.loadControls(&m_controls);
	m_config.loadPrograms(&m_programs);

	// number of channels
	setChannels(nchannels);

	// set default sample rate
	setSampleRate(srate);

	// set default buffer size
	setBufferSize(nsize);

	// start clean empty
	clearElements();

	// reset all voices
	allControllersOff();
	allNotesOff();

	running(true);
}


// destructor

drumkv1_impl::~drumkv1_impl (void)
{
#if 0
	// DO NOT save programs database here:
	// prevent multi-instance clash...
	m_config.savePrograms(&m_programs);
#endif

	// deallocate sample filenames
	setSampleFile(nullptr);

	// deallocate special sample element port
	delete m_key;

	// deallocate voice pool.
	for (int i = 0; i < MAX_VOICES; ++i)
		delete m_voices[i];

	delete [] m_voices;

	// deallocate local buffers
	alloc_sfxs(0);

	// deallocate channels
	setChannels(0);

	// deallocate elements
	clearElements();
}


void drumkv1_impl::setChannels ( uint16_t nchannels )
{
	m_nchannels = nchannels;

	// deallocate flangers
	if (m_flanger) {
		delete [] m_flanger;
		m_flanger = nullptr;
	}

	// deallocate phasers
	if (m_phaser) {
		delete [] m_phaser;
		m_phaser = nullptr;
	}

	// deallocate delays
	if (m_delay) {
		delete [] m_delay;
		m_delay = nullptr;
	}

	// deallocate compressors
	if (m_comp) {
		delete [] m_comp;
		m_comp = nullptr;
	}
}


uint16_t drumkv1_impl::channels (void) const
{
	return m_nchannels;
}


void drumkv1_impl::setSampleRate ( float srate )
{
	// set internal sample rate
	m_srate = srate;
}


float drumkv1_impl::sampleRate (void) const
{
	return m_srate;
}


void drumkv1_impl::setBufferSize ( uint32_t nsize )
{
	// set nominal buffer size
	if (m_nsize < nsize) alloc_sfxs(nsize);
}


uint32_t drumkv1_impl::bufferSize (void) const
{
	return m_nsize;
}


void drumkv1_impl::setTempo ( float bpm )
{
	// set nominal tempo (BPM)
	m_bpm = bpm;
}


float drumkv1_impl::tempo (void) const
{
	return m_bpm;
}


// allocate local buffers
void drumkv1_impl::alloc_sfxs ( uint32_t nsize )
{
	if (m_sfxs) {
		for (uint16_t k = 0; k < m_nchannels; ++k)
			delete [] m_sfxs[k];
		delete [] m_sfxs;
		m_sfxs = nullptr;
		m_nsize = 0;
	}

	if (m_nsize < nsize) {
		m_nsize = nsize;
		m_sfxs = new float * [m_nchannels];
		for (uint16_t k = 0; k < m_nchannels; ++k)
			m_sfxs[k] = new float [m_nsize];
	}
}


drumkv1_element *drumkv1_impl::addElement ( int key )
{
	drumkv1_elem *elem = nullptr;
	if (key >= 0 && key < MAX_NOTES) {
		elem = m_elems[key];
		if (elem == nullptr) {
			elem = new drumkv1_elem(m_pDrumk, m_srate, key);
			m_elem_list.append(elem);
			m_elems[key] = elem;
		}
	}
	return (elem ? &(elem->element) : nullptr);
}


drumkv1_element *drumkv1_impl::element ( int key ) const
{
	drumkv1_elem *elem = nullptr;
	if (key >= 0 && key < MAX_NOTES)
		elem = m_elems[key];
	return (elem ? &(elem->element) : nullptr);
}


void drumkv1_impl::removeElement ( int key )
{
	allNotesOff();

	drumkv1_elem *elem = nullptr;
	if (key >= 0 && key < MAX_NOTES)
		elem = m_elems[key];
	if (elem) {
		if (m_elem == elem)
			m_elem = nullptr;
		m_elem_list.remove(elem);
		m_elems[key] = nullptr;
		delete elem;
	}
}


void drumkv1_impl::setCurrentElement ( int key )
{
	if (m_elem && key == m_key0)
		return;

	// swap old element parameter port values
	drumkv1_elem *elem = m_elem;
	if (elem) {
		for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i) {
			const drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
			if (index == drumkv1::GEN1_SAMPLE)
				continue;
			drumkv1_port *pParamPort = elem->element.paramPort(index);
			if (pParamPort) {
				elem->params[1][i] = pParamPort->tick(drumkv1_port2::NSTEP);
				pParamPort->set_port(nullptr);
			}
		}
		resetElement(elem);
	}

	if (key >= 0 && key < MAX_NOTES) {
		// swap new element parameter port values
		elem = m_elems[key];
		if (elem) {
			for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i) {
				const drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
				if (index == drumkv1::GEN1_SAMPLE)
					continue;
				drumkv1_port *pParamPort = elem->element.paramPort(index);
				if (pParamPort) {
					pParamPort->set_port(m_params[i]);
					pParamPort->set_value(elem->params[1][i]);
					pParamPort->tick(drumkv1_port2::NSTEP);
				}
			}
			resetElement(elem);
		}
		// set new current element
		m_elem = elem;
		m_key0 = key;
	} else {
		// null default element
		m_elem = nullptr;
		m_key0 = -1; // int(drumkv1_param::paramDefaultValue(drumkv1::GEN1_SAMPLE));
	}

	// set current element key parameter port
	m_key->set_value(float(m_key0));
//	m_key1 = m_key->tick(1);
}


int drumkv1_impl::currentElement (void) const
{
	return m_key0;
}


void drumkv1_impl::setCurrentElementTest ( int key )
{
	m_key1 = key;
}


int drumkv1_impl::currentElementTest (void)
{
	const int key = int(m_key->tick(1));
	return (!m_running || m_key1 == key ? -1 : key);
}


void drumkv1_impl::clearElements (void)
{
	// reset element map
	for (int note = 0; note < MAX_NOTES; ++note)
		m_elems[note] = nullptr;

	// reset current element
	m_elem = nullptr;
	m_key0 = -1; // int(drumkv1_param::paramDefaultValue(drumkv1::GEN1_SAMPLE));
	m_key1 = m_key0;

	// deallocate elements
	drumkv1_elem *elem = m_elem_list.next();
	while (elem) {
		m_elem_list.remove(elem);
		delete elem;
		elem = m_elem_list.next();
	}
}


void drumkv1_impl::setSampleFile ( const char *pszSampleFile )
{
	reset();

	if (m_elem) {
		m_elem->element.setSampleFile(pszSampleFile);
		m_elem->updateEnvTimes(m_srate);
	}
}


const char *drumkv1_impl::sampleFile (void) const
{
	return (m_elem ? m_elem->element.sampleFile() : nullptr);
}


drumkv1_sample *drumkv1_impl::sample (void) const
{
	return (m_elem ? m_elem->element.sample() : nullptr);
}


void drumkv1_impl::setReverse ( bool bReverse )
{
	if (m_elem) m_elem->element.setReverse(bReverse);
}


bool drumkv1_impl::isReverse (void) const
{
	return (m_elem ? m_elem->element.isReverse() : false);
}


void drumkv1_impl::setOffset ( bool bOffset )
{
	if (m_elem) m_elem->element.setOffset(bOffset);
}

bool drumkv1_impl::isOffset (void) const
{
	return (m_elem ? m_elem->element.isOffset() : false);
}


void drumkv1_impl::setOffsetRange ( uint32_t iOffsetStart, uint32_t iOffsetEnd )
{
	if (m_elem) m_elem->element.setOffsetRange(iOffsetStart, iOffsetEnd);
}

uint32_t drumkv1_impl::offsetStart (void) const
{
	return (m_elem ? m_elem->element.offsetStart() : 0);
}

uint32_t drumkv1_impl::offsetEnd (void) const
{
	return (m_elem ? m_elem->element.offsetEnd() : 0);
}


void drumkv1_impl::setParamPort ( drumkv1::ParamIndex index, float *pfParam )
{
	static float s_fDummy = 0.0f;

	if (pfParam == nullptr)
		pfParam = &s_fDummy;

	drumkv1_port *pParamPort = paramPort(index);
	if (pParamPort)
		pParamPort->set_port(pfParam);

	// check null connections.
	if (pfParam == &s_fDummy)
		return;

	if (m_elem) {
		switch (index) {
		case drumkv1::DCA1_VOLUME:
		case drumkv1::OUT1_VOLUME:
			m_elem->vol1.reset(
				m_elem->out1.volume.value_ptr(),
				m_elem->dca1.volume.value_ptr(),
				&m_ctl.volume);
			break;
		case drumkv1::OUT1_WIDTH:
			m_elem->wid1.reset(
				m_elem->out1.width.value_ptr());
			break;
		case drumkv1::OUT1_PANNING:
			m_elem->pan1.reset(
				m_elem->out1.panning.value_ptr(),
				&m_ctl.panning);
			break;
		default:
			break;
		}
	}

	if (index < drumkv1::NUM_ELEMENT_PARAMS) {
		if (index == drumkv1::GEN1_SAMPLE)
			m_key->set_port(pfParam);
		else
			m_params[index] = pfParam;
	}
}


drumkv1_port *drumkv1_impl::paramPort ( drumkv1::ParamIndex index )
{
	drumkv1_port *pParamPort = nullptr;

	switch (index) {
	case drumkv1::DEF1_PITCHBEND: pParamPort = &m_def.pitchbend; break;
	case drumkv1::DEF1_MODWHEEL:  pParamPort = &m_def.modwheel;  break;
	case drumkv1::DEF1_PRESSURE:  pParamPort = &m_def.pressure;  break;
	case drumkv1::DEF1_VELOCITY:  pParamPort = &m_def.velocity;  break;
	case drumkv1::DEF1_CHANNEL:   pParamPort = &m_def.channel;   break;
	case drumkv1::DEF1_NOTEOFF:   pParamPort = &m_def.noteoff;   break;
	case drumkv1::CHO1_WET:       pParamPort = &m_cho.wet;       break;
	case drumkv1::CHO1_DELAY:     pParamPort = &m_cho.delay;	 break;
	case drumkv1::CHO1_FEEDB:     pParamPort = &m_cho.feedb;     break;
	case drumkv1::CHO1_RATE:      pParamPort = &m_cho.rate;      break;
	case drumkv1::CHO1_MOD:       pParamPort = &m_cho.mod;       break;
	case drumkv1::FLA1_WET:       pParamPort = &m_fla.wet;       break;
	case drumkv1::FLA1_DELAY:     pParamPort = &m_fla.delay;     break;
	case drumkv1::FLA1_FEEDB:     pParamPort = &m_fla.feedb;     break;
	case drumkv1::FLA1_DAFT:      pParamPort = &m_fla.daft;      break;
	case drumkv1::PHA1_WET:       pParamPort = &m_pha.wet;       break;
	case drumkv1::PHA1_RATE:      pParamPort = &m_pha.rate;      break;
	case drumkv1::PHA1_FEEDB:     pParamPort = &m_pha.feedb;     break;
	case drumkv1::PHA1_DEPTH:     pParamPort = &m_pha.depth;     break;
	case drumkv1::PHA1_DAFT:      pParamPort = &m_pha.daft;      break;
	case drumkv1::DEL1_WET:       pParamPort = &m_del.wet;       break;
	case drumkv1::DEL1_DELAY:     pParamPort = &m_del.delay;     break;
	case drumkv1::DEL1_FEEDB:     pParamPort = &m_del.feedb;     break;
	case drumkv1::DEL1_BPM:       pParamPort = &m_del.bpm;       break;
	case drumkv1::REV1_WET:       pParamPort = &m_rev.wet;       break;
	case drumkv1::REV1_ROOM:      pParamPort = &m_rev.room;      break;
	case drumkv1::REV1_DAMP:      pParamPort = &m_rev.damp;      break;
	case drumkv1::REV1_FEEDB:     pParamPort = &m_rev.feedb;     break;
	case drumkv1::REV1_WIDTH:     pParamPort = &m_rev.width;     break;
	case drumkv1::DYN1_COMPRESS:  pParamPort = &m_dyn.compress;  break;
	case drumkv1::DYN1_LIMITER:   pParamPort = &m_dyn.limiter;   break;
	default:
		if (m_elem) pParamPort = m_elem->element.paramPort(index);
		break;
	}

	return pParamPort;
}


void drumkv1_impl::setParamValue ( drumkv1::ParamIndex index, float fValue )
{
	drumkv1_port *pParamPort = paramPort(index);
	if (pParamPort)
		pParamPort->set_value(fValue);
}


float drumkv1_impl::paramValue ( drumkv1::ParamIndex index )
{
	drumkv1_port *pParamPort = paramPort(index);
	return (pParamPort ? pParamPort->value() : 0.0f);
}


// handle midi input

void drumkv1_impl::process_midi ( uint8_t *data, uint32_t size )
{
	for (uint32_t i = 0; i < size; ++i) {

		// channel status
		const int channel = (data[i] & 0x0f) + 1;
		const int status  = (data[i] & 0xf0);

		// channel filter
		const int ch = int(*m_def.channel);
		const int on = (ch == 0 || ch == channel);

		// all system common/real-time ignored
		if (status == 0xf0)
			continue;

		// check data size (#1)
		if (++i >= size)
			break;

		const int key = (data[i] & 0x7f);

		// program change
		if (status == 0xc0) {
			if (on) m_programs.prog_change(key);
			continue;
		}

		// channel aftertouch
		if (status == 0xd0) {
			if (on) m_ctl.pressure = float(key) / 127.0f;
			continue;
		}

		// check data size (#2)
		if (++i >= size)
			break;

		// channel value
		const int value = (data[i] & 0x7f);

		// channel/controller filter
		if (!on) {
			if (status == 0xb0)
				m_controls.process_enqueue(channel, key, value);
			continue;
		}

		// note on
		if (status == 0x90 && value > 0) {
			drumkv1_voice *pv = m_notes[key];
			if (pv && !(*m_def.noteoff > 0.0f) && pv->note >= 0) {
				drumkv1_elem *elem = pv->elem;
				// retrigger fast release
				elem->dcf1.env.note_off_fast(&pv->dcf1_env);
				elem->lfo1.env.note_off_fast(&pv->lfo1_env);
				elem->dca1.env.note_off_fast(&pv->dca1_env);
				m_notes[key] = nullptr;
				pv->note = -1;
			}
			// find free voice
			pv = alloc_voice(key);
			if (pv) {
				drumkv1_elem *elem = pv->elem;
				// waveform
				pv->note = key;
				// velocity
				const float vel = float(value) / 127.0f;
				// quadratic velocity law
				pv->vel = drumkv1_velocity(vel * vel, *m_def.velocity);
				// pressure/aftertouch
				pv->pre = 0.0f;
				pv->dca1_pre.reset(
					m_def.pressure.value_ptr(),
					&m_ctl.pressure, &pv->pre);
				// generate
				pv->gen1.start();
				// frequencies
				const float gen1_tuning
					= *elem->gen1.coarse * COARSE_SCALE
					+ *elem->gen1.fine * FINE_SCALE;
				pv->gen1_freq = m_freqs[key] * drumkv1_freq2(gen1_tuning);
				// filters
				const int dcf1_type = int(*elem->dcf1.type);
				pv->dcf11.reset(drumkv1_filter1::Type(dcf1_type));
				pv->dcf12.reset(drumkv1_filter1::Type(dcf1_type));
				pv->dcf13.reset(drumkv1_filter2::Type(dcf1_type));
				pv->dcf14.reset(drumkv1_filter2::Type(dcf1_type));
				pv->dcf15.reset(drumkv1_filter3::Type(dcf1_type));
				pv->dcf16.reset(drumkv1_filter3::Type(dcf1_type));
				// formant filters
				const float dcf1_cutoff = *elem->dcf1.cutoff;
				const float dcf1_reso = *elem->dcf1.reso;
				pv->dcf17.reset_filters(dcf1_cutoff, dcf1_reso);
				pv->dcf18.reset_filters(dcf1_cutoff, dcf1_reso);
				// envelopes
				if (*elem->dcf1.enabled > 0.0f)
					elem->dcf1.env.start(&pv->dcf1_env);
				else
					elem->dcf1.env.idle(&pv->dcf1_env);
				if (*elem->lfo1.enabled > 0.0f)
					elem->lfo1.env.start(&pv->lfo1_env);
				else
					elem->lfo1.env.idle(&pv->lfo1_env);
				if (*elem->dca1.enabled > 0.0f)
					elem->dca1.env.start(&pv->dca1_env);
				else
					elem->dca1.env.idle(&pv->dca1_env);
				// lfos
				pv->lfo1_sample = pv->lfo1.start();
				// panning
				pv->out1_panning = 0.0f;
				pv->out1_pan.reset(&pv->out1_panning);
				// volume
				pv->out1_volume = 1.0f;
				pv->out1_vol.reset(&pv->out1_volume);
				// sustain
				pv->sustain = false;
				// allocated
				m_notes[key] = pv;
				// group management
				pv->group = int(*elem->gen1.group) - 1;
				if (pv->group >= 0) {
					drumkv1_voice *pv_group = m_group[pv->group];
					if (pv_group && pv_group->note >= 0 && pv_group->note != key) {
						drumkv1_elem *elem_group = pv_group->elem;
						// retrigger fast release
						elem_group->dcf1.env.note_off_fast(&pv_group->dcf1_env);
						elem_group->lfo1.env.note_off_fast(&pv_group->lfo1_env);
						elem_group->dca1.env.note_off_fast(&pv_group->dca1_env);
						m_notes[pv_group->note] = nullptr;
						pv_group->note = -1;
					}
					m_group[pv->group] = pv;
				}
			}
			m_midi_in.schedule_note(key, value);
		}
		// note off
		else if (status == 0x80 || (status == 0x90 && value == 0)) {
			if (*m_def.noteoff > 0.0f) {
				drumkv1_voice *pv = m_notes[key];
				if (pv && pv->note >= 0) {
					if (m_ctl.sustain)
						pv->sustain = true;
					else
					if (!pv->sustain) {
						if (pv->dca1_env.stage != drumkv1_env::Decay2) {
							drumkv1_elem *elem = pv->elem;
							elem->dca1.env.note_off(&pv->dca1_env);
							elem->dcf1.env.note_off(&pv->dcf1_env);
							elem->lfo1.env.note_off(&pv->lfo1_env);
						}
						m_notes[pv->note] = nullptr;
						pv->note = -1;
					}
				}
			}
			m_midi_in.schedule_note(key, 0);
		}
		// key pressure/poly.aftertouch
		else if (status == 0xa0) {
			drumkv1_voice *pv = m_notes[key];
			if (pv && pv->note >= 0)
				pv->pre = *m_def.pressure * float(value) / 127.0f;
		}
		// control change
		else if (status == 0xb0) {
		switch (key) {
			case 0x00:
				// bank-select MSB (cc#0)
				m_programs.bank_select_msb(value);
				break;
			case 0x01:
				// modulation wheel (cc#1)
				m_ctl.modwheel = *m_def.modwheel * float(value) / 127.0f;
				break;
			case 0x07:
				// channel volume (cc#7)
				m_ctl.volume = float(value) / 127.0f;
				break;
			case 0x0a:
				// channel panning (cc#10)
				m_ctl.panning = float(value - 64) / 64.0f;
				break;
			case 0x20:
				// bank-select LSB (cc#32)
				m_programs.bank_select_lsb(value);
				break;
			case 0x40:
				// sustain/damper pedal (cc#64)
				if (m_ctl.sustain && value <  64)
					allSustainOff();
				m_ctl.sustain = bool(value >= 64);
				break;
			case 0x42:
				// sustenuto pedal (cc#66)
				if (value < 64)
					allSustainOff();
				else
					allSustainOn();
				break;
			case 0x78:
				// all sound off (cc#120)
				allSoundOff();
				break;
			case 0x79:
				// all controllers off (cc#121)
				allControllersOff();
				break;
			case 0x7b:
				// all notes off (cc#123)
				allNotesOff();
				break;
			}
			// process controllers...
			m_controls.process_enqueue(channel, key, value);
		}
		// pitch bend
		else if (status == 0xe0) {
			const float pitchbend = float(key + (value << 7) - 0x2000) / 8192.0f;
			m_ctl.pitchbend = drumkv1_pow2f(*m_def.pitchbend * pitchbend);
		}
	}

	// process pending controllers...
	m_controls.process_dequeue();

	// asynchronous event notification...
	m_midi_in.schedule_event();
}


// all controllers off

void drumkv1_impl::allControllersOff (void)
{
	m_ctl.reset();
}


// all sound off

void drumkv1_impl::allSoundOff (void)
{
	m_chorus.setSampleRate(m_srate);
	m_chorus.reset();

	for (uint16_t k = 0; k < m_nchannels; ++k) {
		m_phaser[k].setSampleRate(m_srate);
		m_delay[k].setSampleRate(m_srate);
		m_comp[k].setSampleRate(m_srate);
		m_flanger[k].reset();
		m_phaser[k].reset();
		m_delay[k].reset();
		m_comp[k].reset();
	}

	m_reverb.setSampleRate(m_srate);
	m_reverb.reset();
}


// all notes off

void drumkv1_impl::allNotesOff (void)
{
	drumkv1_voice *pv = m_play_list.next();
	while (pv) {
		if (pv->note >= 0)
			m_notes[pv->note] = nullptr;
		if (pv->group >= 0)
			m_group[pv->group] = nullptr;
		free_voice(pv);
		pv = m_play_list.next();
	}

	m_direct_note = 0;
}


// all sustained notes off

void drumkv1_impl::allSustainOff (void)
{
	drumkv1_voice *pv = m_play_list.next();
	while (pv) {
		if (pv->note >= 0 && pv->sustain) {
			pv->sustain = false;
			if (pv->dca1_env.stage != drumkv1_env::Decay2) {
				pv->elem->dca1.env.note_off(&pv->dca1_env);
				pv->elem->dcf1.env.note_off(&pv->dcf1_env);
				pv->elem->lfo1.env.note_off(&pv->lfo1_env);
				m_notes[pv->note] = nullptr;
				pv->note = -1;
			}
		}
		pv = pv->next();
	}
}


// sustain all notes on (sustenuto)

void drumkv1_impl::allSustainOn (void)
{
	drumkv1_voice *pv = m_play_list.next();
	while (pv) {
		if (pv->note >= 0 && !pv->sustain)
			pv->sustain = true;
		pv = pv->next();
	}
}


// direct note-on triggered on next cycle...
void drumkv1_impl::directNoteOn ( int note, int vel )
{
	if (vel > 0 && m_nvoices >= MAX_DIRECT_NOTES)
		return;

	const uint32_t i = m_direct_note;
	if (i < MAX_DIRECT_NOTES) {
		const int ch1 = int(*m_def.channel);
		const int chan = (ch1 > 0 ? ch1 - 1 : 0) & 0x0f;
		direct_note& data = m_direct_notes[i];
		data.status = (vel > 0 ? 0x90 : 0x80) | chan;
		data.note = note;
		data.vel = vel;
		++m_direct_note;
	}
}


// element reset

void drumkv1_impl::resetElement ( drumkv1_elem *elem )
{
	elem->vol1.reset(
		elem->out1.volume.value_ptr(),
		elem->dca1.volume.value_ptr(),
		&m_ctl.volume);
	elem->pan1.reset(
		elem->out1.panning.value_ptr(),
		&m_ctl.panning);
	elem->wid1.reset(
		elem->out1.width.value_ptr());
}


// reset/swap all elements params A/B

void drumkv1_impl::resetParamValues ( bool bSwap )
{
	drumkv1_elem *elem = m_elem_list.next();
	while (elem) {
		elem->element.resetParamValues(bSwap);
		elem = elem->next();
	}
}


// controllers accessor

drumkv1_controls *drumkv1_impl::controls (void)
{
	return &m_controls;
}


// programs accessor

drumkv1_programs *drumkv1_impl::programs (void)
{
	return &m_programs;
}


// Micro-tuning support

void drumkv1_impl::setTuningEnabled ( bool enabled )
{
	m_tun.enabled = enabled;
}

bool drumkv1_impl::isTuningEnabled (void) const
{
	return m_tun.enabled;
}


void drumkv1_impl::setTuningRefPitch ( float refPitch )
{
	m_tun.refPitch = refPitch;
}

float drumkv1_impl::tuningRefPitch (void) const
{
	return m_tun.refPitch;
}


void drumkv1_impl::setTuningRefNote ( int refNote )
{
	m_tun.refNote = refNote;
}

int drumkv1_impl::tuningRefNote (void) const
{
	return m_tun.refNote;
}


void drumkv1_impl::setTuningScaleFile ( const char *pszScaleFile )
{
	m_tun.scaleFile = QString::fromUtf8(pszScaleFile);
}

const char *drumkv1_impl::tuningScaleFile (void) const
{
	return m_tun.scaleFile.toUtf8().constData();
}


void drumkv1_impl::setTuningKeyMapFile ( const char *pszKeyMapFile )
{
	m_tun.keyMapFile = QString::fromUtf8(pszKeyMapFile);
}

const char *drumkv1_impl::tuningKeyMapFile (void) const
{
	return m_tun.keyMapFile.toUtf8().constData();
}


void drumkv1_impl::resetTuning (void)
{
	if (m_tun.enabled) {
		// Instance micro-tuning, possibly from Scala keymap and scale files...
		drumkv1_tuning tuning(
			m_tun.refPitch,
			m_tun.refNote);
		if (m_tun.keyMapFile.isEmpty())
		if (!m_tun.keyMapFile.isEmpty())
			tuning.loadKeyMapFile(m_tun.keyMapFile);
		if (!m_tun.scaleFile.isEmpty())
			tuning.loadScaleFile(m_tun.scaleFile);
		for (int note = 0; note < MAX_NOTES; ++note)
			m_freqs[note] = tuning.noteToPitch(note);
		// Done instance tuning.
	}
	else
	if (m_config.bTuningEnabled) {
		// Global/config micro-tuning, possibly from Scala keymap and scale files...
		drumkv1_tuning tuning(
			m_config.fTuningRefPitch,
			m_config.iTuningRefNote);
		if (!m_config.sTuningKeyMapFile.isEmpty())
			tuning.loadKeyMapFile(m_config.sTuningKeyMapFile);
		if (!m_config.sTuningScaleFile.isEmpty())
			tuning.loadScaleFile(m_config.sTuningScaleFile);
		for (int note = 0; note < MAX_NOTES; ++note)
			m_freqs[note] = tuning.noteToPitch(note);
		// Done global/config tuning.
	} else {
		// Native/default tuning, 12-tone equal temperament western standard...
		for (int note = 0; note < MAX_NOTES; ++note)
			m_freqs[note] = drumkv1_freq(note);
		// Done native/default tuning.
	}
}


// all stabilize

void drumkv1_impl::stabilize (void)
{
	for (int i = 0; i < drumkv1::NUM_PARAMS; ++i) {
		drumkv1_port *pParamPort = paramPort(drumkv1::ParamIndex(i));
		if (pParamPort)
			pParamPort->tick(drumkv1_port2::NSTEP);
	}
}


// all reset clear

void drumkv1_impl::reset (void)
{
	// reset all elements
	drumkv1_elem *elem = m_elem_list.next();
	while (elem) {
		resetElement(elem);
		elem->element.resetParamValues(false);
		elem = elem->next();
	}

	// flangers
	if (m_flanger == nullptr)
		m_flanger = new drumkv1_fx_flanger [m_nchannels];

	// phasers
	if (m_phaser == nullptr)
		m_phaser = new drumkv1_fx_phaser [m_nchannels];

	// delays
	if (m_delay == nullptr)
		m_delay = new drumkv1_fx_delay [m_nchannels];

	// compressors
	if (m_comp == nullptr)
		m_comp = new drumkv1_fx_comp [m_nchannels];

	// reverbs
	m_reverb.reset();

	// controllers reset.
	m_controls.reset();

	allSoundOff();
//	allControllersOff();
	allNotesOff();
}


// MIDI input asynchronous status notification accessors

void drumkv1_impl::midiInEnabled ( bool on )
{
	m_midi_in.enabled(on);
}

uint32_t drumkv1_impl::midiInCount (void)
{
	return m_midi_in.count();
}


// synthesize

void drumkv1_impl::process ( float **ins, float **outs, uint32_t nframes )
{
	if (!m_running) return;

	float *v_outs[m_nchannels];
	float *v_sfxs[m_nchannels];

	// FIXME: fx-send buffer reallocation... seriously?
	if (m_nsize < nframes) alloc_sfxs(nframes);

	uint16_t k;

	for (k = 0; k < m_nchannels; ++k) {
		::memset(m_sfxs[k], 0, nframes * sizeof(float));
		::memcpy(outs[k], ins[k], nframes * sizeof(float));
	}

	// process direct note on/off...
	while (m_direct_note > 0) {
		const direct_note& data
			= m_direct_notes[--m_direct_note];
		process_midi((uint8_t *) &data, sizeof(data));
	}

	drumkv1_elem *elem = m_elem_list.next();
	while (elem) {
	#if 0
		if (elem->gen1.sample0 != *elem->gen1.sample) {
			elem->gen1.sample0  = *elem->gen1.sample;
			elem->gen1_sample.reset(note_freq(elem->gen1.sample0));
		}
	#endif
		if (elem->gen1.envtime0 != *elem->gen1.envtime) {
			elem->gen1.envtime0  = *elem->gen1.envtime;
			elem->updateEnvTimes(m_srate);
		}
		if (*elem->lfo1.enabled > 0.0f) {
			elem->lfo1_wave.reset_test(
				drumkv1_wave::Shape(*elem->lfo1.shape), *elem->lfo1.width);
		}
		elem = elem->next();
	}

	// per voice

	drumkv1_voice *pv = m_play_list.next();

	while (pv) {

		drumkv1_voice *pv_next = pv->next();

		// controls
		drumkv1_elem *elem = pv->elem;

		const bool lfo1_enabled = (*elem->lfo1.enabled > 0.0f);

		const float lfo1_freq = (lfo1_enabled
			? get_bpm(*elem->lfo1.bpm) / (60.01f - *elem->lfo1.rate * 60.0f) : 0.0f);

		const float modwheel1 = (lfo1_enabled
			? m_ctl.modwheel + PITCH_SCALE * *elem->lfo1.pitch : 0.0f);

		const bool dcf1_enabled = (*elem->dcf1.enabled > 0.0f);

		const float fxsend1	= *elem->out1.fxsend * *elem->out1.fxsend;

		// channel indexes

		const uint16_t k1 = 0;
		const uint16_t k2 = (elem->gen1_sample.channels() > 1 ? 1 : 0);

		// output buffers

		for (k = 0; k < m_nchannels; ++k) {
			v_outs[k] = outs[k];
			v_sfxs[k] = m_sfxs[k];
		}

		uint32_t nblock = nframes;

		while (nblock > 0) {

			uint32_t ngen = nblock;

			// process envelope stages

			if (pv->dca1_env.running && pv->dca1_env.frames < ngen)
				ngen = pv->dca1_env.frames;
			if (pv->dcf1_env.running && pv->dcf1_env.frames < ngen)
				ngen = pv->dcf1_env.frames;
			if (pv->lfo1_env.running && pv->lfo1_env.frames < ngen)
				ngen = pv->lfo1_env.frames;

			for (uint32_t j = 0; j < ngen; ++j) {

				// velocities

				const float vel1
					= (pv->vel + (1.0f - pv->vel) * pv->dca1_pre.value(j));

				// generators

				const float lfo1_env
					= (lfo1_enabled ? pv->lfo1_env.tick() : 0.0f);
				const float lfo1
					= (lfo1_enabled ? pv->lfo1_sample * lfo1_env : 0.0f);

				pv->gen1.next(pv->gen1_freq
					* (m_ctl.pitchbend + modwheel1 * lfo1));

				float gen1 = pv->gen1.value(k1);
				float gen2 = pv->gen1.value(k2);

				if (lfo1_enabled) {
					pv->lfo1_sample = pv->lfo1.sample(lfo1_freq
						* (1.0f + SWEEP_SCALE * *elem->lfo1.sweep * lfo1_env));
				}

				// filters

				if (dcf1_enabled) {
					const float env1 = 0.5f
						* (1.0f + *elem->dcf1.envelope * pv->dcf1_env.tick());
					const float cutoff1 = drumkv1_sigmoid_1(*elem->dcf1.cutoff
						* env1 * (1.0f + *elem->lfo1.cutoff * lfo1));
					const float reso1 = drumkv1_sigmoid_1(*elem->dcf1.reso
						* env1 * (1.0f + *elem->lfo1.reso * lfo1));
					switch (int(*elem->dcf1.slope)) {
					case 3: // Formant
						gen1 = pv->dcf17.output(gen1, cutoff1, reso1);
						gen2 = pv->dcf18.output(gen2, cutoff1, reso1);
						break;
					case 2: // Biquad
						gen1 = pv->dcf15.output(gen1, cutoff1, reso1);
						gen2 = pv->dcf16.output(gen2, cutoff1, reso1);
						break;
					case 1: // 24db/octave
						gen1 = pv->dcf13.output(gen1, cutoff1, reso1);
						gen2 = pv->dcf14.output(gen2, cutoff1, reso1);
						break;
					case 0: // 12db/octave
					default:
						gen1 = pv->dcf11.output(gen1, cutoff1, reso1);
						gen2 = pv->dcf12.output(gen2, cutoff1, reso1);
						break;
					}
				}

				// volumes

				const float wid1 = elem->wid1.value(j);
				const float mid1 = 0.5f * (gen1 + gen2);
				const float sid1 = 0.5f * (gen1 - gen2);
				const float vol1 = vel1 * elem->vol1.value(j)
					* pv->dca1_env.tick()
					* pv->out1_vol.value(j);

				// outputs

				const float out1 = vol1 * (mid1 + sid1 * wid1)
					* elem->pan1.value(j, 0)
					* pv->out1_pan.value(j, 0);
				const float out2 = vol1 * (mid1 - sid1 * wid1)
					* elem->pan1.value(j, 1)
					* pv->out1_pan.value(j, 1);

				for (k = 0; k < m_nchannels; ++k) {
					const float dry = (k & 1 ? out2 : out1);
					const float wet = fxsend1 * dry;
					*v_outs[k]++ += dry - wet;
					*v_sfxs[k]++ += wet;
				}

				if (j == 0) {
					pv->out1_panning = lfo1 * *elem->lfo1.panning;
					pv->out1_volume  = lfo1 * *elem->lfo1.volume + 1.0f;
				}
			}

			nblock -= ngen;

			// voice ramps countdown

			pv->dca1_pre.process(ngen);
			pv->out1_pan.process(ngen);
			pv->out1_vol.process(ngen);

			// envelope countdowns

			if (pv->dca1_env.running && pv->dca1_env.frames == 0)
				elem->dca1.env.next(&pv->dca1_env);

			if (pv->gen1.isOver() ||
				pv->dca1_env.stage == drumkv1_env::End) {
				if (pv->note >= 0)
					m_notes[pv->note] = nullptr;
				if (pv->group >= 0 && m_group[pv->group] == pv)
					m_group[pv->group] = nullptr;
				free_voice(pv);
				nblock = 0;
			} else {
				if (pv->dcf1_env.running && pv->dcf1_env.frames == 0)
					elem->dcf1.env.next(&pv->dcf1_env);
				if (pv->lfo1_env.running && pv->lfo1_env.frames == 0)
					elem->lfo1.env.next(&pv->lfo1_env);
			}
		}

		// next playing voice

		pv = pv_next;
	}

	// chorus
	if (m_nchannels > 1) {
		m_chorus.process(m_sfxs[0], m_sfxs[1], nframes, *m_cho.wet,
			*m_cho.delay, *m_cho.feedb, *m_cho.rate, *m_cho.mod);
	}

	// effects
	for (k = 0; k < m_nchannels; ++k) {
		float *in = m_sfxs[k];
		// flanger
		m_flanger[k].process(in, nframes, *m_fla.wet,
			*m_fla.delay, *m_fla.feedb, *m_fla.daft * float(k));
		// phaser
		m_phaser[k].process(in, nframes, *m_pha.wet,
			*m_pha.rate, *m_pha.feedb, *m_pha.depth, *m_pha.daft * float(k));
		// delay
		m_delay[k].process(in, nframes, *m_del.wet,
			*m_del.delay, *m_del.feedb, get_bpm(*m_del.bpm));
	}

	// reverb
	if (m_nchannels > 1) {
		m_reverb.process(m_sfxs[0], m_sfxs[1], nframes, *m_rev.wet,
			*m_rev.feedb, *m_rev.room, *m_rev.damp, *m_rev.width);
	}

	// output mix-down
	for (k = 0; k < m_nchannels; ++k) {
		uint32_t n;
		float *sfx = m_sfxs[k];
		// compressor
		if (int(*m_dyn.compress) > 0)
			m_comp[k].process(sfx, nframes);
		// limiter
		if (int(*m_dyn.limiter) > 0) {
			float *p = sfx;
			float *q = sfx;
			for (n = 0; n < nframes; ++n)
				*q++ = drumkv1_sigmoid(*p++);
		}
		// mix-down
		float *out = outs[k];
		for (n = 0; n < nframes; ++n)
			*out++ += *sfx++;
	}

	// post-processing
	elem = m_elem_list.next();
	while (elem) {
		elem->dca1.volume.tick(nframes);
		elem->out1.width.tick(nframes);
		elem->out1.panning.tick(nframes);
		elem->out1.volume.tick(nframes);
		elem->wid1.process(nframes);
		elem->pan1.process(nframes);
		elem->vol1.process(nframes);
		elem = elem->next();
	}

	m_controls.process(nframes);
}


void drumkv1_impl::sampleReverseTest (void)
{
	if (m_running && m_elem) m_elem->element.sampleReverseTest();
}


void drumkv1_impl::sampleReverseSync (void)
{
	if (m_elem) m_elem->element.sampleReverseSync();
}


void drumkv1_impl::sampleOffsetTest (void)
{
	if (m_running && m_elem) m_elem->element.sampleOffsetTest();
}


void drumkv1_impl::sampleOffsetSync (void)
{
	if (m_elem) m_elem->element.sampleOffsetSync();
}


void drumkv1_impl::sampleOffsetRangeSync (void)
{
	if (m_elem) m_elem->element.sampleOffsetRangeSync();
}


void drumkv1_impl::updateEnvTimes (void)
{
	if (m_elem) m_elem->element.updateEnvTimes();
}


// process running state...
bool drumkv1_impl::running ( bool on )
{
	const bool running = m_running;
	m_running = on;
	return running;
}


//-------------------------------------------------------------------------
// drumkv1 - decl.
//

drumkv1::drumkv1 ( uint16_t nchannels, float srate, uint32_t nsize )
{
	m_pImpl = new drumkv1_impl(this, nchannels, srate, nsize);
}


drumkv1::~drumkv1 (void)
{
	delete m_pImpl;
}


void drumkv1::setChannels ( uint16_t nchannels )
{
	m_pImpl->setChannels(nchannels);
}


uint16_t drumkv1::channels (void) const
{
	return m_pImpl->channels();
}


void drumkv1::setSampleRate ( float srate )
{
	m_pImpl->setSampleRate(srate);
}


float drumkv1::sampleRate (void) const
{
	return m_pImpl->sampleRate();
}


void drumkv1::setBufferSize ( uint32_t nsize )
{
	m_pImpl->setBufferSize(nsize);
}


uint32_t drumkv1::bufferSize (void) const
{
	return m_pImpl->bufferSize();
}


drumkv1_element *drumkv1::addElement ( int key )
{
	return m_pImpl->addElement(key);
}

drumkv1_element *drumkv1::element ( int key ) const
{
	return m_pImpl->element(key);
}

void drumkv1::removeElement ( int key )
{
	m_pImpl->removeElement(key);
}


void drumkv1::setCurrentElement ( int key )
{
	selectSample(key);
}

void drumkv1::setCurrentElementEx ( int key )
{
	m_pImpl->setCurrentElement(key);
}


int drumkv1::currentElement (void) const
{
	return m_pImpl->currentElement();
}


void drumkv1::currentElementTest (void)
{
	const int key = m_pImpl->currentElementTest();
	if (key >= 0) {
		m_pImpl->setCurrentElementTest(key);
		selectSample(key);
		return;
	}

	m_pImpl->sampleOffsetTest();
}


void drumkv1::clearElements (void)
{
	m_pImpl->clearElements();
}


void drumkv1::setSampleFile ( const char *pszSampleFile, bool bSync )
{
	m_pImpl->setSampleFile(pszSampleFile);

	if (bSync) updateSample();
}

const char *drumkv1::sampleFile (void) const
{
	return m_pImpl->sampleFile();
}


drumkv1_sample *drumkv1::sample (void) const
{
	return m_pImpl->sample();
}


void drumkv1::setReverse ( bool bReverse, bool bSync )
{
	m_pImpl->setReverse(bReverse);
	m_pImpl->sampleReverseSync();

	if (bSync) updateSample();
}

bool drumkv1::isReverse (void) const
{
	return m_pImpl->isReverse();
}


void drumkv1::setOffset ( bool bOffset, bool bSync )
{
	m_pImpl->setOffset(bOffset);
	m_pImpl->sampleOffsetSync();

	if (bSync) updateOffsetRange();
}

bool drumkv1::isOffset (void) const
{
	return m_pImpl->isOffset();
}


void drumkv1::setOffsetRange ( uint32_t iOffsetStart, uint32_t iOffsetEnd, bool bSync )
{
	m_pImpl->setOffsetRange(iOffsetStart, iOffsetEnd);
	m_pImpl->sampleOffsetRangeSync();
	m_pImpl->updateEnvTimes();

	if (bSync) updateOffsetRange();
}


uint32_t drumkv1::offsetStart (void) const
{
	return m_pImpl->offsetStart();
}


uint32_t drumkv1::offsetEnd (void) const
{
	return m_pImpl->offsetEnd();
}


void drumkv1::setTempo ( float bpm )
{
	m_pImpl->setTempo(bpm);
}


float drumkv1::tempo (void) const
{
	return m_pImpl->tempo();
}


void drumkv1::setParamPort ( ParamIndex index, float *pfParam )
{
	m_pImpl->setParamPort(index, pfParam);
}

drumkv1_port *drumkv1::paramPort ( ParamIndex index ) const
{
	return m_pImpl->paramPort(index);
}


void drumkv1::setParamValue ( ParamIndex index, float fValue )
{
	m_pImpl->setParamValue(index, fValue);
}

float drumkv1::paramValue ( ParamIndex index ) const
{
	return m_pImpl->paramValue(index);
}


void drumkv1::process_midi ( uint8_t *data, uint32_t size )
{
#ifdef CONFIG_DEBUG_0
	fprintf(stderr, "drumkv1[%p]::process_midi(%u)", this, size);
	for (uint32_t i = 0; i < size; ++i)
		fprintf(stderr, " %02x", data[i]);
	fprintf(stderr, "\n");
#endif

	m_pImpl->process_midi(data, size);
}


void drumkv1::process ( float **ins, float **outs, uint32_t nframes )
{
	m_pImpl->process(ins, outs, nframes);

	m_pImpl->sampleReverseTest();
}


// reset/swap all element params A/B

void drumkv1::resetParamValues ( bool bSwap )
{
	m_pImpl->resetParamValues(bSwap);
}


// controllers accessor

drumkv1_controls *drumkv1::controls (void) const
{
	return m_pImpl->controls();
}


// programs accessor

drumkv1_programs *drumkv1::programs (void) const
{
	return m_pImpl->programs();
}


// process state

bool drumkv1::running ( bool on )
{
	return m_pImpl->running(on);
}


// all stabilize

void drumkv1::stabilize (void)
{
	m_pImpl->stabilize();
}


// all reset clear

void drumkv1::reset (void)
{
	m_pImpl->reset();
}


//-------------------------------------------------------------------------
// drumkv1_element - decl.
//

drumkv1_element::drumkv1_element ( drumkv1_elem *pElem )
	: m_pElem(pElem)
{
}


int drumkv1_element::note (void) const
{
	return (m_pElem ? int(m_pElem->gen1.sample0) : -1);
}


void drumkv1_element::setSampleFile ( const char *pszSampleFile )
{
	if (m_pElem) {
		if (pszSampleFile) {
			m_pElem->gen1_sample.open(pszSampleFile,
				drumkv1_freq(m_pElem->gen1.sample0));
		} else {
			m_pElem->gen1_sample.close();
		}
	}
}


const char *drumkv1_element::sampleFile (void) const
{
	return (m_pElem ? m_pElem->gen1_sample.filename() : nullptr);
}


drumkv1_sample *drumkv1_element::sample (void) const
{
	return (m_pElem ? &(m_pElem->gen1_sample) : nullptr);
}


void drumkv1_element::setReverse ( bool bReverse )
{
	if (m_pElem) m_pElem->gen1_sample.setReverse(bReverse);
}


bool drumkv1_element::isReverse (void) const
{
	return (m_pElem ? m_pElem->gen1_sample.isReverse() : false);
}


void drumkv1_element::setOffset ( bool bOffset )
{
	if (m_pElem) m_pElem->gen1_sample.setOffset(bOffset);
}

bool drumkv1_element::isOffset (void) const
{
	return (m_pElem ? m_pElem->gen1_sample.isOffset() : false);
}


void drumkv1_element::setOffsetRange ( uint32_t iOffsetStart, uint32_t iOffsetEnd )
{
	if (m_pElem) m_pElem->gen1_sample.setOffsetRange(iOffsetStart, iOffsetEnd);
}

uint32_t drumkv1_element::offsetStart (void) const
{
	return (m_pElem ? m_pElem->gen1_sample.offsetStart() : 0);
}

uint32_t drumkv1_element::offsetEnd (void) const
{
	return (m_pElem ? m_pElem->gen1_sample.offsetEnd() : 0);
}


void drumkv1_element::setParamPort ( drumkv1::ParamIndex index, float *pfParam )
{
	drumkv1_port *pParamPort = paramPort(index);
	if (pParamPort)
		pParamPort->set_port(pfParam);
}


drumkv1_port *drumkv1_element::paramPort ( drumkv1::ParamIndex index )
{
	if (m_pElem == nullptr)
		return nullptr;

	drumkv1_port *pParamPort = nullptr;

	switch (index) {
//	case drumkv1::GEN1_SAMPLE:   pParamPort = &m_pElem->gen1.sample;     break;
	case drumkv1::GEN1_REVERSE:  pParamPort = &m_pElem->gen1.reverse;    break;
	case drumkv1::GEN1_OFFSET:   pParamPort = &m_pElem->gen1.offset;     break;
	case drumkv1::GEN1_OFFSET_1: pParamPort = &m_pElem->gen1.offset_1;   break;
	case drumkv1::GEN1_OFFSET_2: pParamPort = &m_pElem->gen1.offset_2;   break;
	case drumkv1::GEN1_GROUP:    pParamPort = &m_pElem->gen1.group;      break;
	case drumkv1::GEN1_COARSE:   pParamPort = &m_pElem->gen1.coarse;     break;
	case drumkv1::GEN1_FINE:     pParamPort = &m_pElem->gen1.fine;       break;
	case drumkv1::GEN1_ENVTIME:  pParamPort = &m_pElem->gen1.envtime;    break;
	case drumkv1::DCF1_ENABLED:  pParamPort = &m_pElem->dcf1.enabled;    break;
	case drumkv1::DCF1_CUTOFF:   pParamPort = &m_pElem->dcf1.cutoff;     break;
	case drumkv1::DCF1_RESO:     pParamPort = &m_pElem->dcf1.reso;       break;
	case drumkv1::DCF1_TYPE:     pParamPort = &m_pElem->dcf1.type;       break;
	case drumkv1::DCF1_SLOPE:    pParamPort = &m_pElem->dcf1.slope;      break;
	case drumkv1::DCF1_ENVELOPE: pParamPort = &m_pElem->dcf1.envelope;   break;
	case drumkv1::DCF1_ATTACK:   pParamPort = &m_pElem->dcf1.env.attack; break;
	case drumkv1::DCF1_DECAY1:   pParamPort = &m_pElem->dcf1.env.decay1; break;
	case drumkv1::DCF1_LEVEL2:   pParamPort = &m_pElem->dcf1.env.level2; break;
	case drumkv1::DCF1_DECAY2:   pParamPort = &m_pElem->dcf1.env.decay2; break;
	case drumkv1::LFO1_ENABLED:  pParamPort = &m_pElem->lfo1.enabled;    break;
	case drumkv1::LFO1_SHAPE:    pParamPort = &m_pElem->lfo1.shape;      break;
	case drumkv1::LFO1_WIDTH:    pParamPort = &m_pElem->lfo1.width;      break;
	case drumkv1::LFO1_BPM:      pParamPort = &m_pElem->lfo1.bpm;        break;
	case drumkv1::LFO1_RATE:     pParamPort = &m_pElem->lfo1.rate;       break;
	case drumkv1::LFO1_SWEEP:    pParamPort = &m_pElem->lfo1.sweep;      break;
	case drumkv1::LFO1_PITCH:    pParamPort = &m_pElem->lfo1.pitch;      break;
	case drumkv1::LFO1_CUTOFF:   pParamPort = &m_pElem->lfo1.cutoff;     break;
	case drumkv1::LFO1_RESO:     pParamPort = &m_pElem->lfo1.reso;       break;
	case drumkv1::LFO1_PANNING:  pParamPort = &m_pElem->lfo1.panning;    break;
	case drumkv1::LFO1_VOLUME:   pParamPort = &m_pElem->lfo1.volume;     break;
	case drumkv1::LFO1_ATTACK:   pParamPort = &m_pElem->lfo1.env.attack; break;
	case drumkv1::LFO1_DECAY1:   pParamPort = &m_pElem->lfo1.env.decay1; break;
	case drumkv1::LFO1_LEVEL2:   pParamPort = &m_pElem->lfo1.env.level2; break;
	case drumkv1::LFO1_DECAY2:   pParamPort = &m_pElem->lfo1.env.decay2; break;
	case drumkv1::DCA1_ENABLED:  pParamPort = &m_pElem->dca1.enabled;    break;
	case drumkv1::DCA1_VOLUME:   pParamPort = &m_pElem->dca1.volume;     break;
	case drumkv1::DCA1_ATTACK:   pParamPort = &m_pElem->dca1.env.attack; break;
	case drumkv1::DCA1_DECAY1:   pParamPort = &m_pElem->dca1.env.decay1; break;
	case drumkv1::DCA1_LEVEL2:   pParamPort = &m_pElem->dca1.env.level2; break;
	case drumkv1::DCA1_DECAY2:   pParamPort = &m_pElem->dca1.env.decay2; break;
	case drumkv1::OUT1_WIDTH:    pParamPort = &m_pElem->out1.width;      break;
	case drumkv1::OUT1_PANNING:  pParamPort = &m_pElem->out1.panning;    break;
	case drumkv1::OUT1_FXSEND:   pParamPort = &m_pElem->out1.fxsend;     break;
	case drumkv1::OUT1_VOLUME:   pParamPort = &m_pElem->out1.volume;     break;
	default: break;
	}

	return pParamPort;
}


void drumkv1_element::setParamValue (
	drumkv1::ParamIndex index, float fValue, int pset )
{
	if (index < drumkv1::NUM_ELEMENT_PARAMS && index != drumkv1::GEN1_SAMPLE) {
		m_pElem->params[pset][index] = fValue;
		if (pset == 1) {
			drumkv1_port *pParamPort = paramPort(index);
			if (pParamPort)
				pParamPort->tick(drumkv1_port2::NSTEP);
		}
	}
}


float drumkv1_element::paramValue ( drumkv1::ParamIndex index, int pset )
{
	if (index < drumkv1::NUM_ELEMENT_PARAMS)
		return m_pElem->params[pset][index];
	else
		return 0.0f;
}


void drumkv1_element::resetParamValues ( bool bSwap )
{
	for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i) {
		const drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
		if (index == drumkv1::GEN1_SAMPLE)
			continue;
		const float	fOldValue = m_pElem->params[1][index];
		const float fNewValue = m_pElem->params[2][index];
		m_pElem->params[2][index] = fOldValue;
		if (bSwap)
			m_pElem->params[1][index] = fNewValue;
		else
			m_pElem->params[0][index] = fOldValue;
	}
}


void drumkv1_element::sampleReverseTest (void)
{
	if (m_pElem)
		m_pElem->gen1.reverse.tick(1);
}


void drumkv1_element::sampleReverseSync (void)
{
	if (m_pElem == nullptr)
		return;

	const bool bReverse
		= m_pElem->gen1_sample.isReverse();

	m_pElem->gen1.reverse.set_value_sync(bReverse ? 1.0f : 0.0f);
}


void drumkv1_element::sampleOffsetTest (void)
{
	if (m_pElem) {
		m_pElem->gen1.offset.tick(1);
		m_pElem->gen1.offset_1.tick(1);
		m_pElem->gen1.offset_2.tick(1);
	}
}


void drumkv1_element::sampleOffsetSync (void)
{
	if (m_pElem == nullptr)
		return;

	const bool bOffset
		= m_pElem->gen1_sample.isOffset();

	m_pElem->gen1.offset.set_value_sync(bOffset ? 1.0f : 0.0f);
}


void drumkv1_element::sampleOffsetRangeSync (void)
{
	if (m_pElem == nullptr)
		return;

	const uint32_t iSampleLength
		= m_pElem->gen1_sample.length();
	const uint32_t iOffsetStart
		= m_pElem->gen1_sample.offsetStart();
	const uint32_t iOffsetEnd
		= m_pElem->gen1_sample.offsetEnd();

	const float offset_1 = (iSampleLength > 0
		? float(iOffsetStart) / float(iSampleLength)
		: 0.0f);
	const float offset_2 = (iSampleLength > 0
		? float(iOffsetEnd) / float(iSampleLength)
		: 1.0f);

	m_pElem->gen1.offset_1.set_value_sync(offset_1);
	m_pElem->gen1.offset_2.set_value_sync(offset_2);
}


void drumkv1_element::updateEnvTimes (void)
{
	if (m_pElem)
		m_pElem->updateEnvTimes(m_pElem->gen1_sample.sampleRate());
}


// MIDI input asynchronous status notification accessors

void drumkv1::midiInEnabled ( bool on )
{
	m_pImpl->midiInEnabled(on);
}


uint32_t drumkv1::midiInCount (void)
{
	return m_pImpl->midiInCount();
}


// MIDI direct note on/off triggering

void drumkv1::directNoteOn ( int note, int vel )
{
	m_pImpl->directNoteOn(note, vel);
}


// Micro-tuning support
void drumkv1::setTuningEnabled ( bool enabled )
{
	m_pImpl->setTuningEnabled(enabled);
}

bool drumkv1::isTuningEnabled (void) const
{
	return m_pImpl->isTuningEnabled();
}


void drumkv1::setTuningRefPitch ( float refPitch )
{
	m_pImpl->setTuningRefPitch(refPitch);
}

float drumkv1::tuningRefPitch (void) const
{
	return m_pImpl->tuningRefPitch();
}


void drumkv1::setTuningRefNote ( int refNote )
{
	m_pImpl->setTuningRefNote(refNote);
}

int drumkv1::tuningRefNote (void) const
{
	return m_pImpl->tuningRefNote();
}


void drumkv1::setTuningScaleFile ( const char *pszScaleFile )
{
	m_pImpl->setTuningScaleFile(pszScaleFile);
}

const char *drumkv1::tuningScaleFile (void) const
{
	return m_pImpl->tuningScaleFile();
}


void drumkv1::setTuningKeyMapFile ( const char *pszKeyMapFile )
{
	m_pImpl->setTuningKeyMapFile(pszKeyMapFile);
}

const char *drumkv1::tuningKeyMapFile (void) const
{
	return m_pImpl->tuningKeyMapFile();
}


void drumkv1::resetTuning (void)
{
	m_pImpl->resetTuning();
}


// end of drumkv1.cpp

