// drumkv1_param.cpp
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

#include "drumkv1_param.h"
#include "drumkv1_config.h"

#include <QHash>

#include <QDomDocument>
#include <QTextStream>
#include <QDir>

#include <math.h>


//-------------------------------------------------------------------------
// state params description.

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
	{ "GEN1_GROUP",    PARAM_FLOAT,   0.0f,   0.0f, 128.0f }, // GEN1 Group
	{ "GEN1_COARSE",   PARAM_FLOAT,   0.0f,  -4.0f,   4.0f }, // GEN1 Coarse
	{ "GEN1_FINE",     PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // GEN1 Fine
	{ "GEN1_ENVTIME",  PARAM_FLOAT,   0.2f,   0.0f,   1.0f }, // GEN1 Env.Time
	{ "DCF1_CUTOFF",   PARAM_FLOAT,   1.0f,   0.0f,   1.0f }, // DCF1 Cutoff
	{ "DCF1_RESO",     PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // DCF1 Resonance
	{ "DCF1_TYPE",     PARAM_INT,     0.0f,   0.0f,   3.0f }, // DCF1 Type
	{ "DCF1_SLOPE",    PARAM_INT,     0.0f,   0.0f,   1.0f }, // DCF1 Slope
	{ "DCF1_ENVELOPE", PARAM_FLOAT,   1.0f,  -1.0f,   1.0f }, // DCF1 Envelope
	{ "DCF1_ATTACK",   PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // DCF1 Attack
	{ "DCF1_DECAY1",   PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // DCF1 Decay 1
	{ "DCF1_LEVEL2",   PARAM_FLOAT,   0.2f,   0.0f,   1.0f }, // DCF1 Level 2
	{ "DCF1_DECAY2",   PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // DCF1 Decay 2
	{ "LFO1_SHAPE",    PARAM_INT,     1.0f,   0.0f,   4.0f }, // LFO1 Wave Shape
	{ "LFO1_WIDTH",    PARAM_FLOAT,   1.0f,   0.0f,   1.0f }, // LFO1 Wave Width
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
	{ "DCA1_VOLUME",   PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // DCA1 Volume
	{ "DCA1_ATTACK",   PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // DCA1 Attack
	{ "DCA1_DECAY1",   PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // DCA1 Decay1
	{ "DCA1_LEVEL2",   PARAM_FLOAT,   0.2f,   0.0f,   1.0f }, // DCA1 Level 2
	{ "DCA1_DECAY2",   PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // DCA1 Decay 2
	{ "OUT1_WIDTH",    PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // OUT1 Stereo Width
	{ "OUT1_PANNING",  PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // OUT1 Panning
	{ "OUT1_VOLUME",   PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // OUT1 Volume

	{ "DEF1_PITCHBEND",PARAM_FLOAT,   0.2f,   0.0f,   1.0f }, // DEF1 Pitchbend
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
	{ "DEL1_BPM",      PARAM_FLOAT, 180.0f,   3.6f, 360.0f }, // Delay BPM
	{ "DEL1_BPMSYNC",  PARAM_BOOL,    0.0f,   0.0f,   1.0f }, // Delay BPM (sync)
	{ "DEL1_BPMHOST",  PARAM_FLOAT, 180.0f,   3.6f, 360.0f }, // Delay BPM (host)
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


// Element serialization methods.
void drumkv1_param::loadElements (
	drumkv1 *pDrumk, const QDomElement& eElements,
	const drumkv1_param::map_path& mapPath )
{
	if (pDrumk == NULL)
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
			int note = eElement.attribute("index").toInt();
			drumkv1_element *element = pDrumk->addElement(note);
			for (QDomNode nChild = eElement.firstChild();
					!nChild.isNull();
						nChild = nChild.nextSibling()) {
				QDomElement eChild = nChild.toElement();
				if (eChild.isNull())
					continue;
				if (eChild.tagName() == "sample") {
				//	int index = eChild.attribute("index").toInt();
					const QString& sFilename = eChild.text();
					//	= QFileInfo(eChild.text()).canonicalFilePath();
					element->setSampleFile(
						mapPath.absolutePath(sFilename).toUtf8().constData());
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
							const float fValue = eParam.text().toFloat();
							element->setParamValue(index, fValue, 0);
							element->setParamValue(index, fValue);
						}
					}
				}
			}
		}
	}
}


void drumkv1_param::saveElements (
	drumkv1 *pDrumk, QDomDocument& doc, QDomElement& eElements,
	const drumkv1_param::map_path& mapPath )
{
	if (pDrumk == NULL)
		return;

	for (int note = 0; note < 128; ++note) {
		drumkv1_element *element = pDrumk->element(note);
		if (element == NULL)
			continue;
		const char *pszSampleFile = element->sampleFile();
		if (pszSampleFile == NULL)
			continue;
		QDomElement eElement = doc.createElement("element");
		eElement.setAttribute("index", QString::number(note));
	//	eElement.setAttribute("name", noteName(note));
		QDomElement eSample = doc.createElement("sample");
		eSample.setAttribute("index", 0);
		eSample.setAttribute("name", "GEN1_SAMPLE");
		eSample.appendChild(doc.createTextNode(
			mapPath.abstractPath(QString::fromUtf8(pszSampleFile))));
		eElement.appendChild(eSample);
		QDomElement eParams = doc.createElement("params");
		for (uint32_t i = 0; i < drumkv1::NUM_ELEMENT_PARAMS; ++i) {
			QDomElement eParam = doc.createElement("param");
			eParam.setAttribute("index", QString::number(i));
			eParam.setAttribute("name", drumkv1_params[i].name);
			const drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
			eParam.appendChild(doc.createTextNode(
				QString::number(element->paramValue(index))));
			eParams.appendChild(eParam);
		}
		eElement.appendChild(eParams);
		eElements.appendChild(eElement);
	}
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



// Preset serialization methods.
void drumkv1_param::loadPreset ( drumkv1 *pDrumk, const QString& sFilename )
{
	if (pDrumk == NULL)
		return;

	QFileInfo fi(sFilename);
	if (!fi.exists()) {
		drumkv1_config *pConfig = drumkv1_config::getInstance();
		if (pConfig) {
			const QString& sPresetFile
				= pConfig->presetFile(sFilename);
			if (sPresetFile.isEmpty())
				return;
			fi.setFile(sPresetFile);
			if (!fi.exists())
				return;
		}
	}

	QFile file(fi.filePath());
	if (!file.open(QIODevice::ReadOnly))
		return;

	static QHash<QString, drumkv1::ParamIndex> s_hash;
	if (s_hash.isEmpty()) {
		for (uint32_t i = drumkv1::NUM_ELEMENT_PARAMS; i < drumkv1::NUM_PARAMS; ++i) {
			const drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
			s_hash.insert(drumkv1_param::paramName(index), index);
		}
	}

	const QDir currentDir(QDir::current());
	QDir::setCurrent(fi.absolutePath());

	QDomDocument doc(DRUMKV1_TITLE);
	if (doc.setContent(&file)) {
		QDomElement ePreset = doc.documentElement();
		if (ePreset.tagName() == "preset"
			&& ePreset.attribute("name") == fi.completeBaseName()) {
			for (QDomNode nChild = ePreset.firstChild();
					!nChild.isNull();
						nChild = nChild.nextSibling()) {
				QDomElement eChild = nChild.toElement();
				if (eChild.isNull())
					continue;
				if (eChild.tagName() == "elements") {
					drumkv1_param::loadElements(pDrumk, eChild);
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
							if (!sName.isEmpty()) {
								if (!s_hash.contains(sName))
									continue;
								index = s_hash.value(sName);
							}
							const float fValue = eParam.text().toFloat();
						#if 0//--legacy support < 0.3.0.4
							if (index == drumkv1::DEL1_BPM && fValue < 3.6f)
								fValue *= 100.0f;
						#endif
							pDrumk->setParamValue(index, fValue);
						}
					}
				}
			}
		}
	}

	file.close();

	pDrumk->reset();

	QDir::setCurrent(currentDir.absolutePath());
}


void drumkv1_param::savePreset ( drumkv1 *pDrumk, const QString& sFilename )
{
	if (pDrumk == NULL)
		return;

	const QFileInfo fi(sFilename);
	const QDir currentDir(QDir::current());
	QDir::setCurrent(fi.absolutePath());

	QDomDocument doc(DRUMKV1_TITLE);
	QDomElement ePreset = doc.createElement("preset");
	ePreset.setAttribute("name", fi.completeBaseName());
	ePreset.setAttribute("version", DRUMKV1_VERSION);

	QDomElement eElements = doc.createElement("elements");
	drumkv1_param::saveElements(pDrumk, doc, eElements);
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

	QFile file(fi.filePath());
	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		QTextStream(&file) << doc.toString();
		file.close();
	}

	QDir::setCurrent(currentDir.absolutePath());
}


// end of drumkv1_param.cpp
