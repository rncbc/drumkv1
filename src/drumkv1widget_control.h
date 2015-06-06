// drumkv1widget_control.h
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

#ifndef __drumkv1widget_control_h
#define __drumkv1widget_control_h

#include "ui_drumkv1widget_control.h"

#include "drumkv1_controls.h"
#include "drumkv1_param.h"

// forward decls.
class QCloseEvent;


//----------------------------------------------------------------------------
// drumkv1widget_control -- UI wrapper form.

class drumkv1widget_control : public QDialog
{
	Q_OBJECT

public:

	// Pseudo-singleton instance.
	static drumkv1widget_control *getInstance();

	// Pseudo-constructor.
	static void showInstance(
		drumkv1_controls *pControls, drumkv1::ParamIndex index,
		QWidget *pParent = 0, Qt::WindowFlags wflags = 0);

	// Control accessors.
	void setControls(drumkv1_controls *pControls, drumkv1::ParamIndex index);
	drumkv1_controls *controls() const;
	drumkv1::ParamIndex controlIndex() const;

	// Process incoming controller key event.
	void setControlKey(const drumkv1_controls::Key& key);
	drumkv1_controls::Key controlKey() const;

protected slots:

	void changed();

	void accept();
	void reject();

	void activateControlType(int);
	void editControlParamFinished();

	void stabilize();

protected:

	// Constructor.
	drumkv1widget_control(QWidget *pParent = 0, Qt::WindowFlags wflags = 0);

	// Destructor.
	~drumkv1widget_control();

	// Pseudo-destructor.
	void closeEvent(QCloseEvent *pCloseEvent);

	// Control type dependency refresh.
	void updateControlType(int iControlType);

	void setControlType(drumkv1_controls::Type ctype);
	drumkv1_controls::Type controlType() const;

	void setControlParam(unsigned short param);
	unsigned short controlParam() const;

	unsigned short controlChannel() const;

	drumkv1_controls::Type controlTypeFromIndex (int iIndex) const;
	int indexFromControlType(drumkv1_controls::Type ctype) const;

	unsigned short controlParamFromIndex(int iIndex) const;
	int indexFromControlParam(unsigned short param) const;

private:

	// The Qt-designer UI struct...
	Ui::drumkv1widget_control m_ui;

	// Instance variables.
	drumkv1_controls *m_pControls;

	// Target subject.
	drumkv1::ParamIndex m_index;

	// Instance variables.
	int m_iControlParamUpdate;

	int m_iDirtyCount;
	int m_iDirtySetup;

	// Pseudo-singleton instance.
	static drumkv1widget_control *g_pInstance;
};


#endif	// __drumkv1widget_control_h


// end of drumkv1widget_control.h
