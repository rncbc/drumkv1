// drumkv1.cpp
//
/****************************************************************************
   Copyright (C) 2012-2015, rncbc aka Rui Nuno Capela. All rights reserved.

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


#ifdef CONFIG_DEBUG_0
#include <stdio.h>
#endif

#include <string.h>


//-------------------------------------------------------------------------
// drumkv1_impl
//
// -- borrowed and revamped from synth.h of synth4
//    Copyright (C) 2007 jorgen, linux-vst.com
//

const uint16_t MAX_VOICES = 32;			// polyphony
const uint8_t  MAX_NOTES  = 128;
const uint8_t  MAX_GROUP  = 128;

const float MIN_ENV_MSECS = 2.0f;		// min 2msec per stage
const float MAX_ENV_MSECS = 2000.0f;	// max 2 sec per stage (default)

const float DETUNE_SCALE  = 0.5f;
const float PHASE_SCALE   = 0.5f;
const float COARSE_SCALE  = 12.0f;
const float FINE_SCALE    = 1.0f;
const float SWEEP_SCALE   = 0.5f;
const float PITCH_SCALE   = 0.5f;

const float LFO_FREQ_MIN  = 0.4f;
const float LFO_FREQ_MAX  = 40.0f;


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

inline float drumkv1_freq ( float note )
{
	return (440.0f / 32.0f) * ::powf(2.0f, (note - 9.0f) / 12.0f);
}


// envelope

struct drumkv1_env
{
	// envelope stages

	enum Stage { Idle = 0, Attack, Decay1, Decay2 };

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
		p->phase = 0.0f;
		if (p->frames > 0) {
			p->delta = 1.0f / float(p->frames);
			p->value = 0.0f;
		} else {
			p->delta = 0.0f;
			p->value = 1.0f;
		}
		p->c1 = 1.0f;
		p->c0 = 0.0f;
	}

	void next(State *p)
	{
		if (p->stage == Attack) {
			p->stage = Decay1;
			p->frames = uint32_t(*decay1 * *decay1 * max_frames);
			if (p->frames < min_frames) // prevent click on too fast decay
				p->frames = min_frames;
			p->phase = 0.0f;
			p->delta = 1.0f / float(p->frames);
			p->c1 = *level2 - 1.0f;
			p->c0 = p->value;
		}
		else if (p->stage == Decay1) {
			p->stage = Decay2;
			p->frames = uint32_t(*decay2 * *decay2 * max_frames);
			if (p->frames < min_frames) // prevent click on too fast decay
				p->frames = min_frames;
			p->phase = 0.0f;
			p->delta = 1.0f / float(p->frames);
			p->c1 = -(p->value);
			p->c0 = p->value;
		}
		else if (p->stage == Decay2) {
			p->running = false;
			p->stage = Idle;
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
		if (p->frames < min_frames) // prevent click on too fast release
			p->frames = min_frames;
		p->phase = 0.0f;
		p->delta = 1.0f / float(p->frames);
		p->c1 = -(p->value);
		p->c0 = p->value;
	}

	void note_off_fast(State *p)
	{
		p->running = true;
		p->stage = Decay2;
		p->frames = min_frames;
		p->phase = 0.0f;
		p->delta = 1.0f / float(p->frames);
		p->c1 = -(p->value);
		p->c0 = p->value;
	}

	// parameters

	float *attack;
	float *decay1;
	float *level2;
	float *decay2;

	uint32_t min_frames;
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
	}

	float pressure;
	float pitchbend;
	float modwheel;
	float panning;
	float volume;
};


// internal control

struct drumkv1_aux
{
	drumkv1_aux() { reset(); }

	void reset()
	{
		panning = 0.0f;
		volume = 1.0f;
	}

	float panning;
	float volume;
};


// dco

struct drumkv1_gen
{
	float *sample, sample0;
	float *reverse;
	float *group;
	float *coarse;
	float *fine;
	float *envtime, envtime0;
};


// dcf

struct drumkv1_dcf
{
	float *cutoff;
	float *reso;
	float *type;
	float *slope;
	float *envelope;

	drumkv1_env env;
};


// lfo

struct drumkv1_lfo
{
	float *shape;
	float *width;
	float *rate;
	float *sync;
	float *sweep;
	float *pitch;
	float *cutoff;
	float *reso;
	float *panning;
	float *volume;

	drumkv1_env env;

	float *bpmsync;
};


// dca

struct drumkv1_dca
{
	float *volume;

	drumkv1_env env;

	float *bpmsync;
};



// def (ranges)

struct drumkv1_def
{
	float *pitchbend;
	float *modwheel;
	float *pressure;
	float *velocity;
	float *channel;
	float *noteoff;
};


// out (mix)

struct drumkv1_out
{
	float *width;
	float *panning;
	float *fxsend;
	float *volume;
};


// chorus (fx)

struct drumkv1_cho
{
	float *wet;
	float *delay;
	float *feedb;
	float *rate;
	float *mod;
};


// flanger (fx)

struct drumkv1_fla
{
	float *wet;
	float *delay;
	float *feedb;
	float *daft;
};


// phaser (fx)

struct drumkv1_pha
{
	float *wet;
	float *rate;
	float *feedb;
	float *depth;
	float *daft;
};


// delay (fx)

struct drumkv1_del
{
	float *wet;
	float *delay;
	float *feedb;
	float *bpm;
	float *bpmsync;
};


// reverb

struct drumkv1_rev
{
	float *wet;
	float *room;
	float *damp;
	float *feedb;
	float *width;
};


// dynamic(compressor/limiter)

struct drumkv1_dyn
{
	float *compress;
	float *limiter;
};


// panning smoother (3 parameters)

class drumkv1_pan : public drumkv1_ramp3
{
public:

	drumkv1_pan() : drumkv1_ramp3(2) {}

protected:

	float evaluate(uint16_t i)
	{
		drumkv1_ramp3::update();

		const float wpan = 0.25f * M_PI
			* (1.0f + m_param1_v)
			* (1.0f + m_param2_v)
			* (1.0f + m_param3_v);

		return M_SQRT2 * (i == 0 ? ::cosf(wpan) : ::sinf(wpan));
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

	drumkv1_phasor(uint32_t nsize = 128)
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

	drumkv1_sample  gen1_sample;
	drumkv1_wave_lf lfo1_wave;

	drumkv1_formant::Impl dcf1_formant;

	drumkv1_gen    gen1;
	drumkv1_dcf    dcf1;
	drumkv1_lfo    lfo1;
	drumkv1_dca    dca1;
	drumkv1_out    out1;

	drumkv1_aux    aux1;
	drumkv1_ramp1  wid1;
	drumkv1_pan    pan1;
	drumkv1_ramp4  vol1;

	float params[3][drumkv1::NUM_ELEMENT_PARAMS];

	void updateEnvTimes(float srate);
};


// synth element

drumkv1_elem::drumkv1_elem ( drumkv1 *pDrumk, float srate, int key )
	: element(this), gen1_sample(pDrumk)
{
	// element parameter value set
	for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i)
		for (int j = 0; j < 3; ++j) params[j][i] = 0.0f;

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
	if (envtime_msecs < MIN_ENV_MSECS)
		envtime_msecs = (gen1_sample.length() >> 1) / srate_ms;
	if (envtime_msecs < MIN_ENV_MSECS)
		envtime_msecs = MIN_ENV_MSECS + 1.0f;

	const uint32_t min_frames = uint32_t(srate_ms * MIN_ENV_MSECS);
	const uint32_t max_frames = uint32_t(srate_ms * envtime_msecs);

	dcf1.env.min_frames = min_frames;
	dcf1.env.max_frames = max_frames;

	lfo1.env.min_frames = min_frames;
	lfo1.env.max_frames = max_frames;

	dca1.env.min_frames = min_frames;
	dca1.env.max_frames = max_frames;
}


// voice

struct drumkv1_voice : public drumkv1_list<drumkv1_voice>
{
	drumkv1_voice(drumkv1_elem *pElem = 0) { reset(pElem); }

	void reset(drumkv1_elem *pElem)
	{
		elem = pElem;

		gen1.reset(pElem ? &pElem->gen1_sample : 0);
		lfo1.reset(pElem ? &pElem->lfo1_wave : 0);

		dcf17.reset(pElem ? &pElem->dcf1_formant : 0);
		dcf18.reset(pElem ? &pElem->dcf1_formant : 0);
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
};


// synth engine implementation

class drumkv1_impl
{
public:

	drumkv1_impl(drumkv1 *pDrumk, uint16_t nchannels, float srate);

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

	void clearElements();

	void setSampleFile(const char *pszSampleFile);
	const char *sampleFile() const;

	drumkv1_sample *sample() const;

	void setReverse(bool bReverse);
	bool isReverse() const;

	void setParamPort(drumkv1::ParamIndex index, float *pfParam = 0);
	float *paramPort(drumkv1::ParamIndex index) const;

	void setParamValue(drumkv1::ParamIndex index, float fValue);
	float paramValue(drumkv1::ParamIndex index) const;

	drumkv1_controls *controls();
	drumkv1_programs *programs();

	void process_midi(uint8_t *data, uint32_t size);
	void process(float **ins, float **outs, uint32_t nframes);

	void resetParamValues(bool bSwap);
	void reset();

protected:

	void allSoundOff();
	void allControllersOff();
	void allNotesOff();

	void resetElement(drumkv1_elem *elem);

	drumkv1_voice *alloc_voice ( int key )
	{
		drumkv1_voice *pv = 0;
		drumkv1_elem *elem = m_elems[key];
		if (elem) {
			pv = m_free_list.next();
			if (pv) {
				pv->reset(elem);
				m_free_list.remove(pv);
				m_play_list.append(pv);
			}
		}
		return pv;
	}

	void free_voice ( drumkv1_voice *pv )
	{
		m_play_list.remove(pv);
		m_free_list.append(pv);
		pv->reset(0);
	}

	void alloc_sfxs(uint32_t nsize);

private:

	drumkv1 *m_pDrumk;

	drumkv1_config   m_config;
	drumkv1_controls m_controls;
	drumkv1_programs m_programs;

	uint16_t m_nchannels;
	float    m_srate;

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
	drumkv1_phasor m_phasor;
};


// synth engine constructor

drumkv1_impl::drumkv1_impl (
	drumkv1 *pDrumk, uint16_t nchannels, float srate )
	: m_pDrumk(pDrumk), m_controls(pDrumk), m_programs(pDrumk)
{
	// allocate voice pool.
	m_voices = new drumkv1_voice * [MAX_VOICES];

	for (int i = 0; i < MAX_VOICES; ++i) {
		m_voices[i] = new drumkv1_voice();
		m_free_list.append(m_voices[i]);
	}

	for (int note = 0; note < MAX_NOTES; ++note)
		m_notes[note] = 0;

	for (int group = 0; group < MAX_GROUP; ++group)
		m_group[group] = 0;

	// local buffers none yet
	m_sfxs = NULL;
	m_nsize = 0;

	// flangers none yet
	m_flanger = 0;

	// phasers none yet
	m_phaser = 0;

	// delays none yet
	m_delay = 0;

	// compressors none yet
	m_comp = 0;

	// load controllers & programs database...
	m_config.loadControls(&m_controls);
	m_config.loadPrograms(&m_programs);

	// number of channels
	setChannels(nchannels);

	// set default sample rate
	setSampleRate(srate);

	// init default element (eg. 36=Bass Drum 1)
	clearElements();

	// element parameter ports (default)
	for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i)
		m_params[i] = 0;

	// parameter ports
	for (uint32_t i = 0; i < drumkv1::NUM_PARAMS; ++i)
		setParamPort(drumkv1::ParamIndex(i));

	// reset all voices
	allControllersOff();
	allNotesOff();
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
	setSampleFile(0);

	// deallocate voice pool.
	for (int i = 0; i < MAX_VOICES; ++i)
		delete m_voices[i];

	delete [] m_voices;

	// deallocate channels
	setChannels(0);

	// deallocate elements
	clearElements();

	// deallocate local buffers
	alloc_sfxs(0);
}


void drumkv1_impl::setChannels ( uint16_t nchannels )
{
	m_nchannels = nchannels;

	// deallocate flangers
	if (m_flanger) {
		delete [] m_flanger;
		m_flanger = 0;
	}

	// deallocate phasers
	if (m_phaser) {
		delete [] m_phaser;
		m_phaser = 0;
	}

	// deallocate delays
	if (m_delay) {
		delete [] m_delay;
		m_delay = 0;
	}

	// deallocate compressors
	if (m_comp) {
		delete [] m_comp;
		m_comp = 0;
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


// allocate local buffers
void drumkv1_impl::alloc_sfxs ( uint32_t nsize )
{
	if (m_sfxs) {
		for (uint16_t k = 0; k < m_nchannels; ++k)
			delete [] m_sfxs[k];
		delete [] m_sfxs;
		m_sfxs = NULL;
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
	drumkv1_elem *elem = 0;
	if (key >= 0 && key < MAX_NOTES) {
		elem = m_elems[key];
		if (elem == 0) {
			elem = new drumkv1_elem(m_pDrumk, m_srate, key);
			m_elem_list.append(elem);
			m_elems[key] = elem;
		}
	}
	return (elem ? &(elem->element) : 0);
}


drumkv1_element *drumkv1_impl::element ( int key ) const
{
	drumkv1_elem *elem = 0;
	if (key >= 0 && key < MAX_NOTES)
		elem = m_elems[key];
	return (elem ? &(elem->element) : 0);
}


void drumkv1_impl::removeElement ( int key )
{
	allNotesOff();

	drumkv1_elem *elem = 0;
	if (key >= 0 && key < MAX_NOTES)
		elem = m_elems[key];
	if (elem) {
		if (m_elem == elem)
			m_elem = 0;
		m_elem_list.remove(elem);
		m_elems[key] = 0;
		delete elem;
	}
}


void drumkv1_impl::setCurrentElement ( int key )
{
	if (key >= 0 && key < MAX_NOTES) {
		// swap old element parameter port values
		drumkv1_elem *elem = m_elem;
		if (elem) {
			drumkv1_element *element = &(elem->element);
			for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i) {
				const drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
				if (index == drumkv1::GEN1_SAMPLE)
					continue;
				float *pfParam = m_params[i];
				if (pfParam) {
					elem->params[1][i] = *pfParam;
					element->setParamPort(index, &(elem->params[1][i]));
				}
			}
			resetElement(elem);
		}
		// swap new element parameter port values
		elem = m_elems[key];
		if (elem) {
			drumkv1_element *element = &(elem->element);
			for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i) {
				const drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
				if (index == drumkv1::GEN1_SAMPLE)
					continue;
				float *pfParam = m_params[i];
				if (pfParam) {
					*pfParam = elem->params[1][i]; // LV2:BUG?
					element->setParamPort(index, pfParam);
				}
			}
			resetElement(elem);
		}
		// set new current element
		m_elem = elem;
	}
	else m_elem = 0;
}


int drumkv1_impl::currentElement (void) const
{
	return (m_elem ? int(m_elem->gen1.sample0) : -1);
}


void drumkv1_impl::clearElements (void)
{
	// reset element map
	for (int note = 0; note < MAX_NOTES; ++note)
		m_elems[note] = 0;

	// reset current element
	m_elem = 0;

	// deallocate elements
	drumkv1_elem *elem = m_elem_list.next();
	while (elem) {
		m_elem_list.remove(elem);
		delete elem;
		elem = m_elem_list.next();
	}

#if 0
	// init default element (eg. 36=Bass Drum 1)
	m_elem_list.append(new drumkv1_elem(m_srate, 36));
	m_elem = m_elem_list.next();
	m_elems[36] = m_elem;
#endif
}


void drumkv1_impl::setSampleFile ( const char *pszSampleFile )
{
	reset();

	if (m_elem) m_elem->element.setSampleFile(pszSampleFile);
}


const char *drumkv1_impl::sampleFile (void) const
{
	return (m_elem ? m_elem->element.sampleFile() : 0);
}


drumkv1_sample *drumkv1_impl::sample (void) const
{
	return (m_elem ? m_elem->element.sample() : 0);
}


void drumkv1_impl::setReverse ( bool bReverse )
{
	if (m_elem) m_elem->element.setReverse(bReverse);
}


bool drumkv1_impl::isReverse (void) const
{
	return (m_elem ? m_elem->element.isReverse() : false);
}


void drumkv1_impl::setParamPort ( drumkv1::ParamIndex index, float *pfParam )
{
	static float s_fDummy = 0.0f;

	if (pfParam == 0)
		pfParam = &s_fDummy;

	switch (index) {
	case drumkv1::DEF1_PITCHBEND: m_def.pitchbend = pfParam; break;
	case drumkv1::DEF1_MODWHEEL:  m_def.modwheel  = pfParam; break;
	case drumkv1::DEF1_PRESSURE:  m_def.pressure  = pfParam; break;
	case drumkv1::DEF1_VELOCITY:  m_def.velocity  = pfParam; break;
	case drumkv1::DEF1_CHANNEL:   m_def.channel   = pfParam; break;
	case drumkv1::DEF1_NOTEOFF:   m_def.noteoff   = pfParam; break;
	case drumkv1::CHO1_WET:       m_cho.wet       = pfParam; break;
	case drumkv1::CHO1_DELAY:     m_cho.delay     = pfParam; break;
	case drumkv1::CHO1_FEEDB:     m_cho.feedb     = pfParam; break;
	case drumkv1::CHO1_RATE:      m_cho.rate      = pfParam; break;
	case drumkv1::CHO1_MOD:       m_cho.mod       = pfParam; break;
	case drumkv1::FLA1_WET:       m_fla.wet       = pfParam; break;
	case drumkv1::FLA1_DELAY:     m_fla.delay     = pfParam; break;
	case drumkv1::FLA1_FEEDB:     m_fla.feedb     = pfParam; break;
	case drumkv1::FLA1_DAFT:      m_fla.daft      = pfParam; break;
	case drumkv1::PHA1_WET:       m_pha.wet       = pfParam; break;
	case drumkv1::PHA1_RATE:      m_pha.rate      = pfParam; break;
	case drumkv1::PHA1_FEEDB:     m_pha.feedb     = pfParam; break;
	case drumkv1::PHA1_DEPTH:     m_pha.depth     = pfParam; break;
	case drumkv1::PHA1_DAFT:      m_pha.daft      = pfParam; break;
	case drumkv1::DEL1_WET:       m_del.wet       = pfParam; break;
	case drumkv1::DEL1_DELAY:     m_del.delay     = pfParam; break;
	case drumkv1::DEL1_FEEDB:     m_del.feedb     = pfParam; break;
	case drumkv1::DEL1_BPM:       m_del.bpm       = pfParam; break;
	case drumkv1::DEL1_BPMSYNC:   m_del.bpmsync   = pfParam; break;
	case drumkv1::REV1_WET:       m_rev.wet       = pfParam; break;
	case drumkv1::REV1_ROOM:      m_rev.room      = pfParam; break;
	case drumkv1::REV1_DAMP:      m_rev.damp      = pfParam; break;
	case drumkv1::REV1_FEEDB:     m_rev.feedb     = pfParam; break;
	case drumkv1::REV1_WIDTH:     m_rev.width     = pfParam; break;
	case drumkv1::DYN1_COMPRESS:  m_dyn.compress  = pfParam; break;
	case drumkv1::DYN1_LIMITER:   m_dyn.limiter   = pfParam; break;
	default:
		if (m_elem) {
			m_elem->element.setParamPort(index, pfParam);
			// check null connections.
			if (pfParam != &s_fDummy)
			switch (index) {
			case drumkv1::DCA1_VOLUME:
			case drumkv1::OUT1_VOLUME:
				m_elem->vol1.reset(m_elem->out1.volume,
					m_elem->dca1.volume,
					&m_ctl.volume,
					&m_elem->aux1.volume);
				break;
			case drumkv1::OUT1_WIDTH:
				m_elem->wid1.reset(m_elem->out1.width);
				break;
			case drumkv1::OUT1_PANNING:
				m_elem->pan1.reset(m_elem->out1.panning,
					&m_ctl.panning,
					&m_elem->aux1.panning);
				break;
			default:
				break;
			}
		}
		m_params[index] = pfParam;
		break;
	}
}


float *drumkv1_impl::paramPort ( drumkv1::ParamIndex index ) const
{
	float *pfParam = 0;

	switch (index) {
	case drumkv1::DEF1_PITCHBEND: pfParam = m_def.pitchbend; break;
	case drumkv1::DEF1_MODWHEEL:  pfParam = m_def.modwheel;  break;
	case drumkv1::DEF1_PRESSURE:  pfParam = m_def.pressure;  break;
	case drumkv1::DEF1_VELOCITY:  pfParam = m_def.velocity;  break;
	case drumkv1::DEF1_CHANNEL:   pfParam = m_def.channel;   break;
	case drumkv1::DEF1_NOTEOFF:   pfParam = m_def.noteoff;   break;
	case drumkv1::CHO1_WET:       pfParam = m_cho.wet;       break;
	case drumkv1::CHO1_DELAY:     pfParam = m_cho.delay;	 break;
	case drumkv1::CHO1_FEEDB:     pfParam = m_cho.feedb;     break;
	case drumkv1::CHO1_RATE:      pfParam = m_cho.rate;      break;
	case drumkv1::CHO1_MOD:       pfParam = m_cho.mod;       break;
	case drumkv1::FLA1_WET:       pfParam = m_fla.wet;       break;
	case drumkv1::FLA1_DELAY:     pfParam = m_fla.delay;     break;
	case drumkv1::FLA1_FEEDB:     pfParam = m_fla.feedb;     break;
	case drumkv1::FLA1_DAFT:      pfParam = m_fla.daft;      break;
	case drumkv1::PHA1_WET:       pfParam = m_pha.wet;       break;
	case drumkv1::PHA1_RATE:      pfParam = m_pha.rate;      break;
	case drumkv1::PHA1_FEEDB:     pfParam = m_pha.feedb;     break;
	case drumkv1::PHA1_DEPTH:     pfParam = m_pha.depth;     break;
	case drumkv1::PHA1_DAFT:      pfParam = m_pha.daft;      break;
	case drumkv1::DEL1_WET:       pfParam = m_del.wet;       break;
	case drumkv1::DEL1_DELAY:     pfParam = m_del.delay;     break;
	case drumkv1::DEL1_FEEDB:     pfParam = m_del.feedb;     break;
	case drumkv1::DEL1_BPM:       pfParam = m_del.bpm;       break;
	case drumkv1::DEL1_BPMSYNC:   pfParam = m_del.bpmsync;   break;
	case drumkv1::REV1_WET:       pfParam = m_rev.wet;       break;
	case drumkv1::REV1_ROOM:      pfParam = m_rev.room;      break;
	case drumkv1::REV1_DAMP:      pfParam = m_rev.damp;      break;
	case drumkv1::REV1_FEEDB:     pfParam = m_rev.feedb;     break;
	case drumkv1::REV1_WIDTH:     pfParam = m_rev.width;     break;
	case drumkv1::DYN1_COMPRESS:  pfParam = m_dyn.compress;  break;
	case drumkv1::DYN1_LIMITER:   pfParam = m_dyn.limiter;   break;
	default:
		pfParam = m_params[index];
		break;
	}

	return pfParam;
}


void drumkv1_impl::setParamValue ( drumkv1::ParamIndex index, float fValue )
{
	float *pfParamPort = paramPort(index);
	if (pfParamPort)
		*pfParamPort = fValue; // LV2:BUG?
}


float drumkv1_impl::paramValue ( drumkv1::ParamIndex index ) const
{
	float *pfParamPort = paramPort(index);
	return (pfParamPort ? *pfParamPort : 0.0f);
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

		// channel filter
		if (!on)
			continue;

		const int value = (data[i] & 0x7f);

		// note on
		if (status == 0x90 && value > 0) {
			drumkv1_voice *pv = m_notes[key];
			if (pv && !(int(*m_def.noteoff) > 0)) {
				drumkv1_elem *elem = pv->elem;
				// retrigger fast release
				elem->dcf1.env.note_off_fast(&pv->dcf1_env);
				elem->lfo1.env.note_off_fast(&pv->lfo1_env);
				elem->dca1.env.note_off_fast(&pv->dca1_env);
				m_notes[key] = 0;
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
				pv->vel  = drumkv1_velocity(vel * vel, *m_def.velocity);
				// pressure/aftertouch
				pv->pre = 0.0f;
				pv->dca1_pre.reset(m_def.pressure, &m_ctl.pressure, &pv->pre);
				// generate
				pv->gen1.start();
				// frequencies
				const float freq1 = float(key)
					+ *elem->gen1.coarse * COARSE_SCALE
					+ *elem->gen1.fine * FINE_SCALE;
				pv->gen1_freq = drumkv1_freq(freq1);
				// filters
				const int type1 = int(*elem->dcf1.type);
				pv->dcf11.reset(drumkv1_filter1::Type(type1));
				pv->dcf12.reset(drumkv1_filter1::Type(type1));
				pv->dcf13.reset(drumkv1_filter2::Type(type1));
				pv->dcf14.reset(drumkv1_filter2::Type(type1));
				pv->dcf15.reset(drumkv1_filter3::Type(type1));
				pv->dcf16.reset(drumkv1_filter3::Type(type1));
				pv->dcf17.reset_filters();
				pv->dcf18.reset_filters();
				// envelopes
				elem->dcf1.env.start(&pv->dcf1_env);
				elem->lfo1.env.start(&pv->lfo1_env);
				elem->dca1.env.start(&pv->dca1_env);
				// lfos
				const float pshift1
					= (*elem->lfo1.sync > 0.0f ? m_phasor.pshift() : 0.0f);
				pv->lfo1_sample = pv->lfo1.start(pshift1);
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
						m_notes[pv_group->note] = 0;
						pv_group->note = -1;
					}
					m_group[pv->group] = pv;
				}
			}
		}
		// note off
		else if (status == 0x80 || (status == 0x90 && value == 0)) {
			if (int(*m_def.noteoff) > 0) {
				drumkv1_voice *pv = m_notes[key];
				if (pv && pv->note >= 0) {
					if (pv->dca1_env.stage != drumkv1_env::Decay2) {
						drumkv1_elem *elem = pv->elem;
						elem->dca1.env.note_off(&pv->dca1_env);
						elem->dcf1.env.note_off(&pv->dcf1_env);
						elem->lfo1.env.note_off(&pv->lfo1_env);
					}
				}
			}
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
			m_notes[pv->note] = 0;
		if (pv->group >= 0)
			m_group[pv->group] = 0;
		free_voice(pv);
		pv = m_play_list.next();
	}

	drumkv1_elem *elem = m_elem_list.next();
	while (elem) {
		elem->aux1.reset();
		elem = elem->next();
	}
}


// element reset

void drumkv1_impl::resetElement ( drumkv1_elem *elem )
{
	elem->vol1.reset(elem->out1.volume, elem->dca1.volume,
		&m_ctl.volume, &elem->aux1.volume);
	elem->pan1.reset(elem->out1.panning,
		&m_ctl.panning, &elem->aux1.panning);
	elem->wid1.reset(elem->out1.width);
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


// all reset clear

void drumkv1_impl::reset (void)
{
#if 0//--legacy support < 0.3.0.4
	if (*m_del.bpm < 3.6f)
		*m_del.bpm *= 100.0f;
#endif

	// reset all elements
	drumkv1_elem *elem = m_elem_list.next();
	while (elem) {
		resetElement(elem);
		elem->element.resetParamValues(false);
		elem = elem->next();
	}

	// flangers
	if (m_flanger == 0)
		m_flanger = new drumkv1_fx_flanger [m_nchannels];

	// phasers
	if (m_phaser == 0)
		m_phaser = new drumkv1_fx_phaser [m_nchannels];

	// delays
	if (m_delay == 0)
		m_delay = new drumkv1_fx_delay [m_nchannels];

	// compressors
	if (m_comp == 0)
		m_comp = new drumkv1_fx_comp [m_nchannels];

	// reverbs
	m_reverb.reset();

	// controllers reset.
	m_controls.reset();

	allSoundOff();
//	allControllersOff();
	allNotesOff();
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


// synthesize

void drumkv1_impl::process ( float **ins, float **outs, uint32_t nframes )
{
	float *v_outs[m_nchannels];
	float *v_sfxs[m_nchannels];

	// buffer i/o transfer
	if (m_nsize < nframes) alloc_sfxs(nframes);

	uint16_t k;

	for (k = 0; k < m_nchannels; ++k) {
		::memcpy(m_sfxs[k], ins[k], nframes * sizeof(float));
		::memset(outs[k], 0, nframes * sizeof(float));
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
		elem->gen1_sample.reverse_test(*elem->gen1.reverse > 0.5f);
		elem->lfo1_wave.reset_test(
			drumkv1_wave::Shape(*elem->lfo1.shape), *elem->lfo1.width);
		elem = elem->next();
	}

	// per voice

	drumkv1_voice *pv = m_play_list.next();

	while (pv) {

		drumkv1_voice *pv_next = pv->next();

		// controls
		drumkv1_elem *elem = pv->elem;

		const float lfo1_rate = *elem->lfo1.rate * *elem->lfo1.rate;

		const float lfo1_freq
			= LFO_FREQ_MIN + lfo1_rate * (LFO_FREQ_MAX - LFO_FREQ_MIN);

		const float modwheel1
			= m_ctl.modwheel + PITCH_SCALE * *elem->lfo1.pitch;

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

				const float lfo1_env = pv->lfo1_env.tick();
				const float lfo1 = pv->lfo1_sample * lfo1_env;

				pv->gen1.next(pv->gen1_freq
					* (m_ctl.pitchbend + modwheel1 * lfo1));

				float gen1 = pv->gen1.value(k1);
				float gen2 = pv->gen1.value(k2);

				pv->lfo1_sample = pv->lfo1.sample(lfo1_freq
					* (1.0f + SWEEP_SCALE * *elem->lfo1.sweep * lfo1_env));

				// filters

				const float env1 = 0.5f * (1.0f + vel1
					* *elem->dcf1.envelope * pv->dcf1_env.tick());
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

				// volumes

				const float wid1 = elem->wid1.value(j);
				const float mid1 = 0.5f * (gen1 + gen2);
				const float sid1 = 0.5f * (gen1 - gen2);
				const float vol1 = vel1 * elem->vol1.value(j)
					* pv->dca1_env.tick();

				// outputs

				const float out1
					= vol1 * (mid1 + sid1 * wid1) * elem->pan1.value(j, 0);
				const float out2
					= vol1 * (mid1 - sid1 * wid1) * elem->pan1.value(j, 1);

				for (k = 0; k < m_nchannels; ++k) {
					const float dry = (k & 1 ? out2 : out1);
					const float wet = fxsend1 * dry;
					*v_outs[k]++ += dry - wet;
					*v_sfxs[k]++ += wet;
				}

				if (j == 0) {
					elem->aux1.panning = lfo1 * *elem->lfo1.panning;
					elem->aux1.volume  = lfo1 * *elem->lfo1.volume + 1.0f;
				}
			}

			nblock -= ngen;

			// voice ramps countdown

			pv->dca1_pre.process(ngen);

			// envelope countdowns

			if (pv->dca1_env.running && pv->dca1_env.frames == 0)
				elem->dca1.env.next(&pv->dca1_env);

			if (pv->dca1_env.stage == drumkv1_env::Idle || pv->gen1.isOver()) {
				if (pv->note >= 0)
					m_notes[pv->note] = 0;
				if (pv->group >= 0 && m_group[pv->group] == pv)
					m_group[pv->group] = 0;
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
			*m_del.delay, *m_del.feedb, *m_del.bpm);
	}

	// reverb
	if (m_nchannels > 1) {
		m_reverb.process(m_sfxs[0], m_sfxs[1], nframes, *m_rev.wet,
			*m_rev.feedb, *m_rev.room, *m_rev.damp, *m_rev.width);
	}

	// output mix-down
	for (k = 0; k < m_nchannels; ++k) {
		uint32_t n;
		// fx sends
		float *in  = m_sfxs[k];
		float *out = outs[k];
		for (n = 0; n < nframes; ++n)
			*out++ += *in++;
		// dynamics
		in = outs[k];
		// compressor
		if (int(*m_dyn.compress) > 0)
			m_comp[k].process(in, nframes);
		// limiter
		if (int(*m_dyn.limiter) > 0) {
			out = in;
			for (n = 0; n < nframes; ++n)
				*out++ = drumkv1_sigmoid(*in++);
		}
	}

	// post-processing
	m_phasor.process(nframes);

	elem = m_elem_list.next();
	while (elem) {
		elem->wid1.process(nframes);
		elem->pan1.process(nframes);
		elem->vol1.process(nframes);
		elem = elem->next();
	}

	m_controls.process(nframes);
}



//-------------------------------------------------------------------------
// drumkv1 - decl.
//

drumkv1::drumkv1 ( uint16_t nchannels, float srate )
{
	m_pImpl = new drumkv1_impl(this, nchannels, srate);
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
	m_pImpl->setCurrentElement(key);
}

int drumkv1::currentElement (void) const
{
	return m_pImpl->currentElement();
}


void drumkv1::clearElements (void)
{
	m_pImpl->clearElements();
}


void drumkv1::setSampleFile ( const char *pszSampleFile )
{
	m_pImpl->setSampleFile(pszSampleFile);
}

const char *drumkv1::sampleFile (void) const
{
	return m_pImpl->sampleFile();
}


drumkv1_sample *drumkv1::sample (void) const
{
	return m_pImpl->sample();
}


void drumkv1::setReverse ( bool bReverse )
{
	m_pImpl->setReverse(bReverse);
}

bool drumkv1::isReverse (void) const
{
	return m_pImpl->isReverse();
}


void drumkv1::setParamPort ( ParamIndex index, float *pfParam )
{
	m_pImpl->setParamPort(index, pfParam);
}

float *drumkv1::paramPort ( ParamIndex index ) const
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
	for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i) {
		const drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
		setParamPort(index, &(m_pElem->params[1][i]));
	}
}


int drumkv1_element::note (void) const
{
	return (m_pElem ? int(m_pElem->gen1.sample0) : -1);
}


void drumkv1_element::setSampleFile ( const char *pszSampleFile )
{
	if (m_pElem) {
		m_pElem->gen1_sample.close();
		if (pszSampleFile) {
		#if 0
			m_pElem->gen1.sample0 = *(m_pElem->gen1.sample);
		#endif
			m_pElem->gen1_sample.open(pszSampleFile,
				drumkv1_freq(m_pElem->gen1.sample0));
		}
	}
}


const char *drumkv1_element::sampleFile (void) const
{
	return (m_pElem ? m_pElem->gen1_sample.filename() : 0);
}


drumkv1_sample *drumkv1_element::sample (void) const
{
	return (m_pElem ? &(m_pElem->gen1_sample) : 0);
}


void drumkv1_element::setReverse ( bool bReverse )
{
	if (m_pElem) m_pElem->gen1_sample.setReverse(bReverse);
}


bool drumkv1_element::isReverse (void) const
{
	return (m_pElem ? m_pElem->gen1_sample.isReverse() : false);
}


void drumkv1_element::setParamPort ( drumkv1::ParamIndex index, float *pfParam )
{
	if (m_pElem == 0)
		return;

	switch (index) {
//	case drumkv1::GEN1_SAMPLE:   m_pElem->gen1.sample      = pfParam; break;
	case drumkv1::GEN1_REVERSE:  m_pElem->gen1.reverse     = pfParam; break;
	case drumkv1::GEN1_GROUP:    m_pElem->gen1.group       = pfParam; break;
	case drumkv1::GEN1_COARSE:   m_pElem->gen1.coarse      = pfParam; break;
	case drumkv1::GEN1_FINE:     m_pElem->gen1.fine        = pfParam; break;
	case drumkv1::GEN1_ENVTIME:  m_pElem->gen1.envtime     = pfParam; break;
	case drumkv1::DCF1_CUTOFF:   m_pElem->dcf1.cutoff      = pfParam; break;
	case drumkv1::DCF1_RESO:     m_pElem->dcf1.reso        = pfParam; break;
	case drumkv1::DCF1_TYPE:     m_pElem->dcf1.type        = pfParam; break;
	case drumkv1::DCF1_SLOPE:    m_pElem->dcf1.slope       = pfParam; break;
	case drumkv1::DCF1_ENVELOPE: m_pElem->dcf1.envelope    = pfParam; break;
	case drumkv1::DCF1_ATTACK:   m_pElem->dcf1.env.attack  = pfParam; break;
	case drumkv1::DCF1_DECAY1:   m_pElem->dcf1.env.decay1  = pfParam; break;
	case drumkv1::DCF1_LEVEL2:   m_pElem->dcf1.env.level2  = pfParam; break;
	case drumkv1::DCF1_DECAY2:   m_pElem->dcf1.env.decay2  = pfParam; break;
	case drumkv1::LFO1_SHAPE:    m_pElem->lfo1.shape       = pfParam; break;
	case drumkv1::LFO1_WIDTH:    m_pElem->lfo1.width       = pfParam; break;
	case drumkv1::LFO1_RATE:     m_pElem->lfo1.rate        = pfParam; break;
	case drumkv1::LFO1_SYNC:     m_pElem->lfo1.sync        = pfParam; break;
	case drumkv1::LFO1_SWEEP:    m_pElem->lfo1.sweep       = pfParam; break;
	case drumkv1::LFO1_PITCH:    m_pElem->lfo1.pitch       = pfParam; break;
	case drumkv1::LFO1_CUTOFF:   m_pElem->lfo1.cutoff      = pfParam; break;
	case drumkv1::LFO1_RESO:     m_pElem->lfo1.reso        = pfParam; break;
	case drumkv1::LFO1_PANNING:  m_pElem->lfo1.panning     = pfParam; break;
	case drumkv1::LFO1_VOLUME:   m_pElem->lfo1.volume      = pfParam; break;
	case drumkv1::LFO1_ATTACK:   m_pElem->lfo1.env.attack  = pfParam; break;
	case drumkv1::LFO1_DECAY1:   m_pElem->lfo1.env.decay1  = pfParam; break;
	case drumkv1::LFO1_LEVEL2:   m_pElem->lfo1.env.level2  = pfParam; break;
	case drumkv1::LFO1_DECAY2:   m_pElem->lfo1.env.decay2  = pfParam; break;
	case drumkv1::LFO1_BPMSYNC:  m_pElem->lfo1.bpmsync     = pfParam; break;
	case drumkv1::DCA1_VOLUME:   m_pElem->dca1.volume      = pfParam; break;
	case drumkv1::DCA1_ATTACK:   m_pElem->dca1.env.attack  = pfParam; break;
	case drumkv1::DCA1_DECAY1:   m_pElem->dca1.env.decay1  = pfParam; break;
	case drumkv1::DCA1_LEVEL2:   m_pElem->dca1.env.level2  = pfParam; break;
	case drumkv1::DCA1_DECAY2:   m_pElem->dca1.env.decay2  = pfParam; break;
	case drumkv1::OUT1_WIDTH:    m_pElem->out1.width       = pfParam; break;
	case drumkv1::OUT1_PANNING:  m_pElem->out1.panning     = pfParam; break;
	case drumkv1::OUT1_FXSEND:   m_pElem->out1.fxsend      = pfParam; break;
	case drumkv1::OUT1_VOLUME:   m_pElem->out1.volume      = pfParam; break;
	default: break;
	}
}


float *drumkv1_element::paramPort ( drumkv1::ParamIndex index )
{
	if (m_pElem == 0)
		return 0;

	float *pfParam = 0;

	switch (index) {
//	case drumkv1::GEN1_SAMPLE:   pfParam = m_pElem->gen1.sample;     break;
	case drumkv1::GEN1_REVERSE:  pfParam = m_pElem->gen1.reverse;    break;
	case drumkv1::GEN1_GROUP:    pfParam = m_pElem->gen1.group;      break;
	case drumkv1::GEN1_COARSE:   pfParam = m_pElem->gen1.coarse;     break;
	case drumkv1::GEN1_FINE:     pfParam = m_pElem->gen1.fine;       break;
	case drumkv1::GEN1_ENVTIME:  pfParam = m_pElem->gen1.envtime;    break;
	case drumkv1::DCF1_CUTOFF:   pfParam = m_pElem->dcf1.cutoff;     break;
	case drumkv1::DCF1_RESO:     pfParam = m_pElem->dcf1.reso;       break;
	case drumkv1::DCF1_TYPE:     pfParam = m_pElem->dcf1.type;       break;
	case drumkv1::DCF1_SLOPE:    pfParam = m_pElem->dcf1.slope;      break;
	case drumkv1::DCF1_ENVELOPE: pfParam = m_pElem->dcf1.envelope;   break;
	case drumkv1::DCF1_ATTACK:   pfParam = m_pElem->dcf1.env.attack; break;
	case drumkv1::DCF1_DECAY1:   pfParam = m_pElem->dcf1.env.decay1; break;
	case drumkv1::DCF1_LEVEL2:   pfParam = m_pElem->dcf1.env.level2; break;
	case drumkv1::DCF1_DECAY2:   pfParam = m_pElem->dcf1.env.decay2; break;
	case drumkv1::LFO1_SHAPE:    pfParam = m_pElem->lfo1.shape;      break;
	case drumkv1::LFO1_WIDTH:    pfParam = m_pElem->lfo1.width;      break;
	case drumkv1::LFO1_RATE:     pfParam = m_pElem->lfo1.rate;       break;
	case drumkv1::LFO1_SYNC:     pfParam = m_pElem->lfo1.sync;       break;
	case drumkv1::LFO1_SWEEP:    pfParam = m_pElem->lfo1.sweep;      break;
	case drumkv1::LFO1_PITCH:    pfParam = m_pElem->lfo1.pitch;      break;
	case drumkv1::LFO1_CUTOFF:   pfParam = m_pElem->lfo1.cutoff;     break;
	case drumkv1::LFO1_RESO:     pfParam = m_pElem->lfo1.reso;       break;
	case drumkv1::LFO1_PANNING:  pfParam = m_pElem->lfo1.panning;    break;
	case drumkv1::LFO1_VOLUME:   pfParam = m_pElem->lfo1.volume;     break;
	case drumkv1::LFO1_ATTACK:   pfParam = m_pElem->lfo1.env.attack; break;
	case drumkv1::LFO1_DECAY1:   pfParam = m_pElem->lfo1.env.decay1; break;
	case drumkv1::LFO1_LEVEL2:   pfParam = m_pElem->lfo1.env.level2; break;
	case drumkv1::LFO1_DECAY2:   pfParam = m_pElem->lfo1.env.decay2; break;
	case drumkv1::LFO1_BPMSYNC:  pfParam = m_pElem->lfo1.bpmsync;    break;
	case drumkv1::DCA1_VOLUME:   pfParam = m_pElem->dca1.volume;     break;
	case drumkv1::DCA1_ATTACK:   pfParam = m_pElem->dca1.env.attack; break;
	case drumkv1::DCA1_DECAY1:   pfParam = m_pElem->dca1.env.decay1; break;
	case drumkv1::DCA1_LEVEL2:   pfParam = m_pElem->dca1.env.level2; break;
	case drumkv1::DCA1_DECAY2:   pfParam = m_pElem->dca1.env.decay2; break;
	case drumkv1::OUT1_WIDTH:    pfParam = m_pElem->out1.width;      break;
	case drumkv1::OUT1_PANNING:  pfParam = m_pElem->out1.panning;    break;
	case drumkv1::OUT1_FXSEND:   pfParam = m_pElem->out1.fxsend;     break;
	case drumkv1::OUT1_VOLUME:   pfParam = m_pElem->out1.volume;     break;
	default: break;
	}

	return pfParam;
}


void drumkv1_element::setParamValue (
	drumkv1::ParamIndex index, float fValue, int pset )
{
	if (index < drumkv1::NUM_ELEMENT_PARAMS && index != drumkv1::GEN1_SAMPLE)
		m_pElem->params[pset][index] = fValue;
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
		const float	fOldValue = m_pElem->params[1][index];
		const float fNewValue = m_pElem->params[2][index];
		m_pElem->params[2][index] = fOldValue;
		if (bSwap)
			m_pElem->params[1][index] = fNewValue;
		else
			m_pElem->params[0][index] = fOldValue;
	}
}


// scalar converter helpers (static)

float drumkv1::lfo_rate_bpm ( float bpm )
{
	const float freq_bpm = ::fmaxf(LFO_FREQ_MIN, (bpm / 60.f));
	return ::sqrtf((freq_bpm - LFO_FREQ_MIN) / (LFO_FREQ_MAX - LFO_FREQ_MIN));
}


// end of drumkv1.cpp
