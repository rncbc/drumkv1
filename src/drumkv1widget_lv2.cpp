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
drumkv1widget_lv2::drumkv1widget_lv2 ( drumkv1_lv2 *pSampl,
	LV2UI_Controller controller, LV2UI_Write_Function write_function )
	: drumkv1widget(), m_pSampl(pSampl)
{
	m_controller = controller;
	m_write_function = write_function;

	// Update notifier setup.
	m_pUpdateNotifier = new QSocketNotifier(
		m_pSampl->update_fds(1), QSocketNotifier::Read, this);

	QObject::connect(m_pUpdateNotifier,
		SIGNAL(activated(int)),
		SLOT(updateNotify()));

	// Initial update, always...
	if (m_pSampl->sampleFile())
		updateSample(m_pSampl->sample());
	else
		initPreset();
}


// Destructor.
drumkv1widget_lv2::~drumkv1widget_lv2 (void)
{
	delete m_pUpdateNotifier;
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


// Sample reset slot.
void drumkv1widget_lv2::clearSample (void)
{
#ifdef CONFIG_DEBUG
	qDebug("drumkv1widget_lv2::clearSample()");
#endif
	m_pSampl->setSampleFile(0);

	updateSample(0);
}


// Sample loader slot.
void drumkv1widget_lv2::loadSample ( const QString& sFilename )
{
#ifdef CONFIG_DEBUG
	qDebug("drumkv1widget_lv2::loadSample(\"%s\")", sFilename.toUtf8().constData());
#endif
	m_pSampl->setSampleFile(sFilename.toUtf8().constData());

	updateSample(m_pSampl->sample(), true);
}


// Sample filename retriever (crude experimental stuff III).
QString drumkv1widget_lv2::sampleFile (void) const
{
	return QString::fromUtf8(m_pSampl->sampleFile());
}


// Param method.
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

	updateSample(m_pSampl->sample());

	for (uint32_t i = 0; i < drumkv1::NUM_PARAMS; ++i) {
		drumkv1::ParamIndex index = drumkv1::ParamIndex(i);
		const float *pfValue = m_pSampl->paramPort(index);
		setParamValue(index, (pfValue ? *pfValue : 0.0f));
	}

	m_pSampl->update_reset();
}


// end of drumkv1widget_lv2.cpp
