// drumkv1widget.cpp
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

#include "drumkv1widget.h"

#include <QDomDocument>
#include <QTextStream>
#include <QFileInfo>

#include <QMessageBox>


//-------------------------------------------------------------------------
// default state (params)

static
struct {

	const char *name;
	float value;

} drumkv1_default_params[drumkv1::NUM_PARAMS] = {

	{ "GEN1_SAMPLE",   60.0f }, // middle-C aka. C4 (60)
	{ "GEN1_LOOP",      0.0f },
	{ "GEN1_OCTAVE",    0.0f },
	{ "GEN1_TUNING",    0.0f },
	{ "DCF1_CUTOFF",    1.0f }, // 0.5f
	{ "DCF1_RESO",      0.0f },
	{ "DCF1_TYPE",      0.0f },
	{ "DCF1_SLOPE",     0.0f },
	{ "DCF1_ENVELOPE",  1.0f },
	{ "DCF1_ATTACK",    0.0f },
	{ "DCF1_DECAY1",    0.2f },
	{ "DCF1_DECAY2",    0.5f },
	{ "LFO1_SHAPE",     1.0f },
	{ "LFO1_WIDTH",     1.0f },
	{ "LFO1_RATE",      0.5f },
	{ "LFO1_SWEEP",     0.0f },
	{ "LFO1_PITCH",     0.0f },
	{ "LFO1_CUTOFF",    0.0f },
	{ "LFO1_RESO",      0.0f },
	{ "LFO1_PANNING",   0.0f },
	{ "LFO1_VOLUME",    0.0f },
	{ "LFO1_ATTACK",    0.0f },
	{ "LFO1_DECAY1",    0.2f },
	{ "LFO1_DECAY2",    0.5f },
	{ "DCA1_VOLUME",    0.5f },
	{ "DCA1_ATTACK",    0.0f },
	{ "DCA1_DECAY1",    0.2f },
	{ "DCA1_DECAY2",    0.5f },	// 0.1f
	{ "DEF1_PITCHBEND", 0.2f },
	{ "DEF1_MODWHEEL",  0.2f },
	{ "DEF1_PRESSURE",  0.2f },
	{ "OUT1_WIDTH",     0.0f },
	{ "OUT1_PANNING",   0.0f },
	{ "OUT1_VOLUME",    0.5f },

	{ "CHO1_WET",       0.0f },
	{ "CHO1_DELAY",     0.5f },
	{ "CHO1_FEEDB",     0.5f },
	{ "CHO1_RATE",      0.5f },
	{ "CHO1_MOD",       0.5f },
	{ "FLA1_WET",       0.0f },
	{ "FLA1_DELAY",     0.5f },
	{ "FLA1_FEEDB",     0.5f },
	{ "FLA1_DAFT",      0.0f },
	{ "PHA1_WET",       0.0f },
	{ "PHA1_RATE",      0.5f },
	{ "PHA1_FEEDB",     0.5f },
	{ "PHA1_DEPTH",     0.5f },
	{ "PHA1_DAFT",      0.0f },
	{ "DEL1_WET",       0.0f },
	{ "DEL1_DELAY",     0.5f },
	{ "DEL1_FEEDB",     0.5f },
	{ "DEL1_BPM",       1.8f },
	{ "DYN1_COMPRESS",  0.0f },
	{ "DYN1_LIMIT",     1.0f }
};


//-------------------------------------------------------------------------
// drumkv1widget - impl.
//

