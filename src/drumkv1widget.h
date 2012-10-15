// drumkv1widget.h
//
/****************************************************************************
   Copyright (C) 2012, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __drumkv1widget_h
#define __drumkv1widget_h

#include "ui_drumkv1widget.h"

#include "drumkv1widget_config.h"

#include "drumkv1.h"

#include <QHash>



//-------------------------------------------------------------------------
// drumkv1widget - decl.
//

class drumkv1widget : public QWidget
{
	Q_OBJECT

public:

	// Constructor
	drumkv1widget(QWidget *pParent = 0, Qt::WindowFlags wflags = 0);

	// Param port accessors.
	void setParamValue(drumkv1::ParamIndex index, float fValue);
	float paramValue(drumkv1::ParamIndex index) const;

	// Param kbob (widget) mapper.
	void setParamKnob(drumkv1::ParamIndex index, drumkv1widget_knob *pKnob);
	drumkv1widget_knob *paramKnob(drumkv1::ParamIndex index) const;

	// MIDI note/octave name helper.
	static QString noteName(int note, bool bDrums);

	// Dirty close prompt,
	bool queryClose();

public slots:

	// Preset file I/O.
	void loadPreset(const QString& sFilename);
	void savePreset(const QString& sFilename);

protected slots:

	// Preset clear.
	void newPreset();

	// Param knob (widget) slots.
	void paramChanged(float fValue);

	// Sample clear slot.
	void clearSample();

	// Sample loader slot.
	void loadSample(const QString& sFilename);

	// Element activation.
	void activateElement();

	// Menu actions.
	void helpAbout();
	void helpAboutQt();

protected:

	// Synth engine accessor.
	virtual drumkv1 *instance() const = 0;

	// Preset init.
	void initPreset();

	// Reload all elements.
	void refreshElement();

	// Reset all param/knob default values.
	void resetParamValues();
	void resetParamKnobs();

	// Sample filename retriever.
	QString sampleFile() const;

	// Sample updater.
	void updateSample(drumkv1_sample *pSample, bool bDirty = false);

	// Param port method.
	virtual void updateParam(drumkv1::ParamIndex index, float fValue) const = 0;

private:

	// Instance variables.
	Ui::drumkv1widget m_ui;

	drumkv1widget_config m_config;

	QHash<drumkv1::ParamIndex, drumkv1widget_knob *> m_paramKnobs;
	QHash<drumkv1widget_knob *, drumkv1::ParamIndex> m_knobParams;

	int m_iUpdate;
};


#endif	// __drumkv1widget_h

// end of drumkv1widget.h
