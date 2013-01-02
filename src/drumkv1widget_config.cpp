// drumkv1widget_config.cpp
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

#include "drumkv1widget_config.h"


//-------------------------------------------------------------------------
// drumkv1widget_config - Prototype settings structure (pseudo-singleton).
//

// Singleton instance accessor (static).
drumkv1widget_config *drumkv1widget_config::g_pSettings = NULL;

drumkv1widget_config *drumkv1widget_config::getInstance (void)
{
	return g_pSettings;
}


// Constructor.
drumkv1widget_config::drumkv1widget_config (void)
	: QSettings(DRUMKV1_DOMAIN, DRUMKV1_TITLE)
{
	g_pSettings = this;

	load();
}


// Default destructor.
drumkv1widget_config::~drumkv1widget_config (void)
{
	save();

	g_pSettings = NULL;
}


// Explicit I/O methods.
void drumkv1widget_config::load (void)
{
	QSettings::beginGroup("/Default");
	sPreset = QSettings::value("/Preset").toString();
	sPresetDir = QSettings::value("/PresetDir").toString();
	sSampleDir = QSettings::value("/SampleDir").toString();
	QSettings::endGroup();
}


void drumkv1widget_config::save (void)
{
	QSettings::beginGroup("/Program");
	QSettings::setValue("/Version", DRUMKV1_VERSION);
	QSettings::endGroup();

	QSettings::beginGroup("/Default");
	QSettings::setValue("/Preset", sPreset);
	QSettings::setValue("/PresetDir", sPresetDir);
	QSettings::setValue("/SampleDir", sSampleDir);
	QSettings::endGroup();

	QSettings::sync();
}


// end of drumkv1widget_config.cpp
