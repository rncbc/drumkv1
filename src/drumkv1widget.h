// drumkv1widget.h
//
/****************************************************************************
   Copyright (C) 2012-2017, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "drumkv1_config.h"
#include "drumkv1_sched.h"

#include "drumkv1_ui.h"


// forward decls.
class drumkv1widget_sched;


//-------------------------------------------------------------------------
// drumkv1widget - decl.
//

class drumkv1widget : public QWidget
{
	Q_OBJECT

public:

	// Constructor
	drumkv1widget(QWidget *pParent = 0, Qt::WindowFlags wflags = 0);

	// Destructor.
	virtual ~drumkv1widget();

	// Open/close the scheduler/work notifier.
	void openSchedNotifier();
	void closeSchedNotifier();

	// Param port accessors.
	void setParamValue(
		drumkv1::ParamIndex index, float fValue, bool bDefault = false);
	float paramValue(drumkv1::ParamIndex index) const;

	// Param kbob (widget) mapper.
	void setParamKnob(drumkv1::ParamIndex index, drumkv1widget_param *pKnob);
	drumkv1widget_param *paramKnob(drumkv1::ParamIndex index) const;

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

	// Sample loader.
	void loadSample(const QString& sFilename);

	// All element clear.
	void clearElements();

	// Element activation.
	void activateElement(bool bOpenSample = false);

	// Element sample requester.
	void doubleClickElement();

	// Element sample loader.
	void loadSampleElement(const QString& sFilename);

	// Element playback (direct note-on/off).
	void playElement(void);

	// Element deactivation.
	void resetElement(void);

	// Common context menu.
	void contextMenuRequest(const QPoint& pos);

	// Reset param knobs to default value.
	void resetParams();

	// Swap params A/B.
	void swapParams(bool bOn);

	// Notification updater.
	void updateSchedNotify(int stype, int sid);

	// MIDI In LED timeout.
	void midiInLedTimeout();

	// Param knob context menu.
	void paramContextMenu(const QPoint& pos);

	// Menu actions.
	void helpConfigure();

	void helpAbout();
	void helpAboutQt();

protected:

	// Synth engine accessor.
	virtual drumkv1_ui *ui_instance() const = 0;

	// Reload all elements.
	void refreshElements();

	// Update element (as current).
	void updateElement();

	// Reset swap params A/B group.
	void resetSwapParams();

	// Initialize param values.
	void updateParamValues(uint32_t nparams);

	// Reset all param/knob default values.
	void resetParamValues(uint32_t nparams);
	void resetParamKnobs(uint32_t nparams);

	// (En|Dis)able all param/knobs.
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

	// Update local tied widgets.
	void updateParamEx(drumkv1::ParamIndex index, float fValue);

	// Update scheduled controllers param/knob widgets.
	void updateSchedParam(drumkv1::ParamIndex index, float fValue);

	// Preset status updater.
	void updateLoadPreset(const QString& sPreset);

	// Dirty flag (overridable virtual) methods.
	virtual void updateDirtyPreset(bool bDirtyPreset);

	// Show/hide dget handlers.
	void showEvent(QShowEvent *pShowEvent);
	void hideEvent(QHideEvent *pHideEvent);

private:

	// Instance variables.
	Ui::drumkv1widget m_ui;

	drumkv1widget_sched *m_sched_notifier;

	QHash<drumkv1::ParamIndex, drumkv1widget_param *> m_paramKnobs;
	QHash<drumkv1widget_param *, drumkv1::ParamIndex> m_knobParams;

	float m_params_ab[drumkv1::NUM_PARAMS];

	int m_iUpdate;
};


//-------------------------------------------------------------------------
// drumkv1widget_sched - worker/schedule proxy decl.
//

class drumkv1widget_sched : public QObject
{
	Q_OBJECT

public:

	// ctor.
	drumkv1widget_sched(drumkv1 *pDrumk, QObject *pParent = NULL)
		: QObject(pParent), m_notifier(pDrumk, this) {}

signals:

	// Notification signal.
	void notify(int stype, int sid);

protected:

	// Notififier visitor.
	class Notifier : public drumkv1_sched_notifier
	{
	public:

		Notifier(drumkv1 *pDrumk, drumkv1widget_sched *pSched)
			: drumkv1_sched_notifier(pDrumk), m_pSched(pSched) {}

		void notify(drumkv1_sched::Type stype, int sid) const
			{ m_pSched->emit_notify(stype, sid); }

	private:

		drumkv1widget_sched *m_pSched;
	};

	// Notification method.
	void emit_notify(drumkv1_sched::Type stype, int sid)
		{ emit notify(int(stype), sid); }

private:

	// Instance variables.
	Notifier m_notifier;
};


#endif	// __drumkv1widget_h

// end of drumkv1widget.h
