// drumkv1widget_lv2.cpp
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

#include "drumkv1widget_lv2.h"

#include "drumkv1_lv2.h"

#include <QSocketNotifier>


//-------------------------------------------------------------------------
// drumkv1widget_lv2 - impl.
//

// Constructor.
drumkv1widget_lv2::drumkv1widget_lv2 ( drumkv1_lv2 *pDrumk,
	LV2UI_Controller controller, LV2UI_Write_Function write_function )
	: drumkv1widget(), m_pDrumk(pDrumk)
{
	m_controller = controller;
	m_write_function = write_function;

	// Update notifier setup.
	m_pUpdateNotifier = new QSocketNotifier(
		m_pDrumk->update_fds(1), QSocketNotifier::Read, this);

	QObject::connect(m_pUpdateNotifier,
		SIGNAL(activated(int)),
		SLOT(updateNotify()));

	// Initial update, always...
	refreshElements();
}


// Destructor.
drumkv1widget_lv2::~drumkv1widget_lv2 (void)
{
	delete m_pUpdateNotifier;
}


// Synth engine accessor.
drumkv1 *drumkv1widget_lv2::instance (void) const
{
	return m_pDrumk;
}


// Plugin port event notification.
void drumkv1widget_lv2::port_event ( uint32_t port_index,
	uint32_t buffer_size, uint32_t format, const void *buffer )
{
	if (format == 0 && buffer_size == sizeof(float)) {
		float fValue = *(float *) buffer;
		setParamValue(drumkv1::ParamIndex(
			port_index - drumkv1_lv2::ParamBase), fValue);
	}
}


// Param port method.
void drumkv1widget_lv2::updateParam (
	drumkv1::ParamIndex index, float fValue ) const
{
	m_write_function(m_controller,
		drumkv1_lv2::ParamBase + index, sizeof(float), 0, &fValue);
}


// Update notification slot.
void drumkv1widget_lv2::updateNotify (void)
{
#ifdef CONFIG_DEBUG
	qDebug("drumkv1widget_lv2::updateNotify()");
#endif

	updateSample(m_pDrumk->sample());

	for (uint32_t i = 0; i < drumkv1::NUM_PARAMS; ++i) {
		drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
		const float *pfValue = m_pDrumk->paramPort(index);
		setParamValue(index, (pfValue ? *pfValue : 0.0f));
	}

	m_pDrumk->update_reset();
}


// end of drumkv1widget_lv2.cpp
