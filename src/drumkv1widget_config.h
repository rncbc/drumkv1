// drumkv1widget_config.h
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

#ifndef __drumkv1widget_config_h
#define __drumkv1widget_config_h

#include "ui_drumkv1widget_config.h"

#include "drumkv1_programs.h"
#include "drumkv1_config.h"


//----------------------------------------------------------------------------
// drumkv1widget_config -- UI wrapper form.

class drumkv1widget_config : public QDialog
{
	Q_OBJECT

public:

	// ctor.
	drumkv1widget_config(QWidget *pParent = 0, Qt::WindowFlags wflags = 0);

	// dtor.
	~drumkv1widget_config();

	// programs accessors.
	void setPrograms(drumkv1_programs *pPrograms);
	drumkv1_programs *programs() const;

protected slots:

	// command slots.
	void programsAddBankItem();
	void programsAddItem();
	void programsEditItem();
	void programsDeleteItem();

	// janitor slots.
	void programsCurrentChanged();
	void programsActivated();
	void programsContextMenuRequested(const QPoint&);

	void programsChanged();
	void optionsChanged();

	// dialog slots.
	void accept();
	void reject();

protected:

	// stabilizer.
	void stabilize();

private:

	// UI struct.
	Ui::drumkv1widget_config m_ui;

	// Programs database.
	drumkv1_programs *m_pPrograms;

	// Dialog dirty flag.
	int m_iDirtyPrograms;
	int m_iDirtyOptions;
};


#endif	// __drumkv1widget_config_h

// end of drumkv1widget_config.h
