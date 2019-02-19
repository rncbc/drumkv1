// drumkv1widget_jack.h
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
public:

	// Constructor.
	drumkv1widget_jack(drumkv1_jack *pDrumk);

	// Destructor.
	~drumkv1widget_jack();

#ifdef CONFIG_NSM
	// NSM client accessors.
	void setNsmClient(drumkv1_nsm *pNsmClient);
	drumkv1_nsm *nsmClient() const;
#endif	// CONFIG_NSM

protected:

	// Synth engine accessor.
	drumkv1_ui *ui_instance() const;

	// Param port method.
	void updateParam(drumkv1::ParamIndex index, float fValue) const;

	// Dirty flag method.
	void updateDirtyPreset(bool bDirtyPreset);

	// Application close.
	void closeEvent(QCloseEvent *pCloseEvent);

#ifdef CONFIG_NSM
	// Optional GUI handlers.
	void showEvent(QShowEvent *pShowEvent);
	void hideEvent(QHideEvent *pHideEvent);
#endif	// CONFIG_NSM

private:

	// Instance variables.
	drumkv1     *m_pDrumk;
	drumkv1_ui  *m_pDrumkUi;

#ifdef CONFIG_NSM
	drumkv1_nsm *m_pNsmClient;
#endif
};


#endif	// __drumkv1widget_jack_h

// end of drumkv1widget_jack.h
