// drumkv1widget_lv2.h
//
/****************************************************************************
   Copyright (C) 2012, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __drumkv1widget_lv2_h
#define __drumkv1widget_lv2_h

#include "drumkv1widget.h"

#include "lv2.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"


#define DRUMKV1_LV2UI_URI DRUMKV1_LV2_PREFIX "ui"


// Forward decls.
class drumkv1_lv2;

class QSocketNotifier;


//-------------------------------------------------------------------------
// drumkv1widget_lv2 - decl.
//

class drumkv1widget_lv2 : public drumkv1widget
{
	Q_OBJECT

public:

	// Constructor.
	drumkv1widget_lv2(drumkv1_lv2 *pDrumk,
		LV2UI_Controller controller, LV2UI_Write_Function write_function);

	// Destructor.
	~drumkv1widget_lv2();

	// Plugin port event notification.
	void port_event(uint32_t port_index,
		uint32_t buffer_size, uint32_t format, const void *buffer);

protected slots:

	// Update notification slot.
	void updateNotify();

protected:

	// Synth engine accessor.
	drumkv1 *instance() const;

	// Param port method.
	void updateParam(drumkv1::ParamIndex index, float fValue) const;

private:

	// Instance variables.
	drumkv1_lv2 *m_pDrumk;

	LV2UI_Controller     m_controller;
	LV2UI_Write_Function m_write_function;

	// Update notifier.
	QSocketNotifier *m_pUpdateNotifier;
};


#endif	// __drumkv1widget_lv2_h

// end of drumkv1widget_lv2.h
