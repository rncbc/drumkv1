// drumkv1_ui.h
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

#ifndef __drumkv1_ui_h
#define __drumkv1_ui_h

#include "drumkv1.h"

#include <QString>


//-------------------------------------------------------------------------
// drumkv1_ui - decl.
//

class drumkv1_ui
{
public:

	drumkv1_ui(drumkv1 *pDrumk, bool bPlugin);

	drumkv1 *instance() const;

	bool isPlugin() const;

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

	void setOffset(bool bOffset);
	bool isOffset() const;

	void setOffsetRange(uint32_t iOffsetStart, uint32_t iOffsetEnd);
	uint32_t offsetStart() const;
	uint32_t offsetEnd() const;

	bool newPreset();

	bool loadPreset(const QString& sFilename);
	bool savePreset(const QString& sFilename);

	void setParamValue(drumkv1::ParamIndex index, float fValue);
	float paramValue(drumkv1::ParamIndex index) const;

	drumkv1_controls *controls() const;
	drumkv1_programs *programs() const;

	void resetParamValues(bool bSwap);
	void reset();

	void updatePreset(bool bDirty);

	void midiInEnabled(bool bEnabled);
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

	// MIDI note/octave name helper.
	static QString noteName(int note);

private:

	drumkv1 *m_pDrumk;

	bool m_bPlugin;
};


#endif// __drumkv1_ui_h

// end of drumkv1_ui.h
