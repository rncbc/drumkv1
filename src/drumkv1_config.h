// drumkv1_config.h
//
/****************************************************************************
   Copyright (C) 2012-2023, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __drumkv1_config_h
#define __drumkv1_config_h

#include "config.h"

#define DRUMKV1_TITLE       PACKAGE_NAME

#define DRUMKV1_SUBTITLE    "an old-school drum-kit sampler."
#define DRUMKV1_WEBSITE     "https://drumkv1.sourceforge.io"
#define DRUMKV1_COPYRIGHT   "Copyright (C) 2012-2023, rncbc aka Rui Nuno Capela. All rights reserved."

#define DRUMKV1_DOMAIN      "rncbc.org"


//-------------------------------------------------------------------------
// drumkv1_config - Prototype settings class (singleton).
//

#include <QSettings>
#include <QStringList>

// forward decls.
class drumkv1_programs;
class drumkv1_controls;


class drumkv1_config : public QSettings
{
public:

	// Constructor.
	drumkv1_config();

	// Default destructor.
	~drumkv1_config();

	// Default options...
	QString sPreset;
	QString sPresetDir;
	QString sSampleDir;

	// Knob behavior modes.
	int iKnobDialMode;
	int iKnobEditMode;

	// Special time-formatted spinbox option.
	int iFrameTimeFormat;

	// Default randomize factor (percent).
	float fRandomizePercent;

	// Whether to display GM Standard drum-note/key names.
	bool bUseGMDrumNames;

	// Special persistent options.
	bool bControlsEnabled;
	bool bProgramsEnabled;
	bool bProgramsPreview;
	bool bUseNativeDialogs;
	// Run-time special non-persistent options.
	bool bDontUseNativeDialogs;

	// Custom color palette/widget style themes.
	QString sCustomColorTheme;
	QString sCustomStyleTheme;

	// Micro-tuning options.
	bool    bTuningEnabled;
	float   fTuningRefPitch;
	int     iTuningRefNote;
	QString sTuningScaleDir;
	QString sTuningScaleFile;
	QString sTuningKeyMapDir;
	QString sTuningKeyMapFile;

	// Singleton instance accessor.
	static drumkv1_config *getInstance();

	// Preset utility methods.
	QString presetFile(const QString& sPreset);
	void setPresetFile(const QString& sPreset, const QString& sPresetFile);
	void removePreset(const QString& sPreset);
	const QStringList& presetList();

	// Programs utility methods.
	void loadPrograms(drumkv1_programs *pPrograms);
	void savePrograms(drumkv1_programs *pPrograms);

	// Controllers utility methods.
	void loadControls(drumkv1_controls *pControls);
	void saveControls(drumkv1_controls *pControls);

protected:

	// Preset group path.
	QString presetGroup() const;

	// Banks programs group path.
	QString programsGroup() const;
	QString bankPrefix() const;

	void clearPrograms();

	// Controllers group path.
	QString controlsGroup() const;
	QString controlPrefix() const;

	void clearControls();

	// Explicit I/O methods.
	void load();
	void save();

private:

	// The presets list cache.
	QStringList m_presetList;

	// The singleton instance.
	static drumkv1_config *g_pSettings;
};


#endif	// __drumkv1_config_h

// end of drumkv1_config.h

