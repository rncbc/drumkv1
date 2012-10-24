// drumkv1widget_elements.cpp
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

#include "drumkv1widget_elements.h"

#include "drumkv1widget.h"

#include "drumkv1.h"

#include <QAbstractItemModel>
#include <QHeaderView>
#include <QFileInfo>


//----------------------------------------------------------------------------
// drumkv1widget_elements_model -- List model.

class drumkv1widget_elements_model : public QAbstractItemModel
{
public:

	// Constructor.
	drumkv1widget_elements_model(drumkv1 *pDrumk, QObject *pParent = NULL);

	// Concretizers (virtual).
	int rowCount(const QModelIndex& parent = QModelIndex()) const;
	int columnCount(const QModelIndex& parent = QModelIndex()) const;

	QVariant headerData(int section, Qt::Orientation orient, int role) const;
	QVariant data(const QModelIndex& index, int role) const;

	QModelIndex index(int row, int column,
		const QModelIndex& parent = QModelIndex()) const;

	QModelIndex parent(const QModelIndex&) const;

	void reset();

	// Accessor specific.
	drumkv1 *instance() const;

protected:

	// Other specifics
	drumkv1_element *elementFromIndex(const QModelIndex& index) const;

	QString itemDisplay(const QModelIndex& index) const;
	QString itemToolTip(const QModelIndex& index) const;

	int columnAlignment(int column) const;

private:

	// Model variables.
	QStringList m_headers;

	drumkv1 *m_pDrumk;
};


// Constructor.
drumkv1widget_elements_model::drumkv1widget_elements_model (
	drumkv1 *pDrumk, QObject *pParent )
	: QAbstractItemModel(pParent), m_pDrumk(pDrumk)
{
	m_headers
		<< tr("Element")
		<< tr("Sample");

	reset();
}


int drumkv1widget_elements_model::rowCount (
	const QModelIndex& /*parent*/ ) const
{
	return 128;
}


int drumkv1widget_elements_model::columnCount (
	const QModelIndex& /*parent*/ ) const
{
	return m_headers.count();
}


QVariant drumkv1widget_elements_model::headerData (
	int section, Qt::Orientation orient, int role ) const
{
	if (orient == Qt::Horizontal) {
		switch (role) {
		case Qt::DisplayRole:
			return m_headers.at(section);
		case Qt::TextAlignmentRole:
			return columnAlignment(section);
		default:
			break;
		}
	}
	return QVariant();
}


QVariant drumkv1widget_elements_model::data (
	const QModelIndex& index, int role ) const
{
	switch (role) {
	case Qt::DisplayRole:
		return itemDisplay(index);
	case Qt::TextAlignmentRole:
		return columnAlignment(index.column());
	case Qt::ToolTipRole:
		return itemToolTip(index);
	default:
		break;
	}
	return QVariant();
}


QModelIndex drumkv1widget_elements_model::index (
	int row, int column, const QModelIndex& /*parent*/) const
{
	return createIndex(row, column, (m_pDrumk ? m_pDrumk->element(row) : NULL));
}


QModelIndex drumkv1widget_elements_model::parent ( const QModelIndex& ) const
{
	return QModelIndex();
}


drumkv1_element *drumkv1widget_elements_model::elementFromIndex (
	const QModelIndex& index ) const
{
	return static_cast<drumkv1_element *> (index.internalPointer());
}


drumkv1 *drumkv1widget_elements_model::instance (void) const
{
	return m_pDrumk;
}


void drumkv1widget_elements_model::reset (void)
{
	return QAbstractItemModel::reset();
}


QString drumkv1widget_elements_model::itemDisplay (
	const QModelIndex& index ) const
{
	const QString sDash('-');
	switch (index.column()) {
	case 0: // Element.
		return drumkv1widget::completeNoteName(index.row());
	case 1: // Sample.
		drumkv1_element *element = elementFromIndex(index);
		if (element) {
			const char *pszSampleFile = element->sampleFile();
			if (pszSampleFile)
				return QFileInfo(pszSampleFile).completeBaseName();
			else
				return tr("(None)");
		}
	}
	return sDash;
}


QString drumkv1widget_elements_model::itemToolTip (
	const QModelIndex& index ) const
{
	QString sToolTip = '[' + drumkv1widget::completeNoteName(index.row()) + ']';
	drumkv1_element *element = elementFromIndex(index);
	if (element) {
		const char *pszSampleFile = element->sampleFile();
		if (pszSampleFile) {
			sToolTip += '\n';
			sToolTip += QFileInfo(pszSampleFile).completeBaseName();
		}
	}
	return sToolTip;
}


int drumkv1widget_elements_model::columnAlignment( int /*column*/ ) const
{
	return int(Qt::AlignLeft | Qt::AlignVCenter);
}


//----------------------------------------------------------------------------
// drumkv1widget_elements -- Custom (tree) list view.

// Constructor.
drumkv1widget_elements::drumkv1widget_elements ( QWidget *pParent )
	: QTreeView(pParent), m_pModel(NULL)
{
}


// Destructor.
drumkv1widget_elements::~drumkv1widget_elements (void)
{
	if (m_pModel)
		delete m_pModel;
}


// Settlers.
void drumkv1widget_elements::setInstance ( drumkv1 *pDrumk )
{
	if (m_pModel)
		delete m_pModel;

	m_pModel = new drumkv1widget_elements_model(pDrumk);

	QTreeView::setModel(m_pModel);

	QTreeView::setSelectionMode(QAbstractItemView::SingleSelection);
	QTreeView::setRootIsDecorated(false);
	QTreeView::setUniformRowHeights(true);
	QTreeView::setItemsExpandable(false);
	QTreeView::setAllColumnsShowFocus(true);
	QTreeView::setAlternatingRowColors(true);

	QHeaderView *pHeader = QTreeView::header();
	pHeader->setResizeMode(QHeaderView::ResizeToContents);
//	pHeader->setDefaultAlignment(Qt::AlignLeft);
	pHeader->setStretchLastSection(true);

	// Element selectors
	QObject::connect(QTreeView::selectionModel(),
		SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&)),
		SLOT(currentRowChanged(const QModelIndex&, const QModelIndex&)));
	QObject::connect(this,
		SIGNAL(doubleClicked(const QModelIndex&)),
		SLOT(doubleClicked(const QModelIndex&)));
}


drumkv1 *drumkv1widget_elements::instance (void) const
{
	return (m_pModel ? m_pModel->instance() : NULL);
}


// Current element accessors.
void drumkv1widget_elements::setCurrentIndex ( int row )
{
	QTreeView::setCurrentIndex(m_pModel->index(row, 0));
}

int drumkv1widget_elements::currentIndex (void) const
{
	return QTreeView::currentIndex().row();
}


// Internal slot handlers.
void drumkv1widget_elements::currentRowChanged (
	const QModelIndex& current, const QModelIndex& /*previous*/ )
{
	emit itemActivated(current.row());
}


void drumkv1widget_elements::doubleClicked ( const QModelIndex& index )
{
	emit itemDoubleClicked(index.row());
}


// Refreshne0r.
void drumkv1widget_elements::refresh (void)
{
	if (m_pModel == NULL)
		return;

	QItemSelectionModel *pSelectionModel = QTreeView::selectionModel();
	const QModelIndex& index = pSelectionModel->currentIndex();

	m_pModel->reset();

	pSelectionModel->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
}


// end of drumkv1widget_elements.cpp
