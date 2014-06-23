// drumkv1widget.cpp
//
/****************************************************************************
   Copyright (C) 2012-2014, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "drumkv1widget.h"
#include "drumkv1_param.h"

#include "drumkv1widget_config.h"

#include <QMessageBox>
#include <QDir>


//-------------------------------------------------------------------------
// drumkv1widget - impl.
//

// Constructor.
drumkv1widget::drumkv1widget ( QWidget *pParent, Qt::WindowFlags wflags )
	: QWidget(pParent, wflags)
{
	Q_INIT_RESOURCE(drumkv1);

#if QT_VERSION >= 0x050000
	// HACK: Dark themes grayed/disabled color group fix...
	QPalette pal;
	if (pal.base().color().value() < 0x7f) {
		const QColor& color = pal.window().color();
		const int iGroups = int(QPalette::Active | QPalette::Inactive) + 1;
		for (int i = 0; i < iGroups; ++i) {
			const QPalette::ColorGroup group = QPalette::ColorGroup(i);
			pal.setBrush(group, QPalette::Light,    color.lighter(150));
			pal.setBrush(group, QPalette::Midlight, color.lighter(120));
			pal.setBrush(group, QPalette::Dark,     color.darker(150));
			pal.setBrush(group, QPalette::Mid,      color.darker(120));
			pal.setBrush(group, QPalette::Shadow,   color.darker(200));
		}
		pal.setColor(QPalette::Disabled, QPalette::ButtonText, pal.mid().color());
		QWidget::setPalette(pal);
	}
#endif

	m_ui.setupUi(this);

	// Init swapable params A/B to default.
	for (uint32_t i = 0; i < drumkv1::NUM_PARAMS; ++i)
		m_params_ab[i] = drumkv1_param::paramDefaultValue(drumkv1::ParamIndex(i));

	// Start clean.
	m_iUpdate = 0;

	// Replicate the stacked/pages
	for (int iTab = 0; iTab < m_ui.StackedWidget->count(); ++iTab)
		m_ui.TabBar->addTab(m_ui.StackedWidget->widget(iTab)->windowTitle());

	// Swappable params A/B group.
	QButtonGroup *pSwapParamsGroup = new QButtonGroup(this);
	pSwapParamsGroup->addButton(m_ui.SwapParamsAButton);
	pSwapParamsGroup->addButton(m_ui.SwapParamsBButton);
	pSwapParamsGroup->setExclusive(true);
	m_ui.SwapParamsAButton->setChecked(true);

	// Wave shapes.
	QStringList shapes;
	shapes << tr("Pulse");
	shapes << tr("Saw");
	shapes << tr("Sine");
	shapes << tr("Rand");
	shapes << tr("Noise");

	m_ui.Lfo1ShapeKnob->insertItems(0, shapes);

	// Filter types.
	QStringList types;
	types << tr("LPF");
	types << tr("BPF");
	types << tr("HPF");
	types << tr("BRF");

	m_ui.Dcf1TypeKnob->insertItems(0, types);

	// Filter slopes.
	QStringList slopes;
	slopes << tr("12dB/oct");
	slopes << tr("24dB/oct");

	m_ui.Dcf1SlopeKnob->insertItems(0, slopes);

	// Channel filters
	QStringList channels;
	channels << tr("Omni");
	for (int iChannel = 0; iChannel < 16; ++iChannel)
		channels << QString::number(iChannel + 1);

	m_ui.Def1ChannelKnob->insertItems(0, channels);

	// Noteoff modes.
	QStringList modes;
	modes << tr("Disabled");
	modes << tr("Enabled");

	m_ui.Def1NoteoffKnob->insertItems(0, modes);

	// Dynamic states.
	QStringList states;
	states << tr("Off");
	states << tr("On");

	m_ui.Gen1ReverseKnob->insertItems(0, states);

	m_ui.Dyn1CompressKnob->insertItems(0, states);
	m_ui.Dyn1LimiterKnob->insertItems(0, states);

	// Special values
	const QString& sOff = states.first();
	m_ui.Cho1WetKnob->setSpecialValueText(sOff);
	m_ui.Fla1WetKnob->setSpecialValueText(sOff);
	m_ui.Pha1WetKnob->setSpecialValueText(sOff);
	m_ui.Del1WetKnob->setSpecialValueText(sOff);
	m_ui.Rev1WetKnob->setSpecialValueText(sOff);

	const QString& sAuto = tr("Auto");
	m_ui.Gen1EnvTimeKnob->setSpecialValueText(sAuto);
	m_ui.Del1BpmKnob->setSpecialValueText(sAuto);

	// GEN group limits. [0=off, 1..128]
	m_ui.Gen1GroupKnob->setSpecialValueText(sOff);
	m_ui.Gen1GroupKnob->setMaximum(1.28f);

	// GEN octave limits.
	m_ui.Gen1CoarseKnob->setMinimum(-4.0f);
	m_ui.Gen1CoarseKnob->setMaximum(+4.0f);

	// GEN tune limits.
	m_ui.Gen1FineKnob->setMinimum(-1.0f);
	m_ui.Gen1FineKnob->setMaximum(+1.0f);

	// DCF volume (env.amount) limits.
	m_ui.Dcf1EnvelopeKnob->setMinimum(-1.0f);
	m_ui.Dcf1EnvelopeKnob->setMaximum(+1.0f);

	// LFO parameter limits.
	m_ui.Lfo1SweepKnob->setMinimum(-1.0f);
	m_ui.Lfo1SweepKnob->setMaximum(+1.0f);
	m_ui.Lfo1CutoffKnob->setMinimum(-1.0f);
	m_ui.Lfo1CutoffKnob->setMaximum(+1.0f);
	m_ui.Lfo1ResoKnob->setMinimum(-1.0f);
	m_ui.Lfo1ResoKnob->setMaximum(+1.0f);
	m_ui.Lfo1PitchKnob->setMinimum(-1.0f);
	m_ui.Lfo1PitchKnob->setMaximum(+1.0f);
	m_ui.Lfo1PanningKnob->setMinimum(-1.0f);
	m_ui.Lfo1PanningKnob->setMaximum(+1.0f);
	m_ui.Lfo1VolumeKnob->setMinimum(-1.0f);
	m_ui.Lfo1VolumeKnob->setMaximum(+1.0f);

	// Output (stereo-)width limits.
	m_ui.Out1WidthKnob->setMinimum(-1.0f);
	m_ui.Out1WidthKnob->setMaximum(+1.0f);

	// Output (stereo-)panning limits.
	m_ui.Out1PanningKnob->setMinimum(-1.0f);
	m_ui.Out1PanningKnob->setMaximum(+1.0f);

	// Effects (delay BPM)
	m_ui.Del1BpmKnob->setScale(1.0f);
	m_ui.Del1BpmKnob->setMinimum(3.6f);
	m_ui.Del1BpmKnob->setMaximum(360.0f);

	// GEN1
	setParamKnob(drumkv1::GEN1_REVERSE, m_ui.Gen1ReverseKnob);
	setParamKnob(drumkv1::GEN1_GROUP,   m_ui.Gen1GroupKnob);
	setParamKnob(drumkv1::GEN1_COARSE,  m_ui.Gen1CoarseKnob);
	setParamKnob(drumkv1::GEN1_FINE,    m_ui.Gen1FineKnob);
	setParamKnob(drumkv1::GEN1_ENVTIME, m_ui.Gen1EnvTimeKnob);

	// DCF1
	setParamKnob(drumkv1::DCF1_CUTOFF,   m_ui.Dcf1CutoffKnob);
	setParamKnob(drumkv1::DCF1_RESO,     m_ui.Dcf1ResoKnob);
	setParamKnob(drumkv1::DCF1_TYPE,     m_ui.Dcf1TypeKnob);
	setParamKnob(drumkv1::DCF1_SLOPE,    m_ui.Dcf1SlopeKnob);
	setParamKnob(drumkv1::DCF1_ENVELOPE, m_ui.Dcf1EnvelopeKnob);
	setParamKnob(drumkv1::DCF1_ATTACK,   m_ui.Dcf1AttackKnob);
	setParamKnob(drumkv1::DCF1_DECAY1,   m_ui.Dcf1Decay1Knob);
	setParamKnob(drumkv1::DCF1_LEVEL2,   m_ui.Dcf1Level2Knob);
	setParamKnob(drumkv1::DCF1_DECAY2,   m_ui.Dcf1Decay2Knob);

	QObject::connect(
		m_ui.Dcf1Filt, SIGNAL(cutoffChanged(float)),
		m_ui.Dcf1CutoffKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dcf1CutoffKnob, SIGNAL(valueChanged(float)),
		m_ui.Dcf1Filt, SLOT(setCutoff(float)));

	QObject::connect(
		m_ui.Dcf1Filt, SIGNAL(resoChanged(float)),
		m_ui.Dcf1ResoKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dcf1ResoKnob, SIGNAL(valueChanged(float)),
		m_ui.Dcf1Filt, SLOT(setReso(float)));

	QObject::connect(
		m_ui.Dcf1TypeKnob, SIGNAL(valueChanged(float)),
		m_ui.Dcf1Filt, SLOT(setType(float)));
	QObject::connect(
		m_ui.Dcf1SlopeKnob, SIGNAL(valueChanged(float)),
		m_ui.Dcf1Filt, SLOT(setSlope(float)));

	QObject::connect(
		m_ui.Dcf1Env, SIGNAL(attackChanged(float)),
		m_ui.Dcf1AttackKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dcf1AttackKnob, SIGNAL(valueChanged(float)),
		m_ui.Dcf1Env, SLOT(setAttack(float)));

	QObject::connect(
		m_ui.Dcf1Env, SIGNAL(decay1Changed(float)),
		m_ui.Dcf1Decay1Knob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dcf1Decay1Knob, SIGNAL(valueChanged(float)),
		m_ui.Dcf1Env, SLOT(setDecay1(float)));

	QObject::connect(
		m_ui.Dcf1Env, SIGNAL(level2Changed(float)),
		m_ui.Dcf1Level2Knob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dcf1Level2Knob, SIGNAL(valueChanged(float)),
		m_ui.Dcf1Env, SLOT(setLevel2(float)));

	QObject::connect(
		m_ui.Dcf1Env, SIGNAL(decay2Changed(float)),
		m_ui.Dcf1Decay2Knob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dcf1Decay2Knob, SIGNAL(valueChanged(float)),
		m_ui.Dcf1Env, SLOT(setDecay2(float)));

	// LFO1
	setParamKnob(drumkv1::LFO1_SHAPE,   m_ui.Lfo1ShapeKnob);
	setParamKnob(drumkv1::LFO1_WIDTH,   m_ui.Lfo1WidthKnob);
	setParamKnob(drumkv1::LFO1_RATE,    m_ui.Lfo1RateKnob);
	setParamKnob(drumkv1::LFO1_PANNING, m_ui.Lfo1PanningKnob);
	setParamKnob(drumkv1::LFO1_VOLUME,  m_ui.Lfo1VolumeKnob);
	setParamKnob(drumkv1::LFO1_CUTOFF,  m_ui.Lfo1CutoffKnob);
	setParamKnob(drumkv1::LFO1_RESO,    m_ui.Lfo1ResoKnob);
	setParamKnob(drumkv1::LFO1_PITCH,   m_ui.Lfo1PitchKnob);
	setParamKnob(drumkv1::LFO1_SWEEP,   m_ui.Lfo1SweepKnob);
	setParamKnob(drumkv1::LFO1_ATTACK,  m_ui.Lfo1AttackKnob);
	setParamKnob(drumkv1::LFO1_DECAY1,  m_ui.Lfo1Decay1Knob);
	setParamKnob(drumkv1::LFO1_LEVEL2,  m_ui.Lfo1Level2Knob);
	setParamKnob(drumkv1::LFO1_DECAY2,  m_ui.Lfo1Decay2Knob);

	QObject::connect(
		m_ui.Lfo1ShapeKnob, SIGNAL(valueChanged(float)),
		m_ui.Lfo1Wave, SLOT(setWaveShape(float)));
	QObject::connect(
		m_ui.Lfo1Wave, SIGNAL(waveShapeChanged(float)),
		m_ui.Lfo1ShapeKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Lfo1WidthKnob, SIGNAL(valueChanged(float)),
		m_ui.Lfo1Wave, SLOT(setWaveWidth(float)));
	QObject::connect(
		m_ui.Lfo1Wave, SIGNAL(waveWidthChanged(float)),
		m_ui.Lfo1WidthKnob, SLOT(setValue(float)));

	QObject::connect(
		m_ui.Lfo1Env, SIGNAL(attackChanged(float)),
		m_ui.Lfo1AttackKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Lfo1AttackKnob, SIGNAL(valueChanged(float)),
		m_ui.Lfo1Env, SLOT(setAttack(float)));

	QObject::connect(
		m_ui.Lfo1Env, SIGNAL(decay1Changed(float)),
		m_ui.Lfo1Decay1Knob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Lfo1Decay1Knob, SIGNAL(valueChanged(float)),
		m_ui.Lfo1Env, SLOT(setDecay1(float)));

	QObject::connect(
		m_ui.Lfo1Env, SIGNAL(level2Changed(float)),
		m_ui.Lfo1Level2Knob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Lfo1Level2Knob, SIGNAL(valueChanged(float)),
		m_ui.Lfo1Env, SLOT(setLevel2(float)));

	QObject::connect(
		m_ui.Lfo1Env, SIGNAL(decay2Changed(float)),
		m_ui.Lfo1Decay2Knob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Lfo1Decay2Knob, SIGNAL(valueChanged(float)),
		m_ui.Lfo1Env, SLOT(setDecay2(float)));

	// DCA1
	setParamKnob(drumkv1::DCA1_VOLUME, m_ui.Dca1VolumeKnob);
	setParamKnob(drumkv1::DCA1_ATTACK, m_ui.Dca1AttackKnob);
	setParamKnob(drumkv1::DCA1_DECAY1, m_ui.Dca1Decay1Knob);
	setParamKnob(drumkv1::DCA1_LEVEL2, m_ui.Dca1Level2Knob);
	setParamKnob(drumkv1::DCA1_DECAY2, m_ui.Dca1Decay2Knob);

	QObject::connect(
		m_ui.Dca1Env, SIGNAL(attackChanged(float)),
		m_ui.Dca1AttackKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dca1AttackKnob, SIGNAL(valueChanged(float)),
		m_ui.Dca1Env, SLOT(setAttack(float)));

	QObject::connect(
		m_ui.Dca1Env, SIGNAL(decay1Changed(float)),
		m_ui.Dca1Decay1Knob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dca1Decay1Knob, SIGNAL(valueChanged(float)),
		m_ui.Dca1Env, SLOT(setDecay1(float)));

	QObject::connect(
		m_ui.Dca1Env, SIGNAL(level2Changed(float)),
		m_ui.Dca1Level2Knob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dca1Level2Knob, SIGNAL(valueChanged(float)),
		m_ui.Dca1Env, SLOT(setLevel2(float)));

	QObject::connect(
		m_ui.Dca1Env, SIGNAL(decay2Changed(float)),
		m_ui.Dca1Decay2Knob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dca1Decay2Knob, SIGNAL(valueChanged(float)),
		m_ui.Dca1Env, SLOT(setDecay2(float)));

	// DEF1
	setParamKnob(drumkv1::DEF1_PITCHBEND, m_ui.Def1PitchbendKnob);
	setParamKnob(drumkv1::DEF1_MODWHEEL,  m_ui.Def1ModwheelKnob);
	setParamKnob(drumkv1::DEF1_PRESSURE,  m_ui.Def1PressureKnob);
	setParamKnob(drumkv1::DEF1_VELOCITY,  m_ui.Def1VelocityKnob);
	setParamKnob(drumkv1::DEF1_CHANNEL,   m_ui.Def1ChannelKnob);
	setParamKnob(drumkv1::DEF1_NOTEOFF,   m_ui.Def1NoteoffKnob);

	// OUT1
	setParamKnob(drumkv1::OUT1_WIDTH,   m_ui.Out1WidthKnob);
	setParamKnob(drumkv1::OUT1_PANNING, m_ui.Out1PanningKnob);
	setParamKnob(drumkv1::OUT1_VOLUME,  m_ui.Out1VolumeKnob);

	// Reverb (stereo-)width limits.
	m_ui.Rev1WidthKnob->setMinimum(-1.0f);
	m_ui.Rev1WidthKnob->setMaximum(+1.0f);

	// Effects
	setParamKnob(drumkv1::CHO1_WET,   m_ui.Cho1WetKnob);
	setParamKnob(drumkv1::CHO1_DELAY, m_ui.Cho1DelayKnob);
	setParamKnob(drumkv1::CHO1_FEEDB, m_ui.Cho1FeedbKnob);
	setParamKnob(drumkv1::CHO1_RATE,  m_ui.Cho1RateKnob);
	setParamKnob(drumkv1::CHO1_MOD,   m_ui.Cho1ModKnob);

	setParamKnob(drumkv1::FLA1_WET,   m_ui.Fla1WetKnob);
	setParamKnob(drumkv1::FLA1_DELAY, m_ui.Fla1DelayKnob);
	setParamKnob(drumkv1::FLA1_FEEDB, m_ui.Fla1FeedbKnob);
	setParamKnob(drumkv1::FLA1_DAFT,  m_ui.Fla1DaftKnob);

	setParamKnob(drumkv1::PHA1_WET,   m_ui.Pha1WetKnob);
	setParamKnob(drumkv1::PHA1_RATE,  m_ui.Pha1RateKnob);
	setParamKnob(drumkv1::PHA1_FEEDB, m_ui.Pha1FeedbKnob);
	setParamKnob(drumkv1::PHA1_DEPTH, m_ui.Pha1DepthKnob);
	setParamKnob(drumkv1::PHA1_DAFT,  m_ui.Pha1DaftKnob);

	setParamKnob(drumkv1::DEL1_WET,   m_ui.Del1WetKnob);
	setParamKnob(drumkv1::DEL1_DELAY, m_ui.Del1DelayKnob);
	setParamKnob(drumkv1::DEL1_FEEDB, m_ui.Del1FeedbKnob);
	setParamKnob(drumkv1::DEL1_BPM,   m_ui.Del1BpmKnob);

	QObject::connect(m_ui.Del1BpmKnob,
		SIGNAL(valueChanged(float)),
		SLOT(bpmSyncChanged()));

	// Reverb
	setParamKnob(drumkv1::REV1_WET,   m_ui.Rev1WetKnob);
	setParamKnob(drumkv1::REV1_ROOM,  m_ui.Rev1RoomKnob);
	setParamKnob(drumkv1::REV1_DAMP,  m_ui.Rev1DampKnob);
	setParamKnob(drumkv1::REV1_FEEDB, m_ui.Rev1FeedbKnob);
	setParamKnob(drumkv1::REV1_WIDTH, m_ui.Rev1WidthKnob);

		// Dynamics
	setParamKnob(drumkv1::DYN1_COMPRESS, m_ui.Dyn1CompressKnob);
	setParamKnob(drumkv1::DYN1_LIMITER,  m_ui.Dyn1LimiterKnob);


	// Element selectors...
	QObject::connect(m_ui.Elements,
		SIGNAL(itemActivated(int)),
		SLOT(activateElement()));
	QObject::connect(m_ui.Elements,
		SIGNAL(itemDoubleClicked(int)),
		SLOT(doubleClickElement()));

	// Sample management...
	QObject::connect(m_ui.Gen1Sample,
		SIGNAL(loadSampleFile(const QString&)),
		SLOT(loadSample(const QString&)));

	// Preset management
	QObject::connect(m_ui.Preset,
		SIGNAL(newPresetFile()),
		SLOT(newPreset()));
	QObject::connect(m_ui.Preset,
		SIGNAL(loadPresetFile(const QString&)),
		SLOT(loadPreset(const QString&)));
	QObject::connect(m_ui.Preset,
		SIGNAL(savePresetFile(const QString&)),
		SLOT(savePreset(const QString&)));
	QObject::connect(m_ui.Preset,
		SIGNAL(resetPresetFile()),
		SLOT(resetParams()));

	// Common context menu...
	m_ui.Elements->setContextMenuPolicy(Qt::CustomContextMenu);
	m_ui.Gen1Sample->setContextMenuPolicy(Qt::CustomContextMenu);

	QObject::connect(m_ui.Elements,
		SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(contextMenuRequest(const QPoint&)));
	QObject::connect(m_ui.Gen1Sample,
		SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(contextMenuRequest(const QPoint&)));


	// Swap params A/B
	QObject::connect(m_ui.SwapParamsAButton,
		SIGNAL(toggled(bool)),
		SLOT(swapParams(bool)));
	QObject::connect(m_ui.SwapParamsBButton,
		SIGNAL(toggled(bool)),
		SLOT(swapParams(bool)));

	// Direct stacked-page signal/slot
	QObject::connect(m_ui.TabBar, SIGNAL(currentChanged(int)),
		m_ui.StackedWidget, SLOT(setCurrentIndex(int)));

	drumkv1widget_config *pConfig = drumkv1widget_config::getInstance();
	if (pConfig)
		m_ui.helpUseNativeDialogsAction->setChecked(pConfig->bUseNativeDialogs);

	// Menu actions
	QObject::connect(m_ui.helpUseNativeDialogsAction,
		SIGNAL(triggered(bool)),
		SLOT(helpUseNativeDialogs(bool)));
	QObject::connect(m_ui.helpAboutAction,
		SIGNAL(triggered(bool)),
		SLOT(helpAbout()));
	QObject::connect(m_ui.helpAboutQtAction,
		SIGNAL(triggered(bool)),
		SLOT(helpAboutQt()));

	// Epilog.
	// QWidget::adjustSize();

	m_ui.StatusBar->showMessage(tr("Ready"), 5000);
	m_ui.StatusBar->setModified(false);
	m_ui.Preset->setDirtyPreset(false);
}


// Param kbob (widget) map accesors.
void drumkv1widget::setParamKnob ( drumkv1::ParamIndex index, drumkv1widget_knob *pKnob )
{
	pKnob->setDefaultValue(drumkv1_param::paramDefaultValue(index));

	m_paramKnobs.insert(index, pKnob);
	m_knobParams.insert(pKnob, index);

	QObject::connect(pKnob,
		SIGNAL(valueChanged(float)),
		SLOT(paramChanged(float)));
}

drumkv1widget_knob *drumkv1widget::paramKnob ( drumkv1::ParamIndex index ) const
{
	return m_paramKnobs.value(index, NULL);
}


// Param port accessors.
void drumkv1widget::setParamValue (
	drumkv1::ParamIndex index, float fValue, bool bDefault )
{
	++m_iUpdate;

	drumkv1widget_knob *pKnob = paramKnob(index);
	if (pKnob)
		pKnob->setValue(fValue, bDefault);

	updateParamEx(index, fValue);

	--m_iUpdate;
}

float drumkv1widget::paramValue ( drumkv1::ParamIndex index ) const
{
	float fParamValue = 0.0f;

	drumkv1widget_knob *pKnob = paramKnob(index);
	if (pKnob) {
		fParamValue = pKnob->value();
	} else {
		drumkv1 *pDrumk = instance();
		if (pDrumk) {
			const float *pParamPort = pDrumk->paramPort(index);
			if (pParamPort)
				fParamValue = *pParamPort;
		}
	}

	return fParamValue;
}


// Param knob (widget) slot.
void drumkv1widget::paramChanged ( float fValue )
{
	if (m_iUpdate > 0)
		return;

	drumkv1widget_knob *pKnob = qobject_cast<drumkv1widget_knob *> (sender());
	if (pKnob) {
		drumkv1::ParamIndex index = m_knobParams.value(pKnob);
		updateParam(index, fValue);
		updateParamEx(index, fValue);
		m_ui.StatusBar->showMessage(QString("%1 / %2: %3")
			.arg(m_ui.StackedWidget->currentWidget()->windowTitle())
			.arg(pKnob->toolTip())
			.arg(pKnob->valueText()), 5000);
		updateDirtyPreset(true);
	}
}


// Update local tied widgets.
void drumkv1widget::updateParamEx ( drumkv1::ParamIndex index, float fValue )
{
	drumkv1 *pDrumk = instance();
	if (pDrumk == NULL)
		return;

	++m_iUpdate;

	switch (index) {
	case drumkv1::GEN1_REVERSE: {
		const bool bReverse = bool(fValue > 0.0f);
		pDrumk->setReverse(bReverse);
		updateSample(pDrumk->sample());
		break;
	}
	case drumkv1::DEL1_BPMSYNC:
		if (fValue > 0.0f)
			m_ui.Del1BpmKnob->setValue(0.0f);
		// Fall thru...
	default:
		break;
	}

	--m_iUpdate;
}


// Reset all param knobs to default values.
void drumkv1widget::resetParams (void)
{
	drumkv1 *pDrumk = instance();
	if (pDrumk == NULL)
		return;

	pDrumk->reset();

	resetSwapParams();

	for (uint32_t i = 0; i < drumkv1::NUM_PARAMS; ++i) {
		drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
		float fValue = drumkv1_param::paramDefaultValue(index);
		drumkv1widget_knob *pKnob = paramKnob(index);
		if (pKnob)
			fValue = pKnob->defaultValue();
		setParamValue(index, fValue);
		updateParam(index, fValue);
		m_params_ab[index] = fValue;
	}

	m_ui.StatusBar->showMessage(tr("Reset preset"), 5000);
	updateDirtyPreset(false);
}


// Swap params A/B.
void drumkv1widget::swapParams ( bool bOn )
{
	if (m_iUpdate > 0 || !bOn)
		return;

#ifdef CONFIG_DEBUG
	qDebug("drumkv1widget::swapParams(%d)", int(bOn));
#endif
//	resetParamKnobs(drumkv1::NUM_PARAMS);

	drumkv1 *pDrumk = instance();
	if (pDrumk) {
		// Save current element param values...
		drumkv1_element *element = pDrumk->element(pDrumk->currentElement());
		if (element) {
			for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i) {
				drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
				drumkv1widget_knob *pKnob = paramKnob(index);
				if (pKnob) {
					pKnob->setDefaultValue(element->paramValue(index, 0));
					element->setParamValue(index, pKnob->value());
				}
			}
		}
		// Swap all element params A/B...
		pDrumk->resetParamValues(true);
		// Retrieve current element param values...
		if (element) {
			for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i) {
				drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
				m_params_ab[index] = element->paramValue(index);
			}
		}
	}

	for (uint32_t i = 0; i < drumkv1::NUM_PARAMS; ++i) {
		drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
		drumkv1widget_knob *pKnob = paramKnob(index);
		if (pKnob) {
			const float fOldValue = pKnob->value();
			const float fNewValue = m_params_ab[index];
			setParamValue(index, fNewValue);
			updateParam(index, fNewValue);
			m_params_ab[index] = fOldValue;
		}
	}

	const bool bSwapA = m_ui.SwapParamsAButton->isChecked();
	m_ui.StatusBar->showMessage(tr("Swap %1").arg(bSwapA ? 'A' : 'B'), 5000);
	updateDirtyPreset(true);
}

 
// Reset swap params A/B group.
void drumkv1widget::resetSwapParams (void)
{
	++m_iUpdate;
	m_ui.SwapParamsAButton->setChecked(true);
	--m_iUpdate;
}


// Initialize param values.
void drumkv1widget::updateParamValues ( uint32_t nparams )
{
	resetSwapParams();

	drumkv1 *pDrumk = instance();

	for (uint32_t i = 0; i < nparams; ++i) {
		drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
		float fValue = drumkv1_param::paramDefaultValue(index);
		const float *pfParamPort
			= (pDrumk ? pDrumk->paramPort(index) : NULL);
		if (pfParamPort)
			fValue = *pfParamPort;
		setParamValue(index, fValue, true);
		updateParam(index, fValue);
		updateParamEx(index, fValue);
		m_params_ab[index] = fValue;
	}
}


// Reset all param default values.
void drumkv1widget::resetParamValues ( uint32_t nparams )
{
	resetSwapParams();

	for (uint32_t i = 0; i < nparams; ++i) {
		drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
		float fValue = drumkv1_param::paramDefaultValue(index);
		setParamValue(index, fValue, true);
		updateParam(index, fValue);
		m_params_ab[index] = fValue;
	}
}


// Reset all knob default values.
void drumkv1widget::resetParamKnobs ( uint32_t nparams )
{
	for (uint32_t i = 0; i < nparams; ++i) {
		drumkv1widget_knob *pKnob = paramKnob(drumkv1::ParamIndex(i));
		if (pKnob)
			pKnob->resetDefaultValue();
	}
}


// Preset init.
void drumkv1widget::initPreset (void)
{
	m_ui.Preset->initPreset();
}


// Preset clear.
void drumkv1widget::clearPreset (void)
{
	m_ui.Preset->clearPreset();
}


// Preset renewal.
void drumkv1widget::newPreset (void)
{
#ifdef CONFIG_DEBUG
	qDebug("drumkv1widget::newPreset()");
#endif

	clearElements();

	clearSampleFile();

	resetParamKnobs(drumkv1::NUM_PARAMS);
	resetParamValues(drumkv1::NUM_PARAMS);

	drumkv1 *pDrumk = instance();
	if (pDrumk)
		pDrumk->reset();

	refreshElements();
	activateElement();

	m_ui.StatusBar->showMessage(tr("New preset"), 5000);
	updateDirtyPreset(false);
}


// Preset file I/O slots.
void drumkv1widget::loadPreset ( const QString& sFilename )
{
#ifdef CONFIG_DEBUG
	qDebug("drumkv1widget::loadPreset(\"%s\")", sFilename.toUtf8().constData());
#endif

	clearElements();

	clearSampleFile();

	resetParamKnobs(drumkv1::NUM_PARAMS);
	resetParamValues(drumkv1::NUM_PARAMS);

	drumkv1 *pDrumk = instance();
	if (pDrumk == NULL)
		return;

	drumkv1_param::loadPreset(pDrumk, sFilename);

	updateParamValues(drumkv1::NUM_PARAMS);

	const QString& sPreset
		= QFileInfo(sFilename).completeBaseName();

	m_ui.Preset->setPreset(sPreset);
	m_ui.StatusBar->showMessage(tr("Load preset: %1").arg(sPreset), 5000);
	updateDirtyPreset(false);

	refreshElements();
	activateElement();
}


void drumkv1widget::savePreset ( const QString& sFilename )
{
#ifdef CONFIG_DEBUG
	qDebug("drumkv1widget::savePreset(\"%s\")", sFilename.toUtf8().constData());
#endif

	drumkv1_param::savePreset(instance(), sFilename);

	const QString& sPreset
		= QFileInfo(sFilename).completeBaseName();

	m_ui.StatusBar->showMessage(tr("Save preset: %1").arg(sPreset), 5000);
	updateDirtyPreset(false);
}


// Sample reset slot.
void drumkv1widget::clearSample (void)
{
	clearSampleFile();

	m_ui.StatusBar->showMessage(tr("Clear sample"), 5000);
	updateDirtyPreset(true);
}


// Sample file loader slot.
void drumkv1widget::loadSample ( const QString& sFilename )
{
	loadSampleFile(sFilename);

	m_ui.StatusBar->showMessage(tr("Load sample: %1").arg(sFilename), 5000);
	updateDirtyPreset(true);
}


// Sample openner.
void drumkv1widget::openSample (void)
{
	m_ui.Gen1Sample->openSample(currentNoteName());
}


// Sample file reset.
void drumkv1widget::clearSampleFile (void)
{
#ifdef CONFIG_DEBUG
	qDebug("drumkv1widget::clearSampleFile()");
#endif

	drumkv1 *pDrumk = instance();
	if (pDrumk)
		pDrumk->setSampleFile(NULL);

	updateSample(NULL);
}


// Sample file loader.
void drumkv1widget::loadSampleFile ( const QString& sFilename )
{
#ifdef CONFIG_DEBUG
	qDebug("drumkv1widget::loadSampleFile(\"%s\")", sFilename.toUtf8().constData());
#endif

	drumkv1 *pDrumk = instance();
	if (pDrumk == NULL)
		return;

	const int note = currentNote();
	if (note < 0)
		return;

	drumkv1_element *element = pDrumk->element(note);
	if (element == NULL) {
		element = pDrumk->addElement(note);
		for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i) {
			drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
			float fValue = drumkv1_param::paramDefaultValue(index);
			element->setParamValue(index, fValue);
		}
		pDrumk->setCurrentElement(note);
		for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i) {
			drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
			setParamValue(index, element->paramValue(index));
		}
		activateParamKnobs(true);
	}

	pDrumk->setSampleFile(sFilename.toUtf8().constData());
	updateSample(pDrumk->sample(), true);

	refreshElements();
}


// Sample updater (crude experimental stuff II).
void drumkv1widget::updateSample ( drumkv1_sample *pSample, bool bDirty )
{
	m_ui.Gen1Sample->setSampleName(currentNoteName());
	m_ui.Gen1Sample->setSample(pSample);

	if (pSample && bDirty)
		updateDirtyPreset(true);
}


// Current selected note helpers.
int drumkv1widget::currentNote (void) const
{
	return m_ui.Elements->currentIndex();
}


QString drumkv1widget::currentNoteName (void) const
{
	const int note = currentNote();
	return (note < 0 ? tr("(None)") : completeNoteName(note));
}


// MIDI note/octave name helpers (current).
QString drumkv1widget::noteName ( int note )
{
	static struct
	{
		int note;
		const char *name;

	} s_notes[] = {

		// Diatonic note map...
		{  0, QT_TR_NOOP("C")     },
		{  1, QT_TR_NOOP("C#/Db") },
		{  2, QT_TR_NOOP("D")     },
		{  3, QT_TR_NOOP("D#/Eb") },
		{  4, QT_TR_NOOP("E")     },
		{  5, QT_TR_NOOP("F")     },
		{  6, QT_TR_NOOP("F#/Gb") },
		{  7, QT_TR_NOOP("G")     },
		{  8, QT_TR_NOOP("G#/Ab") },
		{  9, QT_TR_NOOP("A")     },
		{ 10, QT_TR_NOOP("A#/Bb") },
		{ 11, QT_TR_NOOP("B")     },

		// GM Drum note map...
		{ 35, QT_TR_NOOP("Acoustic Bass Drum") },
		{ 36, QT_TR_NOOP("Bass Drum 1") },
		{ 37, QT_TR_NOOP("Side Stick") },
		{ 38, QT_TR_NOOP("Acoustic Snare") },
		{ 39, QT_TR_NOOP("Hand Clap") },
		{ 40, QT_TR_NOOP("Electric Snare") },
		{ 41, QT_TR_NOOP("Low Floor Tom") },
		{ 42, QT_TR_NOOP("Closed Hi-Hat") },
		{ 43, QT_TR_NOOP("High Floor Tom") },
		{ 44, QT_TR_NOOP("Pedal Hi-Hat") },
		{ 45, QT_TR_NOOP("Low Tom") },
		{ 46, QT_TR_NOOP("Open Hi-Hat") },
		{ 47, QT_TR_NOOP("Low-Mid Tom") },
		{ 48, QT_TR_NOOP("Hi-Mid Tom") },
		{ 49, QT_TR_NOOP("Crash Cymbal 1") },
		{ 50, QT_TR_NOOP("High Tom") },
		{ 51, QT_TR_NOOP("Ride Cymbal 1") },
		{ 52, QT_TR_NOOP("Chinese Cymbal") },
		{ 53, QT_TR_NOOP("Ride Bell") },
		{ 54, QT_TR_NOOP("Tambourine") },
		{ 55, QT_TR_NOOP("Splash Cymbal") },
		{ 56, QT_TR_NOOP("Cowbell") },
		{ 57, QT_TR_NOOP("Crash Cymbal 2") },
		{ 58, QT_TR_NOOP("Vibraslap") },
		{ 59, QT_TR_NOOP("Ride Cymbal 2") },
		{ 60, QT_TR_NOOP("Hi Bongo") },
		{ 61, QT_TR_NOOP("Low Bongo") },
		{ 62, QT_TR_NOOP("Mute Hi Conga") },
		{ 63, QT_TR_NOOP("Open Hi Conga") },
		{ 64, QT_TR_NOOP("Low Conga") },
		{ 65, QT_TR_NOOP("High Timbale") },
		{ 66, QT_TR_NOOP("Low Timbale") },
		{ 67, QT_TR_NOOP("High Agogo") },
		{ 68, QT_TR_NOOP("Low Agogo") },
		{ 69, QT_TR_NOOP("Cabasa") },
		{ 70, QT_TR_NOOP("Maracas") },
		{ 71, QT_TR_NOOP("Short Whistle") },
		{ 72, QT_TR_NOOP("Long Whistle") },
		{ 73, QT_TR_NOOP("Short Guiro") },
		{ 74, QT_TR_NOOP("Long Guiro") },
		{ 75, QT_TR_NOOP("Claves") },
		{ 76, QT_TR_NOOP("Hi Wood Block") },
		{ 77, QT_TR_NOOP("Low Wood Block") },
		{ 78, QT_TR_NOOP("Mute Cuica") },
		{ 79, QT_TR_NOOP("Open Cuica") },
		{ 80, QT_TR_NOOP("Mute Triangle") },
		{ 81, QT_TR_NOOP("Open Triangle") },

		{  0, NULL }
	};

	static QHash<int, QString> s_names;

	// Pre-load drum-names hash table...
	if (s_names.isEmpty()) {
		for (int i = 12; s_notes[i].name; ++i) {
			s_names.insert(s_notes[i].note,
				QObject::tr(s_notes[i].name, "noteName"));
		}
	}
	// Check whether the drum note exists...
	QHash<int, QString>::ConstIterator iter = s_names.constFind(note);
	if (iter != s_names.constEnd())
		return iter.value();

	return QString("%1 %2").arg(s_notes[note % 12].name).arg((note / 12) - 1);
}

QString drumkv1widget::completeNoteName ( int note )
{
	return QString("%1 - %2").arg(note).arg(noteName(note));
}


// Dirty close prompt,
bool drumkv1widget::queryClose (void)
{
	return m_ui.Preset->queryPreset();
}


// Reload all elements.
void drumkv1widget::refreshElements (void)
{
	bool bBlockSignals = m_ui.Elements->blockSignals(true);

	if (m_ui.Elements->instance() == NULL)
		m_ui.Elements->setInstance(instance());

	int iCurrentNote = currentNote();

#ifdef CONFIG_DEBUG
	qDebug("drumkv1widget::refreshElements(%d)", iCurrentNote);
#endif

	m_ui.Elements->refresh();

	if (iCurrentNote < 0) iCurrentNote = 36; // Bass Drum 1 (default)
	m_ui.Elements->setCurrentIndex(iCurrentNote);
	m_ui.Gen1Sample->setSampleName(completeNoteName(iCurrentNote));

	m_ui.Elements->blockSignals(bBlockSignals);
}


// All element clear.
void drumkv1widget::clearElements (void)
{
	drumkv1 *pDrumk = instance();
	if (pDrumk)
		pDrumk->clearElements();
}


// Element activation.
void drumkv1widget::activateElement ( bool bOpenSample )
{
	const int iCurrentNote = currentNote();
	if (iCurrentNote < 0)
		return;

#ifdef CONFIG_DEBUG
	qDebug("drumkv1widget::activateElement(%d)", iCurrentNote);
#endif

	drumkv1 *pDrumk = instance();
	if (pDrumk == NULL)
		return;

	drumkv1_element *element = pDrumk->element(iCurrentNote);
	if (element == NULL && bOpenSample) {
		element = pDrumk->addElement(iCurrentNote);
		for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i) {
			drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
			float fValue = drumkv1_param::paramDefaultValue(index);
			element->setParamValue(index, fValue, 0);
			element->setParamValue(index, fValue);
		}
	}

	pDrumk->setCurrentElement(iCurrentNote);

	resetParamKnobs(drumkv1::NUM_ELEMENT_PARAMS);

	if (element) {
		for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i) {
			drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
			drumkv1widget_knob *pKnob = paramKnob(index);
			if (pKnob)
				pKnob->setDefaultValue(element->paramValue(index, 0));
			setParamValue(index, element->paramValue(index));
		}
		updateSample(pDrumk->sample());
		refreshElements();
	} else {
		updateSample(NULL);
		resetParamValues(drumkv1::NUM_ELEMENT_PARAMS);
	}

	activateParamKnobs(element != NULL);

	const QString& sElementName = completeNoteName(iCurrentNote);
	m_ui.StatusBar->showMessage(tr("Element: %1").arg(sElementName), 5000);

	if (bOpenSample)
		m_ui.Gen1Sample->openSample(sElementName);
}


// Element sample loader.
void drumkv1widget::doubleClickElement (void)
{
	activateElement(true);
}


// Element deactivation.
void drumkv1widget::resetElement (void)
{
	clearSampleFile();

	drumkv1 *pDrumk = instance();
	if (pDrumk) {
		pDrumk->removeElement(pDrumk->currentElement());
		updateDirtyPreset(true);
	}

	refreshElements();
	activateElement();
}


// (En|Dis)able/ all param/knobs.
void drumkv1widget::activateParamKnobs ( bool bEnabled )
{
	activateParamKnobsGroupBox(m_ui.Gen1GroupBox, bEnabled);
	activateParamKnobsGroupBox(m_ui.Dcf1GroupBox, bEnabled);
	activateParamKnobsGroupBox(m_ui.Lfo1GroupBox, bEnabled);
	activateParamKnobsGroupBox(m_ui.Dca1GroupBox, bEnabled);
	activateParamKnobsGroupBox(m_ui.Out1GroupBox, bEnabled);

	m_ui.Gen1Sample->setEnabled(true);
}


void drumkv1widget::activateParamKnobsGroupBox ( QGroupBox *pGroupBox, bool bEnabled )
{
	const QList<QWidget *>& children
		= pGroupBox->findChildren<QWidget *> ();
	QListIterator<QWidget *> iter(children);
	while (iter.hasNext())
		iter.next()->setEnabled(bEnabled);
}


// Delay BPM change.
void drumkv1widget::bpmSyncChanged (void)
{
	if (m_iUpdate > 0)
		return;

	++m_iUpdate;
	drumkv1 *pDrumk = instance();
	if (pDrumk) {
		float *pBpmSync = pDrumk->paramPort(drumkv1::DEL1_BPMSYNC);
		if (pBpmSync) {
			const bool bBpmSync0
				= (*pBpmSync > 0.0f);
			const bool bBpmSync1
				= (m_ui.Del1BpmKnob->minimum() >= m_ui.Del1BpmKnob->value());
			if ((bBpmSync1 && !bBpmSync0) || (!bBpmSync1 && bBpmSync0))
				*pBpmSync = (bBpmSync1 ? 1.0f : 0.0f);
		}
	}
	--m_iUpdate;
}


// Common context menu.
void drumkv1widget::contextMenuRequest ( const QPoint& pos )
{
	QWidget *pSender = static_cast<QWidget *> (sender());
	if (pSender == NULL)
		return;

	QMenu menu(this);
	QAction *pAction;

	drumkv1 *pDrumk = instance();
	drumkv1_element *element = NULL;
	if (pDrumk)
		element = pDrumk->element(pDrumk->currentElement());

	pAction = menu.addAction(
		QIcon(":/images/fileOpen.png"),
		tr("Open Sample..."), this, SLOT(openSample()));
	pAction->setEnabled(pDrumk != NULL);
	menu.addSeparator();
	pAction = menu.addAction(
		tr("Reset"), this, SLOT(resetElement()));
	pAction->setEnabled(element != NULL);

	QAbstractScrollArea *pAbstractScrollArea
		= qobject_cast<QAbstractScrollArea *> (pSender);
	if (pAbstractScrollArea)
		pSender = pAbstractScrollArea->viewport();

	menu.exec(pSender->mapToGlobal(pos));
}


// Menu actions.
void drumkv1widget::helpUseNativeDialogs ( bool bOn )
{
	drumkv1widget_config *pConfig = drumkv1widget_config::getInstance();
	if (pConfig) {
		pConfig->bUseNativeDialogs = bOn;
		pConfig->bDontUseNativeDialogs = !pConfig->bUseNativeDialogs;
	}
}


void drumkv1widget::helpAbout (void)
{
	// About...
	QStringList list;
#ifdef CONFIG_DEBUG
	list << tr("Debugging option enabled.");
#endif
#ifndef CONFIG_JACK
	list << tr("JACK stand-alone build disabled.");
#endif
#ifndef CONFIG_JACK_SESSION
	list << tr("JACK session support disabled.");
#endif
#ifndef CONFIG_JACK_MIDI
	list << tr("JACK MIDI support disabled.");
#endif
#ifndef CONFIG_ALSA_MIDI
	list << tr("ALSA MIDI support disabled.");
#endif
#ifndef CONFIG_LV2
	list << tr("LV2 plug-in build disabled.");
#endif

	QString sText = "<p>\n";
	sText += "<b>" DRUMKV1_TITLE "</b> - " + tr(DRUMKV1_SUBTITLE) + "<br />\n";
	sText += "<br />\n";
	sText += tr("Version") + ": <b>" DRUMKV1_VERSION "</b><br />\n";
	sText += "<small>" + tr("Build") + ": " __DATE__ " " __TIME__ "</small><br />\n";
	QStringListIterator iter(list);
	while (iter.hasNext()) {
		sText += "<small><font color=\"red\">";
		sText += iter.next();
		sText += "</font></small><br />";
	}
	sText += "<br />\n";
	sText += tr("Website") + ": <a href=\"" DRUMKV1_WEBSITE "\">" DRUMKV1_WEBSITE "</a><br />\n";
	sText += "<br />\n";
	sText += "<small>";
	sText += DRUMKV1_COPYRIGHT "<br />\n";
	sText += "<br />\n";
	sText += tr("This program is free software; you can redistribute it and/or modify it") + "<br />\n";
	sText += tr("under the terms of the GNU General Public License version 2 or later.");
	sText += "</small>";
	sText += "</p>\n";

	QMessageBox::about(this, tr("About") + " " DRUMKV1_TITLE, sText);
}


void drumkv1widget::helpAboutQt (void)
{
	// About Qt...
	QMessageBox::aboutQt(this);
}


// Dirty flag (overridable virtual) methods.
void drumkv1widget::updateDirtyPreset ( bool bDirtyPreset )
{
	m_ui.StatusBar->setModified(bDirtyPreset);
	m_ui.Preset->setDirtyPreset(bDirtyPreset);
}


// end of drumkv1widget.cpp
