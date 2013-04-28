// drumkv1widget_jack.h
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

#ifndef __drumkv1widget_jack_h
#define __drumkv1widget_jack_h

#include "drumkv1widget.h"


// Forward decls.
class drumkv1_jack;

#ifdef CONFIG_NSM
class drumkv1_nsm;
#endif


//-------------------------------------------------------------------------
// drumkv1widget_jack - decl.
//

class drumkv1widget_jack : public drumkv1widget
{
	Q_OBJECT

public:

	// Constructor.
	drumkv1widget_jack(drumkv1_jack *pDrumk);

	// Destructor.
	~drumkv1widget_jack();

#ifdef CONFIG_JACK_SESSION

	// JACK session self-notification.
	void notifySessionEvent(void *pvSessionArg);

signals:

	// JACK session notify event.
	void sessionNotify(void *);

protected slots:

	// JACK session event handler.
	void sessionEvent(void *pvSessionArg);

#endif	// CONFIG_JACK_SESSION

#ifdef CONFIG_NSM

protected slots:

	// NSM callback slots.
	void openSession();
	void saveSession();
	
#endif	// CONFIG_NSM

protected:

	// Synth engine accessor.
	drumkv1 *instance() const;

	// Param port method.
	void updateParam(drumkv1::ParamIndex index, float fValue) const;

	// Dirty flag method.
	void updateDirtyPreset(bool bDirtyPreset);

	// Application close.
	void closeEvent(QCloseEvent *pCloseEvent);

private:

	// Instance variables.
	drumkv1_jack *m_pDrumk;

#ifdef CONFIG_NSM
	drumkv1_nsm *m_pNsmClient;
	bool m_bNsmDirty;
#endif
};


#endif	// __drumkv1widget_jack_h

// end of drumkv1widget_jack.h
