// drumkv1.h
//
/****************************************************************************
   Copyright (C) 2012-2025, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include <cstdint>


// forward declarations
class drumkv1_impl;
class drumkv1_port;
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

	drumkv1(uint16_t nchannels = 2, float srate = 44100.0f, uint32_t nsize = 1024);

	virtual ~drumkv1();

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
	void setCurrentElementEx(int key);
	int currentElement() const;

	void currentElementTest();

	void resetElements();
	void clearElements();

	void setSampleFile(const char *pszSampleFile, bool bSync = false);
	const char *sampleFile() const;

	drumkv1_sample *sample() const;

	void setReverse(bool bReverse, bool bSync = false);
	bool isReverse() const;

	void setOffset(bool bOffset, bool bSync = false);
	bool isOffset() const;

	void setOffsetRange(uint32_t iOffsetStart, uint32_t iOffsetEnd, bool bSync = false);
	uint32_t offsetStart() const;
	uint32_t offsetEnd() const;

	void setTempo(float bpm);
	float tempo() const;

	enum ParamIndex	 {

		GEN1_SAMPLE = 0,
		GEN1_REVERSE,
		GEN1_OFFSET,
		GEN1_OFFSET_1,
		GEN1_OFFSET_2,
		GEN1_GROUP,
		GEN1_COARSE,
		GEN1_FINE,
		GEN1_ENVTIME,
		DCF1_ENABLED,
		DCF1_CUTOFF,
		DCF1_RESO,
		DCF1_TYPE,
		DCF1_SLOPE,
		DCF1_ENVELOPE,
		DCF1_ATTACK,
		DCF1_DECAY1,
		DCF1_LEVEL2,
		DCF1_DECAY2,
		LFO1_ENABLED,
		LFO1_SHAPE,
		LFO1_WIDTH,
		LFO1_BPM,
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
		DCA1_ENABLED,
		DCA1_VOLUME,
		DCA1_ATTACK,
		DCA1_DECAY1,
		DCA1_LEVEL2,
		DCA1_DECAY2,
		OUT1_WIDTH,
		OUT1_PANNING,
		OUT1_FXSEND,
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
	drumkv1_port *paramPort(ParamIndex index) const;

	void setParamValue(ParamIndex index, float fValue);
	float paramValue(ParamIndex index) const;

	void resetParamValues(bool bSwap);

	bool running(bool on);

	void stabilize();
	void reset();

	drumkv1_controls *controls() const;
	drumkv1_programs *programs() const;

	void process_midi(uint8_t *data, uint32_t size);
	void process(float **ins, float **outs, uint32_t nframes);

	virtual void updatePreset(bool bDirty) = 0;
	virtual void updateParam(ParamIndex index) = 0;
	virtual void updateParams() = 0;

	virtual void updateSample() = 0;

	virtual void updateOffsetRange() = 0;

	virtual void selectSample(int key) = 0;

	void midiInEnabled(bool on);
	uint32_t midiInCount();

	void directNoteOn(int note, int vel);

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

	virtual void updateTuning() = 0;

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
	uint32_t length() const;

	void setReverse(bool bReverse);
	bool isReverse() const;

	void setOffset(bool bOffset);
	bool isOffset() const;

	void setOffsetRange(uint32_t iOffsetStart, uint32_t iOffsetEnd);
	uint32_t offsetStart() const;
	uint32_t offsetEnd() const;

	void setParamPort(drumkv1::ParamIndex index, float *pfParam);
	drumkv1_port *paramPort(drumkv1::ParamIndex index);

	void setParamValue(drumkv1::ParamIndex index, float fValue, int pset = 1);
	float paramValue(drumkv1::ParamIndex index, int pset = 1);

	void resetParamValues(bool bSwap);

	void sampleReverseTest();
	void sampleReverseSync();

	void sampleOffsetTest();
	void sampleOffsetSync();
	void sampleOffsetRangeSync();

	void updateEnvTimes();

private:

	drumkv1_elem *m_pElem;
};


#endif// __drumkv1_h

// end of drumkv1.h

