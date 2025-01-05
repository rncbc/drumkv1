// drumkv1_ui.cpp
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

#include "drumkv1_ui.h"

#include "drumkv1_param.h"


//-------------------------------------------------------------------------
// drumkv1_ui - decl.
//

drumkv1_ui::drumkv1_ui ( drumkv1 *pDrumk, bool bPlugin )
	: m_pDrumk(pDrumk), m_bPlugin(bPlugin)
{
}


drumkv1 *drumkv1_ui::instance (void) const
{
	return m_pDrumk;
}


drumkv1_element *drumkv1_ui::addElement ( int key )
{
	return m_pDrumk->addElement(key);
}

drumkv1_element *drumkv1_ui::element ( int key ) const
{
	return m_pDrumk->element(key);
}

void drumkv1_ui::removeElement ( int key )
{
	m_pDrumk->removeElement(key);
}


void drumkv1_ui::setCurrentElement ( int key )
{
	m_pDrumk->setCurrentElement(key);
}

int drumkv1_ui::currentElement (void) const
{
	return m_pDrumk->currentElement();
}


void drumkv1_ui::clearElements (void)
{
	m_pDrumk->clearElements();
}


bool drumkv1_ui::isPlugin (void) const
{
	return m_bPlugin;
}


void drumkv1_ui::setSampleFile ( const char *pszSampleFile )
{
	m_pDrumk->setSampleFile(pszSampleFile, true);
}

const char *drumkv1_ui::sampleFile (void) const
{
	return m_pDrumk->sampleFile();
}


drumkv1_sample *drumkv1_ui::sample (void) const
{
	return m_pDrumk->sample();
}


void drumkv1_ui::setReverse ( bool bReverse )
{
	m_pDrumk->setReverse(bReverse);
}

bool drumkv1_ui::isReverse (void) const
{
	return m_pDrumk->isReverse();
}


void drumkv1_ui::setOffset ( bool bOffset )
{
	m_pDrumk->setOffset(bOffset);
}

bool drumkv1_ui::isOffset (void) const
{
	return m_pDrumk->isOffset();
}


void drumkv1_ui::setOffsetRange ( uint32_t iOffsetStart, uint32_t iOffsetEnd )
{
	m_pDrumk->setOffsetRange(iOffsetStart, iOffsetEnd, true);
}

uint32_t drumkv1_ui::offsetStart (void) const
{
	return m_pDrumk->offsetStart();
}

uint32_t drumkv1_ui::offsetEnd (void) const
{
	return m_pDrumk->offsetEnd();
}


bool drumkv1_ui::newPreset (void)
{
	return drumkv1_param::newPreset(m_pDrumk);
}


bool drumkv1_ui::loadPreset ( const QString& sFilename )
{
	return drumkv1_param::loadPreset(m_pDrumk, sFilename);
}

bool drumkv1_ui::savePreset ( const QString& sFilename )
{
	return drumkv1_param::savePreset(m_pDrumk, sFilename);
}


void drumkv1_ui::setParamValue ( drumkv1::ParamIndex index, float fValue )
{
	m_pDrumk->setParamValue(index, fValue);
}

float drumkv1_ui::paramValue ( drumkv1::ParamIndex index ) const
{
	return m_pDrumk->paramValue(index);
}


drumkv1_controls *drumkv1_ui::controls (void) const
{
	return m_pDrumk->controls();
}


drumkv1_programs *drumkv1_ui::programs (void) const
{
	return m_pDrumk->programs();
}


void drumkv1_ui::resetParamValues ( bool bSwap )
{
	return m_pDrumk->resetParamValues(bSwap);
}


void drumkv1_ui::reset (void)
{
	return m_pDrumk->reset();
}


void drumkv1_ui::updatePreset ( bool bDirty )
{
	m_pDrumk->updatePreset(bDirty);
}


void drumkv1_ui::updateParam ( drumkv1::ParamIndex index )
{
	m_pDrumk->updateParam(index);
}


void drumkv1_ui::midiInEnabled ( bool bEnabled )
{
	m_pDrumk->midiInEnabled(bEnabled);
}


uint32_t drumkv1_ui::midiInCount (void)
{
	return m_pDrumk->midiInCount();
}


void drumkv1_ui::directNoteOn ( int note, int vel )
{
	m_pDrumk->directNoteOn(note, vel);
}


void drumkv1_ui::setTuningEnabled ( bool enabled )
{
	m_pDrumk->setTuningEnabled(enabled);
}

bool drumkv1_ui::isTuningEnabled (void) const
{
	return m_pDrumk->isTuningEnabled();
}


void drumkv1_ui::setTuningRefPitch ( float refPitch )
{
	m_pDrumk->setTuningRefPitch(refPitch);
}

float drumkv1_ui::tuningRefPitch (void) const
{
	return m_pDrumk->tuningRefPitch();
}


void drumkv1_ui::setTuningRefNote ( int refNote )
{
	m_pDrumk->setTuningRefNote(refNote);
}

int drumkv1_ui::tuningRefNote (void) const
{
	return m_pDrumk->tuningRefNote();
}


void drumkv1_ui::setTuningScaleFile ( const char *pszScaleFile )
{
	m_pDrumk->setTuningScaleFile(pszScaleFile);
}

const char *drumkv1_ui::tuningScaleFile (void) const
{
	return m_pDrumk->tuningScaleFile();
}


void drumkv1_ui::setTuningKeyMapFile ( const char *pszKeyMapFile )
{
	m_pDrumk->setTuningKeyMapFile(pszKeyMapFile);
}

const char *drumkv1_ui::tuningKeyMapFile (void) const
{
	return m_pDrumk->tuningKeyMapFile();
}


void drumkv1_ui::resetTuning (void)
{
	m_pDrumk->resetTuning();
}


// MIDI note/octave name helper (static).
QString drumkv1_ui::noteName ( int note )
{
	static const char *s_notes[] =
	{
		QT_TR_NOOP("C"),
		QT_TR_NOOP("C#/Db"),
		QT_TR_NOOP("D"),
		QT_TR_NOOP("D#/Eb"),
		QT_TR_NOOP("E"),
		QT_TR_NOOP("F"),
		QT_TR_NOOP("F#/Gb"),
		QT_TR_NOOP("G"),
		QT_TR_NOOP("G#/Ab"),
		QT_TR_NOOP("A"),
		QT_TR_NOOP("A#/Bb"),
		QT_TR_NOOP("B")
	};

	return QString("%1 %2").arg(s_notes[note % 12]).arg((note / 12) - 1);
}


// end of drumkv1_ui.cpp
