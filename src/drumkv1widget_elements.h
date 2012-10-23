// drumkv1widget_elements.h
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

#ifndef __drumkv1widget_elements_h
#define __drumkv1widget_elements_h

#include <QAbstractItemModel>
#include <QItemDelegate>
#include <QTreeView>


// Forwards.
class drumkv1_element;
class drumkv1;


//----------------------------------------------------------------------------
// drumkv1widget_elements_model -- List model.

class drumkv1widget_elements_model : public QAbstractItemModel
{
	Q_OBJECT

public:

	// Constructor.
	drumkv1widget_elements_model(drumkv1 *pDrumk, QObject *pParent = 0);

	// Concretizers (virtual).
	int rowCount(const QModelIndex& parent = QModelIndex()) const;
	int columnCount(const QModelIndex& parent = QModelIndex()) const;

	QVariant headerData(int section, Qt::Orientation orient, int role) const;
	QVariant data(const QModelIndex& index, int role) const;

	QModelIndex index(int row, int column,
		const QModelIndex& parent = QModelIndex()) const;

	QModelIndex parent(const QModelIndex&) const;

	drumkv1 *instance() const;

	void reset();

protected:

	// Specifics.
	drumkv1_element *elementFromIndex(const QModelIndex& index) const;

	QString itemDisplay(const QModelIndex& index) const;
	QString itemToolTip(const QModelIndex& index) const;

	int columnAlignment(int column) const;

private:

	// Model variables.
	QStringList m_headers;
	drumkv1    *m_pDrumk;
};


//----------------------------------------------------------------------------
// drumkv1widget_elements -- Custom (tree) list view.

class drumkv1widget_elements : public QTreeView
{
	Q_OBJECT

public:

	// Constructor.
	drumkv1widget_elements(QWidget *pParent = 0);

	// Destructor.
	~drumkv1widget_elements();

	// Settlers.
	void setInstance(drumkv1 *pDrumk);
	drumkv1 *instance() const;

	// Current element accessors.
	void setCurrentIndex(int row);
	int currentIndex() const;

	// Refreshener.
	void refresh();

signals:

	void itemActivated(int);
	void itemDoubleClicked(int);

protected slots:

	// Internal slot handlers.
	void currentRowChanged(const QModelIndex&, const QModelIndex&);
	void doubleClicked(const QModelIndex&);

private:

	// Instance variables.
	drumkv1widget_elements_model *m_pModel;
};


#endif  // __drumkv1widget_elements_h

// end of drumkv1widget_elements.h
