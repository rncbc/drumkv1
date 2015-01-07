// drumkv1widget_programs.h
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

#ifndef __drumkv1widget_programs_h
#define __drumkv1widget_programs_h

#include <QItemDelegate>
#include <QTreeWidget>


// forward decls.
class drumkv1_programs;


//----------------------------------------------------------------------------
// drumkv1widget_programs_item_delegate -- Custom (tree) list item delegate.

class drumkv1widget_programs_item_delegate : public QItemDelegate
{
	Q_OBJECT

public:

	// ctor.
	drumkv1widget_programs_item_delegate(QObject *pParent = 0);

	// QItemDelegate interface...
	QSize sizeHint(
		const QStyleOptionViewItem& option,
		const QModelIndex& index) const;

	QWidget *createEditor(QWidget *pParent,
		const QStyleOptionViewItem& option,
		const QModelIndex& index) const;

	void setEditorData(QWidget *pEditor,
		const QModelIndex& index) const;

	void setModelData(QWidget *pEditor,
		QAbstractItemModel *pModel,
		const QModelIndex& index) const;
};



//----------------------------------------------------------------------------
// drumkv1widget_programs -- Custom (tree) widget.

class drumkv1widget_programs : public QTreeWidget
{
	Q_OBJECT

public:

	// ctor.
	drumkv1widget_programs(QWidget *pParent = 0);
	// dtor.
	~drumkv1widget_programs();

	// utilities.
	void loadPrograms(drumkv1_programs *pPrograms);
	void savePrograms(drumkv1_programs *pPrograms);

	void selectProgram(drumkv1_programs *pPrograms);

public slots:

	// slots.
	void addBankItem();
	void addProgramItem();

protected slots:

	// private slots.
	void itemChangedSlot(QTreeWidgetItem *, int);

	void itemExpandedSlot(QTreeWidgetItem *);
	void itemCollapsedSlot(QTreeWidgetItem *);

protected:

	// factory methods.
	QTreeWidgetItem *newBankItem();
	QTreeWidgetItem *newProgramItem();
};


#endif	// __drumkv1widget_programs_h

// end of drumkv1widget_programs.h
