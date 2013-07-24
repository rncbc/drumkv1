// drumkv1widget.h
//
/****************************************************************************
   Copyright (C) 2012-2013, rncbc aka Rui Nuno Capela. All rights reserved.

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

// forward decl.
class QDomElement;
class QDomDocument;


//-------------------------------------------------------------------------
// drumkv1_map_path - abstract/absolute path functors.

class drumkv1_map_path
{
public:

	virtual QString absolutePath(const QString& sAbstractPath) const
		{ return sAbstractPath; }
	virtual QString abstractPath(const QString& sAbsolutePath) const
		{ return sAbsolutePath; }
};


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

	// Preset init.
	void initPreset();
	// Preset clear.
	void clearPreset();

	// Dirty close prompt.
	bool queryClose();

	// Current selected note helpers.
	int currentNote() const;
	QString currentNoteName() const;

	// MIDI note/octave name helpers.
	static QString noteName(int note);
	static QString completeNoteName(int note);

	// Element serialization methods.
	static void loadElements(drumkv1 *pDrumk,
		const QDomElement& eElements,
		const drumkv1_map_path& mapPath = drumkv1_map_path());
	static void saveElements(drumkv1 *pDrumk,
		QDomDocument& doc, QDomElement& eElements,
		const drumkv1_map_path& mapPath = drumkv1_map_path());

public slots:

	// Preset file I/O.
	void loadPreset(const QString& sFilename);
	void savePreset(const QString& sFilename);

protected slots:

	// Preset renewal.
	void newPreset();

	// Param knob (widget) slots.
	void paramChanged(float fValue);

	// Sample clear slot.
	void clearSample();

	// Sample openner.
	void openSample();

	// Sample loader slot.
	void loadSample(const QString& sFilename);

	// All element clear.
	void clearElements();

	// Element activation.
	void activateElement(bool bOpenSample = false);

	// Element sample loader.
	void doubleClickElement();

	// Element deactivation.
	void resetElement(void);

	// Common context menu.
	void contextMenuRequest(const QPoint& pos);

	// Reset param knobs to default value.
	void resetParams();

	// Swap params A/B.
	void swapParams(bool bOn);

	// Menu actions.
	void helpAbout();
	void helpAboutQt();

protected:

	// Synth engine accessor.
	virtual drumkv1 *instance() const = 0;

	// Reload all elements.
	void refreshElements();

	// Reset swap params A/B group.
	void resetSwapParams();

	// Reset all param/knob default values.
	void resetParamValues(uint32_t nparams);
	void resetParamKnobs(uint32_t nparams);

	// (En|Dis)able/ all param/knobs.
	void activateParamKnobs(bool bEnabled);
	void activateParamKnobsGroupBox(QGroupBox *pGroupBox, bool bEnable);

	// Sample file clearance.
	void clearSampleFile();

	// Sample loader slot.
	void loadSampleFile(const QString& sFilename);

	// Sample updater.
	void updateSample(drumkv1_sample *pSample, bool bDirty = false);

	// Param port method.
	virtual void updateParam(drumkv1::ParamIndex index, float fValue) const = 0;

	// Dirty flag (overridable virtual) methods.
	virtual void updateDirtyPreset(bool bDirtyPreset);

private:

	// Instance variables.
	Ui::drumkv1widget m_ui;

	drumkv1widget_config m_config;

	QHash<drumkv1::ParamIndex, drumkv1widget_knob *> m_paramKnobs;
	QHash<drumkv1widget_knob *, drumkv1::ParamIndex> m_knobParams;

	float m_params_ab[drumkv1::NUM_PARAMS];

	int m_iUpdate;
};


#endif	// __drumkv1widget_h

// end of drumkv1widget.h
