// drumkv1widget_jack.cpp
//
/****************************************************************************
   Copyright (C) 2012-2019, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "drumkv1widget_jack.h"

#include "drumkv1widget_palette.h"

#include "drumkv1_jack.h"

#ifdef CONFIG_NSM
#include "drumkv1_nsm.h"
#endif

#include <QApplication>
#include <QFileInfo>
#include <QDir>

#include <QCloseEvent>

#include <QStyleFactory>

#ifndef CONFIG_LIBDIR
#if defined(__x86_64__)
#define CONFIG_LIBDIR CONFIG_PREFIX "/lib64"
#else
#define CONFIG_LIBDIR CONFIG_PREFIX "/lib"
#endif
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#define CONFIG_PLUGINSDIR CONFIG_LIBDIR "/qt4/plugins"
#else
#define CONFIG_PLUGINSDIR CONFIG_LIBDIR "/qt5/plugins"
#endif


//-------------------------------------------------------------------------
// drumkv1widget_jack - impl.
//

// Constructor.
drumkv1widget_jack::drumkv1widget_jack ( drumkv1_jack *pDrumk )
	: drumkv1widget(), m_pDrumk(pDrumk)
	#ifdef CONFIG_NSM
		, m_pNsmClient(nullptr)
	#endif
{
	// Special style paths...
	if (QDir(CONFIG_PLUGINSDIR).exists())
		QApplication::addLibraryPath(CONFIG_PLUGINSDIR);

	// Custom color/style themes...
	drumkv1_config *pConfig = drumkv1_config::getInstance();
	if (pConfig) {
		if (!pConfig->sCustomColorTheme.isEmpty()) {
			QPalette pal;
			if (drumkv1widget_palette::namedPalette(
					pConfig, pConfig->sCustomColorTheme, pal))
				QApplication::setPalette(pal);
		}
		if (!pConfig->sCustomStyleTheme.isEmpty()) {
			QApplication::setStyle(
				QStyleFactory::create(pConfig->sCustomStyleTheme));
		}
	}

	// Initialize (user) interface stuff...
	m_pDrumkUi = new drumkv1_ui(m_pDrumk, false);

	// Initialise preset stuff...
	clearPreset();

	// Initial update, always...
	refreshElements();
	activateElement();

	resetParamValues(drumkv1::NUM_PARAMS);
	resetParamKnobs(drumkv1::NUM_PARAMS);

	// May initialize the scheduler/work notifier.
	openSchedNotifier();
}


// Destructor.
drumkv1widget_jack::~drumkv1widget_jack (void)
{
	delete m_pDrumkUi;
}


// Synth engine accessor.
drumkv1_ui *drumkv1widget_jack::ui_instance (void) const
{
	return m_pDrumkUi;
}

#ifdef CONFIG_NSM

// NSM client accessors.
void drumkv1widget_jack::setNsmClient ( drumkv1_nsm *pNsmClient )
{
	m_pNsmClient = pNsmClient;
}

drumkv1_nsm *drumkv1widget_jack::nsmClient (void) const
{
	return m_pNsmClient;
}

#endif	// CONFIG_NSM


// Param port method.
void drumkv1widget_jack::updateParam (
	drumkv1::ParamIndex index, float fValue ) const
{
	m_pDrumkUi->setParamValue(index, fValue);
}


// Dirty flag method.
void drumkv1widget_jack::updateDirtyPreset ( bool bDirtyPreset )
{
	drumkv1widget::updateDirtyPreset(bDirtyPreset);

#ifdef CONFIG_NSM
	if (m_pNsmClient && m_pNsmClient->is_active())
		m_pNsmClient->dirty(true); // as far as NSM goes, we're always filthy!
#endif
}


// Application close.
void drumkv1widget_jack::closeEvent ( QCloseEvent *pCloseEvent )
{
#ifdef CONFIG_NSM
	if (m_pNsmClient && m_pNsmClient->is_active())
		drumkv1widget::updateDirtyPreset(false);
#endif

	// Let's be sure about that...
	if (queryClose()) {
		pCloseEvent->accept();
		QApplication::quit();
	} else {
		pCloseEvent->ignore();
	}
}


#ifdef CONFIG_NSM

// Optional GUI handlers.
void drumkv1widget_jack::showEvent ( QShowEvent *pShowEvent )
{
	drumkv1widget::showEvent(pShowEvent);

	if (m_pNsmClient)
		m_pNsmClient->visible(true);
}

void drumkv1widget_jack::hideEvent ( QHideEvent *pHideEvent )
{
	if (m_pNsmClient)
		m_pNsmClient->visible(false);

	drumkv1widget::hideEvent(pHideEvent);
}

#endif	// CONFIG_NSM


// end of drumkv1widget_jack.cpp