// Constructor.
drumkv1widget::drumkv1widget ( QWidget *pParent, Qt::WindowFlags wflags )
	: QWidget(pParent, wflags)
{
	Q_INIT_RESOURCE(drumkv1);

	m_ui.setupUi(this);

	// Start clean.
	m_iUpdate = 0;

	// Note names.
	QStringList notes;
	for (int note = 0; note < 128; ++note)
		notes << noteName(note);

	m_ui.Gen1SampleKnob->insertItems(0, notes);

	// Wave shapes.
	QStringList shapes;
	shapes << tr("Pulse");
	shapes << tr("Saw");
	shapes << tr("Sine");
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

	// Dynamic states.
	QStringList states;
	states << tr("Off");
	states << tr("On");

	m_ui.Gen1LoopKnob->insertItems(0, states);

	m_ui.Dyn1CompressKnob->insertItems(0, states);
	m_ui.Dyn1LimiterKnob->insertItems(0, states);

	// Special values
	const QString& sOff = states.first();
	m_ui.Cho1WetKnob->setSpecialValueText(sOff);
	m_ui.Fla1WetKnob->setSpecialValueText(sOff);
	m_ui.Pha1WetKnob->setSpecialValueText(sOff);
	m_ui.Del1WetKnob->setSpecialValueText(sOff);

	// GEN note limits.
	m_ui.Gen1SampleKnob->setMinimum(0.0f);
	m_ui.Gen1SampleKnob->setMaximum(127.0f);

	// GEN octave limits.
	m_ui.Gen1OctaveKnob->setMinimum(-4.0f);
	m_ui.Gen1OctaveKnob->setMaximum(+4.0f);

	// GEN tune limits.
	m_ui.Gen1TuningKnob->setMinimum(-1.0f);
	m_ui.Gen1TuningKnob->setMaximum(+1.0f);

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
	m_ui.Del1BpmKnob->setMinimum(0.0f);
	m_ui.Del1BpmKnob->setMaximum(3.6f);


	// GEN1
	setParamKnob(drumkv1::GEN1_SAMPLE, m_ui.Gen1SampleKnob);
	setParamKnob(drumkv1::GEN1_LOOP,   m_ui.Gen1LoopKnob);
	setParamKnob(drumkv1::GEN1_OCTAVE, m_ui.Gen1OctaveKnob);
	setParamKnob(drumkv1::GEN1_TUNING, m_ui.Gen1TuningKnob);

	// DCF1
	setParamKnob(drumkv1::DCF1_CUTOFF,   m_ui.Dcf1CutoffKnob);
	setParamKnob(drumkv1::DCF1_RESO,     m_ui.Dcf1ResoKnob);
	setParamKnob(drumkv1::DCF1_TYPE,     m_ui.Dcf1TypeKnob);
	setParamKnob(drumkv1::DCF1_SLOPE,    m_ui.Dcf1SlopeKnob);
	setParamKnob(drumkv1::DCF1_ENVELOPE, m_ui.Dcf1EnvelopeKnob);
	setParamKnob(drumkv1::DCF1_ATTACK,   m_ui.Dcf1AttackKnob);
	setParamKnob(drumkv1::DCF1_DECAY1,   m_ui.Dcf1Decay1Knob);
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
		m_ui.Lfo1Env, SIGNAL(decay2Changed(float)),
		m_ui.Lfo1Decay2Knob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Lfo1Decay2Knob, SIGNAL(valueChanged(float)),
		m_ui.Lfo1Env, SLOT(setDecay2(float)));

	// DCA1
	setParamKnob(drumkv1::DCA1_VOLUME, m_ui.Dca1VolumeKnob);
	setParamKnob(drumkv1::DCA1_ATTACK, m_ui.Dca1AttackKnob);
	setParamKnob(drumkv1::DCA1_DECAY1, m_ui.Dca1Decay1Knob);
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
		m_ui.Dca1Env, SIGNAL(decay2Changed(float)),
		m_ui.Dca1Decay2Knob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dca1Decay2Knob, SIGNAL(valueChanged(float)),
		m_ui.Dca1Env, SLOT(setDecay2(float)));

	// DEF1
	setParamKnob(drumkv1::DEF1_PITCHBEND, m_ui.Def1PitchbendKnob);
	setParamKnob(drumkv1::DEF1_MODWHEEL,  m_ui.Def1ModwheelKnob);
	setParamKnob(drumkv1::DEF1_PRESSURE,  m_ui.Def1PressureKnob);

	// OUT1
	setParamKnob(drumkv1::OUT1_WIDTH,   m_ui.Out1WidthKnob);
	setParamKnob(drumkv1::OUT1_PANNING, m_ui.Out1PanningKnob);
	setParamKnob(drumkv1::OUT1_VOLUME,  m_ui.Out1VolumeKnob);


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

	// Dynamics
	setParamKnob(drumkv1::DYN1_COMPRESS, m_ui.Dyn1CompressKnob);
	setParamKnob(drumkv1::DYN1_LIMITER,  m_ui.Dyn1LimiterKnob);


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


	// Menu actions
	QObject::connect(m_ui.helpAboutAction,
		SIGNAL(triggered(bool)),
		SLOT(helpAbout()));
	QObject::connect(m_ui.helpAboutQtAction,
		SIGNAL(triggered(bool)),
		SLOT(helpAboutQt()));


	// Epilog.
	QWidget::adjustSize();
}


