// drumkv1widget_config.h
//
/****************************************************************************
   Copyright (C) 2012-2020, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "drumkv1_config.h"

#include <QDialog>


// forward decls.
namespace Ui { class drumkv1widget_config; }

class drumkv1_ui;

class QComboBox;
class QFileInfo;


//----------------------------------------------------------------------------
// drumkv1widget_config -- UI wrapper form.

class drumkv1widget_config : public QDialog
{
	Q_OBJECT

public:

	// ctor.
	drumkv1widget_config(drumkv1_ui *pDrumkUi, QWidget *pParent = nullptr);

	// dtor.
	~drumkv1widget_config();

	// UI instance accessors.
	drumkv1_ui *ui_instance() const;

protected slots:

	// command slots.
	void editCustomColorThemes();

	void controlsAddItem();
	void controlsEditItem();
	void controlsDeleteItem();

	void programsAddBankItem();
	void programsAddItem();
	void programsEditItem();
	void programsDeleteItem();

	// janitorial slots.
	void controlsCurrentChanged();
	void controlsContextMenuRequested(const QPoint&);

	void programsCurrentChanged();
	void programsActivated();
	void programsContextMenuRequested(const QPoint&);

	void controlsEnabled(bool);
	void programsEnabled(bool);

	void tuningTabChanged(int);
	void tuningRefNoteClicked();
	void tuningScaleFileClicked();
	void tuningKeyMapFileClicked();

	void tuningChanged();
	void controlsChanged();
	void programsChanged();
	void optionsChanged();

	// dialog slots.
	void accept();
	void reject();

protected:

	// Custom color/style themes settlers.
	void resetCustomColorThemes(const QString& sCustomColorTheme);
	void resetCustomStyleThemes(const QString& sCustomColorTheme);

	// Combo box history persistence helper prototypes.
	void loadComboBoxHistory(QComboBox *pComboBox);
	void saveComboBoxHistory(QComboBox *pComboBox);

	// Combo box settter/gettter helper prototypes.
	bool setComboBoxCurrentItem(QComboBox *pComboBox, const QFileInfo& info);
	QString comboBoxCurrentItem(QComboBox *pComboBox);

	// stabilizer.
	void stabilize();

private:

	// UI struct.
	Ui::drumkv1widget_config *p_ui;
	Ui::drumkv1widget_config& m_ui;

	// Instance reference.
	drumkv1_ui *m_pDrumkUi;

	// Dialog dirty flag.
	int m_iDirtyTuning;
	int m_iDirtyControls;
	int m_iDirtyPrograms;
	int m_iDirtyOptions;
};


#endif	// __drumkv1widget_config_h

// end of drumkv1widget_config.h
