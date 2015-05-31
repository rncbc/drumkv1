// drumkv1.h
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

#ifndef __drumkv1_h
#define __drumkv1_h

#include "config.h"

#include <stdint.h>

// forward declarations
class drumkv1_impl;
class drumkv1_elem;
class drumkv1_element;
class drumkv1_sample;
class drumkv1_controls;
class drumkv1_programs;


//-------------------------------------------------------------------------
// drumkv1 - decl.
//

class drumkv1
{
public:

	drumkv1(uint16_t iChannels = 2, uint32_t iSampleRate = 44100);

	~drumkv1();

	void setChannels(uint16_t iChannels);
	uint16_t channels() const;

	void setSampleRate(uint32_t iSampleRate);
	uint32_t sampleRate() const;

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

	enum ParamIndex	 {

		GEN1_SAMPLE = 0,
		GEN1_REVERSE,
		GEN1_GROUP,
		GEN1_COARSE,
		GEN1_FINE,
		GEN1_ENVTIME,
		DCF1_CUTOFF,
		DCF1_RESO,
		DCF1_TYPE,
		DCF1_SLOPE,
		DCF1_ENVELOPE,
		DCF1_ATTACK,
		DCF1_DECAY1,
		DCF1_LEVEL2,
		DCF1_DECAY2,
		LFO1_SHAPE,
		LFO1_WIDTH,
		LFO1_RATE,
		LFO1_SWEEP,
		LFO1_PITCH,
		LFO1_CUTOFF,
		LFO1_RESO,
		LFO1_PANNING,
		LFO1_VOLUME,
		LFO1_ATTACK,
		LFO1_DECAY1,
		LFO1_LEVEL2,
		LFO1_DECAY2,
		DCA1_VOLUME,
		DCA1_ATTACK,
		DCA1_DECAY1,
		DCA1_LEVEL2,
		DCA1_DECAY2,
		OUT1_WIDTH,
		OUT1_PANNING,
		OUT1_VOLUME,

		NUM_ELEMENT_PARAMS,

		DEF1_PITCHBEND = NUM_ELEMENT_PARAMS,
		DEF1_MODWHEEL,
		DEF1_PRESSURE,
		DEF1_VELOCITY,
		DEF1_CHANNEL,
		DEF1_NOTEOFF,

		CHO1_WET,
		CHO1_DELAY,
		CHO1_FEEDB,
		CHO1_RATE,
		CHO1_MOD,
		FLA1_WET,
		FLA1_DELAY,
		FLA1_FEEDB,
		FLA1_DAFT,
		PHA1_WET,
		PHA1_RATE,
		PHA1_FEEDB,
		PHA1_DEPTH,
		PHA1_DAFT,
		DEL1_WET,
		DEL1_DELAY,
		DEL1_FEEDB,
		DEL1_BPM,
		DEL1_BPMSYNC,
		DEL1_BPMHOST,
		REV1_WET,
		REV1_ROOM,
		REV1_DAMP,
		REV1_FEEDB,
		REV1_WIDTH,
		DYN1_COMPRESS,
		DYN1_LIMITER,

		NUM_PARAMS
	};

	void setParamPort(ParamIndex index, float *pfParam);
	float *paramPort(ParamIndex index) const;

	void setParamValue(ParamIndex index, float fValue);
	float paramValue(ParamIndex index) const;

	void resetParamValues(bool bSwap);
	void reset();

	drumkv1_controls *controls() const;
	drumkv1_programs *programs() const;

	void process_midi(uint8_t *data, uint32_t size);
	void process(float **ins, float **outs, uint32_t nframes);

private:

	drumkv1_impl *m_pImpl;
};


//-------------------------------------------------------------------------
// drumkv1_element - decl.
//

class drumkv1_element
{
public:

	drumkv1_element(drumkv1_elem *pElem);

	int note() const;

	void setSampleFile(const char *pszSampleFile);
	const char *sampleFile() const;

	drumkv1_sample *sample() const;

	void setReverse(bool bReverse);
	bool isReverse() const;

	void setParamPort(drumkv1::ParamIndex index, float *pfParam = 0);
	float *paramPort(drumkv1::ParamIndex index);

	void setParamValue(drumkv1::ParamIndex index, float fValue, int pset = 1);
	float paramValue(drumkv1::ParamIndex index, int pset = 1);

	void resetParamValues(bool bSwap);

private:

	drumkv1_elem *m_pElem;
};


#endif// __drumkv1_h

// end of drumkv1.h