// Param kbob (widget) map accesors.
void drumkv1widget::setParamKnob ( drumkv1::ParamIndex index, drumkv1widget_knob *pKnob )
{
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
void drumkv1widget::setParamValue ( drumkv1::ParamIndex index, float fValue )
{
	++m_iUpdate;

	drumkv1widget_knob *pKnob = paramKnob(index);
	if (pKnob)
		pKnob->setValue(fValue);

	--m_iUpdate;
}

float drumkv1widget::paramValue ( drumkv1::ParamIndex index ) const
{
	drumkv1widget_knob *pKnob = paramKnob(index);
	return (pKnob ? pKnob->value() : 0.0f);
}


// Param knob (widget) slot.
void drumkv1widget::paramChanged ( float fValue )
{
	if (m_iUpdate > 0)
		return;

	drumkv1widget_knob *pKnob = qobject_cast<drumkv1widget_knob *> (sender());
	if (pKnob)
		updateParam(m_knobParams.value(pKnob), fValue);

	m_ui.Preset->dirtyPreset();
}


// Reset all param default values.
void drumkv1widget::resetParamValues (void)
{
	for (uint32_t i = 0; i < drumkv1::NUM_PARAMS; ++i) {
		drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
		float fValue = drumkv1_default_params[i].value;
		setParamValue(index, fValue);
		updateParam(index, fValue);
	}
}


// Reset all knob default values.
void drumkv1widget::resetParamKnobs (void)
{
	for (uint32_t i = 0; i < drumkv1::NUM_PARAMS; ++i) {
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
void drumkv1widget::newPreset (void)
{
#ifdef CONFIG_DEBUG
	qDebug("drumkv1widget::newPreset()");
#endif

	clearSample();

	resetParamKnobs();
	resetParamValues();

	m_ui.Gen1Sample->openSample();
}


// Preset file I/O slots.
void drumkv1widget::loadPreset ( const QString& sFilename )
{
#ifdef CONFIG_DEBUG
	qDebug("drumkv1widget::loadPreset(\"%s\")", sFilename.toUtf8().constData());
#endif

	QFile file(sFilename);
	if (!file.open(QIODevice::ReadOnly))
		return;

	static QHash<QString, drumkv1::ParamIndex> s_hash;
	if (s_hash.isEmpty()) {
		for (uint32_t i = 0; i < drumkv1::NUM_PARAMS; ++i)
			s_hash.insert(drumkv1_default_params[i].name, drumkv1::ParamIndex(i));
	}

	clearSample();

	resetParamValues();
	resetParamKnobs();

	QDomDocument doc(DRUMKV1_TITLE);
	if (doc.setContent(&file)) {
		QDomElement ePreset = doc.documentElement();
		if (ePreset.tagName() == "preset"
			&& ePreset.attribute("name")
				== QFileInfo(sFilename).completeBaseName()) {
			for (QDomNode nChild = ePreset.firstChild();
					!nChild.isNull();
						nChild = nChild.nextSibling()) {
				QDomElement eChild = nChild.toElement();
				if (eChild.isNull())
					continue;
				if (eChild.tagName() == "samples") {
					for (QDomNode nSample = eChild.firstChild();
							!nSample.isNull();
								nSample = nSample.nextSibling()) {
						QDomElement eSample = nSample.toElement();
						if (eSample.isNull())
							continue;
						if (eSample.tagName() == "sample") {
						//	int index = eSample.attribute("index").toInt();
							loadSample(eSample.text());
						}
					}
				}
				else
				if (eChild.tagName() == "params") {
					for (QDomNode nParam = eChild.firstChild();
							!nParam.isNull();
								nParam = nParam.nextSibling()) {
						QDomElement eParam = nParam.toElement();
						if (eParam.isNull())
							continue;
						if (eParam.tagName() == "param") {
							drumkv1::ParamIndex index = drumkv1::ParamIndex(
								eParam.attribute("index").toULong());
							const QString& sName = eParam.attribute("name");
							if (!sName.isEmpty() && s_hash.contains(sName))
								index = s_hash.value(sName);
							float fValue = eParam.text().toFloat();
							setParamValue(index, fValue);
							updateParam(index, fValue);
						}
					}
				}
			}
		}
	}

	file.close();

	m_ui.Preset->setPreset(QFileInfo(sFilename).completeBaseName());
}


void drumkv1widget::savePreset ( const QString& sFilename )
{
#ifdef CONFIG_DEBUG
	qDebug("drumkv1widget::savePreset(\"%s\")", sFilename.toUtf8().constData());
#endif
	QDomDocument doc(DRUMKV1_TITLE);
	QDomElement ePreset = doc.createElement("preset");
	ePreset.setAttribute("name", QFileInfo(sFilename).completeBaseName());
	ePreset.setAttribute("version", DRUMKV1_VERSION);

	QDomElement eSamples = doc.createElement("samples");
	QDomElement eSample = doc.createElement("sample");
	eSample.setAttribute("index", 0);
	eSample.setAttribute("name", "GEN1_SAMPLE");
	eSample.appendChild(doc.createTextNode(sampleFile()));
	eSamples.appendChild(eSample);
	ePreset.appendChild(eSamples);

	QDomElement eParams = doc.createElement("params");
	for (uint32_t i = 0; i < drumkv1::NUM_PARAMS; ++i) {
		QDomElement eParam = doc.createElement("param");
		eParam.setAttribute("index", QString::number(i));
		eParam.setAttribute("name", drumkv1_default_params[i].name);
		eParam.appendChild(doc.createTextNode(QString::number(
			paramValue(drumkv1::ParamIndex(i)))));
		eParams.appendChild(eParam);
	}
	ePreset.appendChild(eParams);
	doc.appendChild(ePreset);

	QFile file(sFilename);
	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		QTextStream(&file) << doc.toString();
		file.close();
	}
}


// Sample updater (crude experimental stuff II).
void drumkv1widget::updateSample ( drumkv1_sample *pSample, bool bDirty )
{
	m_ui.Gen1Sample->setSample(pSample);

	if (pSample && bDirty)
		m_ui.Preset->dirtyPreset();
}


// MIDI note/octave name helper (static).
QString drumkv1widget::noteName ( int note )
{
	static const char *notes[] =
		{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
	return QString("%1 %2").arg(notes[note % 12]).arg((note / 12) - 1);
}


// Dirty close prompt,
bool drumkv1widget::queryClose (void)
{
	return m_ui.Preset->queryPreset();
}


// Menu actions.
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


// end of drumkv1widget.cpp
