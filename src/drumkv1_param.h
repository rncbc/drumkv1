// drumkv1_param.h
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

	// Element serialization methods.
	void loadElements(drumkv1 *pDrumk,
		const QDomElement& eElements,
		const map_path& mapPath = map_path());
	void saveElements(drumkv1 *pDrumk,
		QDomDocument& doc, QDomElement& eElements,
		const map_path& mapPath = map_path());

	// Preset serialization methods.
	void loadPreset(drumkv1 *pDrumk,
		const QString& sFilename);
	void savePreset(drumkv1 *pDrumk,
		const QString& sFilename);

	// Default parameter name/value helpers.
	const char *paramName(drumkv1::ParamIndex index);
	float paramDefaultValue(drumkv1::ParamIndex index);
	float paramValue(drumkv1::ParamIndex index, float fValue);
};


#endif	// __drumkv1_param_h

// end of drumkv1_param.h
