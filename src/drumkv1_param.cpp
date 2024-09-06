// drumkv1_param.cpp
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

#include "drumkv1_param.h"
#include "drumkv1_config.h"

#include <QHash>

#include <QDomDocument>
#include <QTextStream>
#include <QDir>

#include <cmath>


//-------------------------------------------------------------------------
// Abstract/absolute path functors.

QString drumkv1_param::map_path::absolutePath (
	const QString& sAbstractPath ) const
{
	return QDir::current().absoluteFilePath(sAbstractPath);
}

QString drumkv1_param::map_path::abstractPath (
	const QString& sAbsolutePath ) const
{
	return QDir::current().relativeFilePath(sAbsolutePath);
}


//-------------------------------------------------------------------------
// State params description.

enum ParamType { PARAM_FLOAT = 0, PARAM_INT, PARAM_BOOL };

static
struct ParamInfo {

	const char *name;
	ParamType type;
	float def;
	float min;
	float max;

} drumkv1_params[drumkv1::NUM_PARAMS] = {

	// name            type,           def,    min,    max
	{ "GEN1_SAMPLE",   PARAM_INT,    36.0f,   0.0f, 127.0f }, // GEN1 Sample
	{ "GEN1_REVERSE",  PARAM_BOOL,    0.0f,   0.0f,   1.0f }, // GEN1 Reverse
	{ "GEN1_OFFSET",   PARAM_BOOL,    0.0f,   0.0f,   1.0f }, // GEN1 Offset
	{ "GEN1_OFFSET_1", PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // GEN1 Offset Start
	{ "GEN1_OFFSET_2", PARAM_FLOAT,   1.0f,   0.0f,   1.0f }, // GEN1 Offset End
	{ "GEN1_GROUP",    PARAM_FLOAT,   0.0f,   0.0f, 128.0f }, // GEN1 Group
	{ "GEN1_COARSE",   PARAM_FLOAT,   0.0f,  -4.0f,   4.0f }, // GEN1 Coarse
	{ "GEN1_FINE",     PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // GEN1 Fine
	{ "GEN1_ENVTIME",  PARAM_FLOAT,   0.2f,   0.0f,   1.0f }, // GEN1 Env.Time
	{ "DCF1_ENABLED",  PARAM_BOOL,    1.0f,   0.0f,   1.0f }, // DCF1 Enabled
	{ "DCF1_CUTOFF",   PARAM_FLOAT,   1.0f,   0.0f,   1.0f }, // DCF1 Cutoff
	{ "DCF1_RESO",     PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // DCF1 Resonance
	{ "DCF1_TYPE",     PARAM_INT,     0.0f,   0.0f,   3.0f }, // DCF1 Type
	{ "DCF1_SLOPE",    PARAM_INT,     0.0f,   0.0f,   3.0f }, // DCF1 Slope
	{ "DCF1_ENVELOPE", PARAM_FLOAT,   1.0f,  -1.0f,   1.0f }, // DCF1 Envelope
	{ "DCF1_ATTACK",   PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // DCF1 Attack
	{ "DCF1_DECAY1",   PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // DCF1 Decay 1
	{ "DCF1_LEVEL2",   PARAM_FLOAT,   0.2f,   0.0f,   1.0f }, // DCF1 Level 2
	{ "DCF1_DECAY2",   PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // DCF1 Decay 2
	{ "LFO1_ENABLED",  PARAM_BOOL,    1.0f,   0.0f,   1.0f }, // LFO1 Enabled
	{ "LFO1_SHAPE",    PARAM_INT,     1.0f,   0.0f,   4.0f }, // LFO1 Wave Shape
	{ "LFO1_WIDTH",    PARAM_FLOAT,   1.0f,   0.0f,   1.0f }, // LFO1 Wave Width
	{ "LFO1_BPM",      PARAM_FLOAT, 180.0f,   0.0f, 360.0f }, // LFO1 BPM
	{ "LFO1_RATE",     PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // LFO1 Rate
	{ "LFO1_SWEEP",    PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // LFO1 Sweep
	{ "LFO1_PITCH",    PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // LFO1 Pitch
	{ "LFO1_CUTOFF",   PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // LFO1 Cutoff
	{ "LFO1_RESO",     PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // LFO1 Resonance
	{ "LFO1_PANNING",  PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // LFO1 Panning
	{ "LFO1_VOLUME",   PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // LFO1 Volume
	{ "LFO1_ATTACK",   PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // LFO1 Attack
	{ "LFO1_DECAY1",   PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // LFO1 Decay 1
	{ "LFO1_LEVEL2",   PARAM_FLOAT,   0.2f,   0.0f,   1.0f }, // LFO1 Level 2
	{ "LFO1_DECAY2",   PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // LFO1 Decay 2
	{ "DCA1_ENABLED",  PARAM_BOOL,    1.0f,   0.0f,   1.0f }, // DCA1 Enabled
	{ "DCA1_VOLUME",   PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // DCA1 Volume
	{ "DCA1_ATTACK",   PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // DCA1 Attack
	{ "DCA1_DECAY1",   PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // DCA1 Decay1
	{ "DCA1_LEVEL2",   PARAM_FLOAT,   0.2f,   0.0f,   1.0f }, // DCA1 Level 2
	{ "DCA1_DECAY2",   PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // DCA1 Decay 2
	{ "OUT1_WIDTH",    PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // OUT1 Stereo Width
	{ "OUT1_PANNING",  PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // OUT1 Panning
	{ "OUT1_FXSEND",   PARAM_FLOAT,   1.0f,   0.0f,   1.0f }, // OUT1 FX Send
	{ "OUT1_VOLUME",   PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // OUT1 Volume

	{ "DEF1_PITCHBEND",PARAM_FLOAT,   0.2f,   0.0f,   4.0f }, // DEF1 Pitchbend
	{ "DEF1_MODWHEEL", PARAM_FLOAT,   0.2f,   0.0f,   1.0f }, // DEF1 Modwheel
	{ "DEF1_PRESSURE", PARAM_FLOAT,   0.2f,   0.0f,   1.0f }, // DEF1 Pressure
	{ "DEF1_VELOCITY", PARAM_FLOAT,   0.2f,   0.0f,   1.0f }, // DEF1 Velocity
	{ "DEF1_CHANNEL",  PARAM_INT,     0.0f,   0.0f,  16.0f }, // DEF1 Channel
	{ "DEF1_NOTEOFF",  PARAM_INT,     1.0f,   0.0f,   1.0f }, // DEF1 Note Off

	{ "CHO1_WET",      PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // Chorus Wet
	{ "CHO1_DELAY",    PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Chorus Delay
	{ "CHO1_FEEDB",    PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Chorus Feedback
	{ "CHO1_RATE",     PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Chorus Rate
	{ "CHO1_MOD",      PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Chorus Modulation
	{ "FLA1_WET",      PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // Flanger Wet
	{ "FLA1_DELAY",    PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Flanger Delay
	{ "FLA1_FEEDB",    PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Flanger Feedback
	{ "FLA1_DAFT",     PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // Flanger Daft
	{ "PHA1_WET",      PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // Phaser Wet
	{ "PHA1_RATE",     PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Phaser Rate
	{ "PHA1_FEEDB",    PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Phaser Feedback
	{ "PHA1_DEPTH",    PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Phaser Depth
	{ "PHA1_DAFT",     PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // Phaser Daft
	{ "DEL1_WET",      PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // Delay Wet
	{ "DEL1_DELAY",    PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Delay Delay
	{ "DEL1_FEEDB",    PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Delay Feedback
	{ "DEL1_BPM",      PARAM_FLOAT, 180.0f,   0.0f, 360.0f }, // Delay BPM
	{ "REV1_WET",      PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // Reverb Wet
	{ "REV1_ROOM",     PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Reverb Room
	{ "REV1_DAMP",     PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Reverb Damp
	{ "REV1_FEEDB",    PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Reverb Feedback
	{ "REV1_WIDTH",    PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // Reverb Width
	{ "DYN1_COMPRESS", PARAM_BOOL,    0.0f,   0.0f,   1.0f }, // Dynamic Compressor
	{ "DYN1_LIMITER",  PARAM_BOOL,    1.0f,   0.0f,   1.0f }  // Dynamic Limiter
};


const char *drumkv1_param::paramName ( drumkv1::ParamIndex index )
{
	return drumkv1_params[index].name;
}


float drumkv1_param::paramDefaultValue ( drumkv1::ParamIndex index )
{
	return drumkv1_params[index].def;
}


float drumkv1_param::paramSafeValue ( drumkv1::ParamIndex index, float fValue )
{
	const ParamInfo& param = drumkv1_params[index];

	if (param.type == PARAM_BOOL)
		return (fValue > 0.5f ? 1.0f : 0.0f);

	if (fValue < param.min)
		return param.min;
	if (fValue > param.max)
		return param.max;

	if (param.type == PARAM_INT)
		return ::rintf(fValue);
	else
		return fValue;
}


float drumkv1_param::paramValue ( drumkv1::ParamIndex index, float fScale )
{
	const ParamInfo& param = drumkv1_params[index];

	if (param.type == PARAM_BOOL)
		return (fScale > 0.5f ? 1.0f : 0.0f);

	const float fValue = param.min + fScale * (param.max - param.min);

	if (param.type == PARAM_INT)
		return ::rintf(fValue);
	else
		return fValue;
}


float drumkv1_param::paramScale ( drumkv1::ParamIndex index, float fValue )
{
	const ParamInfo& param = drumkv1_params[index];

	if (param.type == PARAM_BOOL)
		return (fValue > 0.5f ? 1.0f : 0.0f);

	const float fScale = (fValue - param.min) / (param.max - param.min);

	if (param.type == PARAM_INT)
		return ::rintf(fScale);
	else
		return fScale;
}


bool drumkv1_param::paramFloat ( drumkv1::ParamIndex index )
{
	return (drumkv1_params[index].type == PARAM_FLOAT);
}



// Element serialization methods.
void drumkv1_param::loadElements (
	drumkv1 *pDrumk, const QDomElement& eElements,
	const drumkv1_param::map_path& mapPath )
{
	if (pDrumk == nullptr)
		return;

	pDrumk->clearElements();

	static QHash<QString, drumkv1::ParamIndex> s_hash;
	if (s_hash.isEmpty()) {
		for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i)
			s_hash.insert(drumkv1_params[i].name, drumkv1::ParamIndex(i));
	}

	for (QDomNode nElement = eElements.firstChild();
			!nElement.isNull();
				nElement = nElement.nextSibling()) {
		QDomElement eElement = nElement.toElement();
		if (eElement.isNull())
			continue;
		if (eElement.tagName() == "element") {
			const int note = eElement.attribute("index").toInt();
			drumkv1_element *element = pDrumk->addElement(note);
			for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i) {
				const drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
				const float fDefValue = paramDefaultValue(index);
				element->setParamValue(index, fDefValue, 0);
				element->setParamValue(index, fDefValue);
			}
			for (QDomNode nChild = eElement.firstChild();
					!nChild.isNull();
						nChild = nChild.nextSibling()) {
				QDomElement eChild = nChild.toElement();
				if (eChild.isNull())
					continue;
				if (eChild.tagName() == "sample") {
				//	const int index = eChild.attribute("index").toInt();
					const uint32_t iOffsetStart
						= eChild.attribute("offset-start").toULong();
					const uint32_t iOffsetEnd
						= eChild.attribute("offset-end").toULong();
					const QString& sSampleFile
						= eChild.text();
					const QByteArray aSampleFile
						= mapPath.absolutePath(
							drumkv1_param::loadFilename(sSampleFile)).toUtf8();
					element->setSampleFile(aSampleFile.constData());
					element->setOffsetRange(iOffsetStart, iOffsetEnd);
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
							if (index == drumkv1::GEN1_OFFSET_1 ||
								index == drumkv1::GEN1_OFFSET_2)
								continue;
							const float fValue
								= drumkv1_param::paramSafeValue(index,
									eParam.text().toFloat());
							element->setParamValue(index, fValue, 0);
							element->setParamValue(index, fValue);
						}
					}
				}
			}
			// Load/correct functional dependent parametrics...
			const bool bOffset
				= element->isOffset();
			const uint32_t iSampleLength
				= element->length();
			const uint32_t iOffsetStart
				= element->offsetStart();
			const uint32_t iOffsetEnd
				= element->offsetEnd();
			const float fOffset_1 = (bOffset && iSampleLength > 0
				? float(iOffsetStart) / float(iSampleLength) : 0.0f);
			const float fOffset_2 = (bOffset && iSampleLength > 0
				? float(iOffsetEnd) / float(iSampleLength) : 1.0f);
			element->setParamValue(drumkv1::GEN1_OFFSET_1, fOffset_1, 0);
			element->setParamValue(drumkv1::GEN1_OFFSET_1, fOffset_1);
			element->setParamValue(drumkv1::GEN1_OFFSET_2, fOffset_2, 0);
			element->setParamValue(drumkv1::GEN1_OFFSET_2, fOffset_2);
		}
	}
}


void drumkv1_param::saveElements (
	drumkv1 *pDrumk, QDomDocument& doc, QDomElement& eElements,
	const drumkv1_param::map_path& mapPath, bool bSymLink )
{
	if (pDrumk == nullptr)
		return;

	for (int note = 0; note < 128; ++note) {
		drumkv1_element *element = pDrumk->element(note);
		if (element == nullptr)
			continue;
		const char *pszSampleFile = element->sampleFile();
		if (pszSampleFile == nullptr)
			continue;
		QDomElement eElement = doc.createElement("element");
		eElement.setAttribute("index", QString::number(note));
	//	eElement.setAttribute("name", noteName(note));
		QDomElement eSample = doc.createElement("sample");
		eSample.setAttribute("index", 0);
		eSample.setAttribute("name", "GEN1_SAMPLE");
		if (element->isOffset()) {
			eSample.setAttribute("offset-start", element->offsetStart());
			eSample.setAttribute("offset-end", element->offsetEnd());
		}
		eSample.appendChild(doc.createTextNode(mapPath.abstractPath(
			drumkv1_param::saveFilename(
				QString::fromUtf8(pszSampleFile), bSymLink))));
		eElement.appendChild(eSample);
		QDomElement eParams = doc.createElement("params");
		for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i) {
			const drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
			if (index == drumkv1::GEN1_OFFSET_1 ||
				index == drumkv1::GEN1_OFFSET_2)
				continue;
			QDomElement eParam = doc.createElement("param");
			eParam.setAttribute("index", QString::number(i));
			eParam.setAttribute("name", drumkv1_params[i].name);
			eParam.appendChild(doc.createTextNode(
				QString::number(element->paramValue(index))));
			eParams.appendChild(eParam);
		}
		eElement.appendChild(eParams);
		eElements.appendChild(eElement);
	}
}


// Preset serialization methods.
bool drumkv1_param::loadPreset (
	drumkv1 *pDrumk, const QString& sFilename )
{
	if (pDrumk == nullptr)
		return false;

	QFileInfo fi(sFilename);
	if (!fi.exists()) {
		drumkv1_config *pConfig = drumkv1_config::getInstance();
		if (pConfig) {
			const QString& sPresetFile
				= pConfig->presetFile(sFilename);
			if (sPresetFile.isEmpty())
				return false;
			fi.setFile(sPresetFile);
			if (!fi.exists())
				return false;
		}
	}

	QFile file(fi.filePath());
	if (!file.open(QIODevice::ReadOnly))
		return false;

	const bool running = pDrumk->running(false);

	pDrumk->setTuningEnabled(false);
	pDrumk->reset();

	static QHash<QString, drumkv1::ParamIndex> s_hash;
	if (s_hash.isEmpty()) {
		for (uint32_t i = drumkv1::NUM_ELEMENT_PARAMS; i < drumkv1::NUM_PARAMS; ++i) {
			const drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
			s_hash.insert(drumkv1_param::paramName(index), index);
		}
	}

	const QDir currentDir(QDir::current());
	QDir::setCurrent(fi.absolutePath());

	QDomDocument doc(PROJECT_NAME);
	if (doc.setContent(&file)) {
		QDomElement ePreset = doc.documentElement();
		if (ePreset.tagName() == "preset") {
		//	&& ePreset.attribute("name") == fi.completeBaseName()) {
			for (QDomNode nChild = ePreset.firstChild();
					!nChild.isNull();
						nChild = nChild.nextSibling()) {
				QDomElement eChild = nChild.toElement();
				if (eChild.isNull())
					continue;
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
							if (!sName.isEmpty()) {
								if (!s_hash.contains(sName))
									continue;
								index = s_hash.value(sName);
							}
							const float fValue = eParam.text().toFloat();
							pDrumk->setParamValue(index,
								drumkv1_param::paramSafeValue(index, fValue));
						}
					}
				}
				else
				if (eChild.tagName() == "elements") {
					drumkv1_param::loadElements(pDrumk, eChild);
				}
				else
				if (eChild.tagName() == "tuning") {
					drumkv1_param::loadTuning(pDrumk, eChild);
				}
			}
		}
	}

	file.close();

	pDrumk->stabilize();
	pDrumk->reset();
	pDrumk->running(running);

	QDir::setCurrent(currentDir.absolutePath());

	return true;
}


bool drumkv1_param::savePreset (
	drumkv1 *pDrumk, const QString& sFilename, bool bSymLink )
{
	if (pDrumk == nullptr)
		return false;

	pDrumk->stabilize();

	const QFileInfo fi(sFilename);
	const QDir currentDir(QDir::current());
	QDir::setCurrent(fi.absolutePath());

	QDomDocument doc(PROJECT_NAME);
	QDomElement ePreset = doc.createElement("preset");
	ePreset.setAttribute("name", fi.completeBaseName());
	ePreset.setAttribute("version", PROJECT_VERSION);

	QDomElement eElements = doc.createElement("elements");
	drumkv1_param::saveElements(pDrumk, doc, eElements, map_path(), bSymLink);
	ePreset.appendChild(eElements);

	QDomElement eParams = doc.createElement("params");
	for (uint32_t i = drumkv1::NUM_ELEMENT_PARAMS; i < drumkv1::NUM_PARAMS; ++i) {
		QDomElement eParam = doc.createElement("param");
		const drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
		eParam.setAttribute("index", QString::number(i));
		eParam.setAttribute("name", drumkv1_param::paramName(index));
		const float fValue = pDrumk->paramValue(index);
		eParam.appendChild(doc.createTextNode(QString::number(fValue)));
		eParams.appendChild(eParam);
	}
	ePreset.appendChild(eParams);
	doc.appendChild(ePreset);

	if (pDrumk->isTuningEnabled()) {
		QDomElement eTuning = doc.createElement("tuning");
		drumkv1_param::saveTuning(pDrumk, doc, eTuning, bSymLink);
		ePreset.appendChild(eTuning);
	}

	QFile file(fi.filePath());
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;

	QTextStream(&file) << doc.toString();
	file.close();

	QDir::setCurrent(currentDir.absolutePath());

	return true;
}


// Tuning serialization methods.
void drumkv1_param::loadTuning (
	drumkv1 *pDrumk, const QDomElement& eTuning )
{
	if (pDrumk == nullptr)
		return;

	pDrumk->setTuningEnabled(eTuning.attribute("enabled").toInt() > 0);

	for (QDomNode nChild = eTuning.firstChild();
			!nChild.isNull();
				nChild = nChild.nextSibling()) {
		QDomElement eChild = nChild.toElement();
		if (eChild.isNull())
			continue;
		if (eChild.tagName() == "enabled") {
			pDrumk->setTuningEnabled(eChild.text().toInt() > 0);
		}
		if (eChild.tagName() == "ref-pitch") {
			pDrumk->setTuningRefPitch(eChild.text().toFloat());
		}
		else
		if (eChild.tagName() == "ref-note") {
			pDrumk->setTuningRefNote(eChild.text().toInt());
		}
		else
		if (eChild.tagName() == "scale-file") {
			const QString& sScaleFile
				= eChild.text();
			const QByteArray aScaleFile
				= drumkv1_param::loadFilename(sScaleFile).toUtf8();
			pDrumk->setTuningScaleFile(aScaleFile.constData());
		}
		else
		if (eChild.tagName() == "keymap-file") {
			const QString& sKeyMapFile
				= eChild.text();
			const QByteArray aKeyMapFile
				= drumkv1_param::loadFilename(sKeyMapFile).toUtf8();
			pDrumk->setTuningScaleFile(aKeyMapFile.constData());
		}
	}

	// Consolidate tuning state...
	pDrumk->updateTuning();
}


void drumkv1_param::saveTuning (
	drumkv1 *pDrumk, QDomDocument& doc, QDomElement& eTuning, bool bSymLink )
{
	if (pDrumk == nullptr)
		return;

	eTuning.setAttribute("enabled", int(pDrumk->isTuningEnabled()));

	QDomElement eRefPitch = doc.createElement("ref-pitch");
	eRefPitch.appendChild(doc.createTextNode(
		QString::number(pDrumk->tuningRefPitch())));
	eTuning.appendChild(eRefPitch);

	QDomElement eRefNote = doc.createElement("ref-note");
	eRefNote.appendChild(doc.createTextNode(
		QString::number(pDrumk->tuningRefNote())));
	eTuning.appendChild(eRefNote);

	const char *pszScaleFile = pDrumk->tuningScaleFile();
	if (pszScaleFile) {
		const QString& sScaleFile
			= QString::fromUtf8(pszScaleFile);
		if (!sScaleFile.isEmpty()) {
			QDomElement eScaleFile = doc.createElement("scale-file");
			eScaleFile.appendChild(doc.createTextNode(
				QDir::current().relativeFilePath(
					drumkv1_param::saveFilename(sScaleFile, bSymLink))));
			eTuning.appendChild(eScaleFile);
		}
	}

	const char *pszKeyMapFile = pDrumk->tuningKeyMapFile();
	if (pszKeyMapFile) {
		const QString& sKeyMapFile
			= QString::fromUtf8(pszKeyMapFile);
		if (!sKeyMapFile.isEmpty()) {
			QDomElement eKeyMapFile = doc.createElement("keymap-file");
			eKeyMapFile.appendChild(doc.createTextNode(
				QDir::current().relativeFilePath(
					drumkv1_param::saveFilename(sKeyMapFile, bSymLink))));
			eTuning.appendChild(eKeyMapFile);
		}
	}
}


// Load/save and convert canonical/absolute filename helpers.
QString drumkv1_param::loadFilename ( const QString& sFilename )
{
	QFileInfo fi(sFilename);
	if (fi.isSymLink())
		fi.setFile(fi.symLinkTarget());
	return fi.filePath();
}


QString drumkv1_param::saveFilename ( const QString& sFilename, bool bSymLink )
{
	QFileInfo fi(sFilename);
	if (bSymLink && fi.absolutePath() != QDir::current().absolutePath()) {
		const QString& sPath = fi.absoluteFilePath();
		const QString& sName = fi.baseName();
		const QString& sExt  = fi.completeSuffix();
		const QString& sLink = sName
			+ '-' + QString::number(qHash(sPath), 16)
			+ '.' + sExt;
		QFile(sPath).link(sLink);
		fi.setFile(QDir::current(), sLink);
	}
	else if (fi.isSymLink()) fi.setFile(fi.symLinkTarget());
	return fi.absoluteFilePath();
}


// end of drumkv1_param.cpp
