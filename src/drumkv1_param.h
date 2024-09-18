// drumkv1_param.h
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

#ifndef __drumkv1_param_h
#define __drumkv1_param_h

#include "drumkv1.h"

#include <QString>

// forward decl.
class QDomElement;
class QDomDocument;


//-------------------------------------------------------------------------
// drumkv1_param - decl.
//

namespace drumkv1_param
{
	// Abstract/absolute path functors.
	class map_path
	{
	public:

		virtual QString absolutePath(const QString& sAbstractPath) const;
		virtual QString abstractPath(const QString& sAbsolutePath) const;
	};

	// Preset initialization method.
	bool newPreset(drumkv1 *pDrumk);

	// Preset serialization methods.
	bool loadPreset(drumkv1 *pDrumk,
		const QString& sFilename);
	bool savePreset(drumkv1 *pDrumk,
		const QString& sFilename,
		bool bSymLink = false);

	// Element serialization methods.
	void loadElements(drumkv1 *pDrumk,
		const QDomElement& eElements,
		const map_path& mapPath = map_path());
	void saveElements(drumkv1 *pDrumk,
		QDomDocument& doc, QDomElement& eElements,
		const map_path& mapPath = map_path(),
		bool bSymLink = false);

	// Tuning serialization methods.
	void loadTuning(drumkv1 *pDrumk,
		const QDomElement& eTuning);
	void saveTuning(drumkv1 *pDrumk,
		QDomDocument& doc, QDomElement& eTuning,
		bool bSymLink = false);

	// Default parameter name/value helpers.
	const char *paramName(drumkv1::ParamIndex index);
	float paramDefaultValue(drumkv1::ParamIndex index);
	float paramSafeValue(drumkv1::ParamIndex index, float fValue);
	float paramValue(drumkv1::ParamIndex index, float fScale);
	float paramScale(drumkv1::ParamIndex index, float fValue);
	bool paramFloat(drumkv1::ParamIndex index);

	// Load/save and convert canonical/absolute filename helpers.
	QString loadFilename(const QString& sFilename);
	QString saveFilename(const QString& sFilename, bool bSymLink);
};


#endif	// __drumkv1_param_h

// end of drumkv1_param.h
