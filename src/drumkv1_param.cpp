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


//-------------------------------------------------------------------------
// default state (params)

static
struct {

	const char *name;
	float value;

} drumkv1_default_params[drumkv1::NUM_PARAMS] = {

	{ "GEN1_SAMPLE",   36.0f }, // Bass Drum 1 (GM) aka. C2 (36)
	{ "GEN1_REVERSE",   0.0f },
	{ "GEN1_GROUP",     0.0f },
	{ "GEN1_COARSE",    0.0f },
	{ "GEN1_FINE",      0.0f },
	{ "GEN1_ENVTIME",   0.2f },
	{ "DCF1_CUTOFF",    1.0f }, // 0.5f
	{ "DCF1_RESO",      0.0f },
	{ "DCF1_TYPE",      0.0f },
	{ "DCF1_SLOPE",     0.0f },
	{ "DCF1_ENVELOPE",  1.0f },
	{ "DCF1_ATTACK",    0.0f },
	{ "DCF1_DECAY1",    0.5f },
	{ "DCF1_LEVEL2",    0.2f },
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
	{ "LFO1_DECAY1",    0.5f },
	{ "LFO1_LEVEL2",    0.2f },
	{ "LFO1_DECAY2",    0.5f },
	{ "DCA1_VOLUME",    0.5f },
	{ "DCA1_ATTACK",    0.0f },
	{ "DCA1_DECAY1",    0.5f },
	{ "DCA1_LEVEL2",    0.2f },
	{ "DCA1_DECAY2",    0.5f },
	{ "OUT1_WIDTH",     0.0f },
	{ "OUT1_PANNING",   0.0f },
	{ "OUT1_VOLUME",    0.5f },

	{ "DEF1_PITCHBEND", 0.2f },
	{ "DEF1_MODWHEEL",  0.2f },
	{ "DEF1_PRESSURE",  0.2f },
	{ "DEF1_VELOCITY",  0.2f },
	{ "DEF1_CHANNEL",   0.0f },
	{ "DEF1_NOTEOFF",   1.0f },

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
	{ "DEL1_BPM",     180.0f },
	{ "DEL1_BPMSYNC",   0.0f },
	{ "DEL1_BPMHOST", 180.0f },
	{ "REV1_WET",       0.0f },
	{ "REV1_ROOM",      0.5f },
	{ "REV1_DAMP",      0.5f },
	{ "REV1_FEEDB",     0.5f },
	{ "REV1_WIDTH",     0.0f },
	{ "DYN1_COMPRESS",  0.0f },
	{ "DYN1_LIMITER",   1.0f }
};


const char *drumkv1_param::paramName ( drumkv1::ParamIndex index )
{
	return drumkv1_default_params[index].name;
}


float drumkv1_param::paramDefaultValue ( drumkv1::ParamIndex index )
{
	return drumkv1_default_params[index].value;
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
			s_hash.insert(drumkv1_default_params[i].name, drumkv1::ParamIndex(i));
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
					const QString& sFilename
						= QFileInfo(eChild.text()).canonicalFilePath();
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
			eParam.setAttribute("name", drumkv1_default_params[i].name);
			drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
			eParam.appendChild(doc.createTextNode(
				QString::number(element->paramValue(index))));
			eParams.appendChild(eParam);
		}
		eElement.appendChild(eParams);
		eElements.appendChild(eElement);
	}
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
			drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
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
							float fValue = eParam.text().toFloat();
						#if 1//--legacy support < 0.3.0.4
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
		drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
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
