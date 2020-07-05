// drumkv1widget_elements.cpp
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

#include "drumkv1widget_elements.h"

#include "drumkv1widget.h"

#include "drumkv1_ui.h"

#include "drumkv1_sample.h"

#include <QApplication>
#include <QHeaderView>
#include <QFileInfo>
#include <QMimeData>
#include <QDrag>
#include <QUrl>
#include <QIcon>
#include <QPixmap>
#include <QTimer>

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>


//----------------------------------------------------------------------------
// drumkv1widget_elements_model -- List model.

// Constructor.
drumkv1widget_elements_model::drumkv1widget_elements_model (
	drumkv1_ui *pDrumkUi, QObject *pParent )
	: QAbstractItemModel(pParent), m_pDrumkUi(pDrumkUi)
{
	QIcon icon;

	icon.addPixmap(
		QPixmap(":/images/ledOff.png"), QIcon::Normal, QIcon::Off);
	icon.addPixmap(
		QPixmap(":/images/ledOn.png"), QIcon::Normal, QIcon::On);

	m_pixmaps[0] = new QPixmap(
		icon.pixmap(12, 12, QIcon::Normal, QIcon::Off));
	m_pixmaps[1] = new QPixmap(
		icon.pixmap(12, 12, QIcon::Normal, QIcon::On));

	m_headers
		<< tr("Element")
		<< tr("Sample");

	for (int i = 0; i < MAX_NOTES; ++i)
		m_notes_on[i] = 0;

	reset();
}


// Destructor.
drumkv1widget_elements_model::~drumkv1widget_elements_model (void)
{
	delete m_pixmaps[1];
	delete m_pixmaps[0];
}


int drumkv1widget_elements_model::rowCount (
	const QModelIndex& /*parent*/ ) const
{
	return MAX_NOTES;
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
	case Qt::DecorationRole:
		if (index.column() == 0)
			return *m_pixmaps[m_notes_on[index.row()] > 0 ? 1 : 0];
		break;
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
	return createIndex(row, column, (m_pDrumkUi ? m_pDrumkUi->element(row) : nullptr));
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


drumkv1_ui *drumkv1widget_elements_model::instance (void) const
{
	return m_pDrumkUi;
}


void drumkv1widget_elements_model::midiInLedNote ( int key, int vel )
{
	if (vel > 0) {
		m_notes_on[key] = vel;
		midiInLedUpdate(key);
	}
	else
	if (m_notes_on[key] > 0) {
		QTimer::singleShot(200, this, SLOT(midiInLedTimeout()));
	}
}


void drumkv1widget_elements_model::midiInLedTimeout (void)
{
	for (int key = 0; key < MAX_NOTES; ++key) {
		if (m_notes_on[key] > 0) {
			m_notes_on[key] = 0;
			midiInLedUpdate(key);
		}
	}
}


void drumkv1widget_elements_model::midiInLedUpdate ( int key )
{
	const QModelIndex& index = drumkv1widget_elements_model::index(key, 0);
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
	emit dataChanged(index, index, QVector<int>() << Qt::DecorationRole);
#else
	emit dataChanged(index, index);
#endif
}


void drumkv1widget_elements_model::reset (void)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	QAbstractItemModel::reset();
#else
	QAbstractItemModel::beginResetModel();
	QAbstractItemModel::endResetModel();
#endif
}


QString drumkv1widget_elements_model::itemDisplay (
	const QModelIndex& index ) const
{
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
	return QString('-');
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
	: QTreeView(pParent), m_pModel(nullptr),
		m_pDragSample(nullptr), m_iDirectNoteOn(-1), m_iDirectNoteOnVelocity(64)
{
	resetDragState();
}


// Destructor.
drumkv1widget_elements::~drumkv1widget_elements (void)
{
	if (m_pModel)
		delete m_pModel;
}


// Settlers.
void drumkv1widget_elements::setInstance ( drumkv1_ui *pDrumkUi )
{
	if (m_pModel)
		delete m_pModel;

	m_pModel = new drumkv1widget_elements_model(pDrumkUi);

	QTreeView::setModel(m_pModel);

	QTreeView::setSelectionMode(QAbstractItemView::SingleSelection);
	QTreeView::setRootIsDecorated(false);
	QTreeView::setUniformRowHeights(true);
	QTreeView::setItemsExpandable(false);
	QTreeView::setAllColumnsShowFocus(true);
	QTreeView::setAlternatingRowColors(true);

	QTreeView::setMinimumSize(QSize(360, 80));
	QTreeView::setSizePolicy(
		QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

	QTreeView::setAcceptDrops(true);

	QHeaderView *pHeader = QTreeView::header();
	pHeader->setDefaultAlignment(Qt::AlignLeft);
	pHeader->setStretchLastSection(true);

	// Element selectors
	QObject::connect(QTreeView::selectionModel(),
		SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&)),
		SLOT(currentRowChanged(const QModelIndex&, const QModelIndex&)));
	QObject::connect(this,
		SIGNAL(doubleClicked(const QModelIndex&)),
		SLOT(doubleClicked(const QModelIndex&)));
}


drumkv1_ui *drumkv1widget_elements::instance (void) const
{
	return (m_pModel ? m_pModel->instance() : nullptr);
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


// Mouse interaction.
void drumkv1widget_elements::mousePressEvent ( QMouseEvent *pMouseEvent )
{
	if (pMouseEvent->button() == Qt::LeftButton) {
		const QPoint& pos = pMouseEvent->pos();
		if (pos.x() > 0 && pos.x() < 16) {
			directNoteOn(QTreeView::indexAt(pos).row());
			return; // avoid double-clicks...
		} else {
			m_dragState = DragStart;
			m_posDrag = pos;
		}
	}

	QTreeView::mousePressEvent(pMouseEvent);
}


void drumkv1widget_elements::mouseMoveEvent ( QMouseEvent *pMouseEvent )
{
	QTreeView::mouseMoveEvent(pMouseEvent);

	if (m_dragState == DragStart
		&& (m_posDrag - pMouseEvent->pos()).manhattanLength()
			> QApplication::startDragDistance()) {
		drumkv1_element *element
			= static_cast<drumkv1_element *> (
				QTreeView::currentIndex().internalPointer());
		// Start dragging alright...
		if (element && element->sample()) {
			QList<QUrl> urls;
			m_pDragSample = element->sample();
			urls.append(QUrl::fromLocalFile(m_pDragSample->filename()));
			QMimeData *pMimeData = new QMimeData();
			pMimeData->setUrls(urls);;
			QDrag *pDrag = new QDrag(this);
			pDrag->setMimeData(pMimeData);
			pDrag->exec(Qt::CopyAction);
		}
		resetDragState();
	}
}


void drumkv1widget_elements::mouseReleaseEvent ( QMouseEvent *pMouseEvent )
{
	QTreeView::mouseReleaseEvent(pMouseEvent);

	directNoteOff();

	m_pDragSample = nullptr;
	resetDragState();
}


// Drag-n-drop (more of the later) support.
void drumkv1widget_elements::dragEnterEvent ( QDragEnterEvent *pDragEnterEvent )
{
	QTreeView::dragEnterEvent(pDragEnterEvent);

	if (pDragEnterEvent->mimeData()->hasUrls())
		pDragEnterEvent->acceptProposedAction();
}


void drumkv1widget_elements::dragMoveEvent ( QDragMoveEvent *pDragMoveEvent )
{
	QTreeView::dragMoveEvent(pDragMoveEvent);

	if (pDragMoveEvent->mimeData()->hasUrls()) {
		const QModelIndex& index
		#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
			= QTreeView::indexAt(pDragMoveEvent->position().toPoint());
		#else
			= QTreeView::indexAt(pDragMoveEvent->pos());
		#endif
		if (index.isValid()) {
			setCurrentIndex(index.row());
			if (m_pDragSample) {
				drumkv1_element *element
					= static_cast<drumkv1_element *> (
						index.internalPointer());
				// Start dragging alright...
				if (element && m_pDragSample == element->sample())
					return;
			}
			pDragMoveEvent->acceptProposedAction();
		}
	}
}


void drumkv1widget_elements::dropEvent ( QDropEvent *pDropEvent )
{
	QTreeView::dropEvent(pDropEvent);

	const QMimeData *pMimeData = pDropEvent->mimeData();
	if (pMimeData->hasUrls()) {
		const QString& sFilename
			= QListIterator<QUrl>(pMimeData->urls()).peekNext().toLocalFile();
		if (!sFilename.isEmpty())
			emit itemLoadSampleFile(sFilename, currentIndex());
	}
}


// Reset drag/select state.
void drumkv1widget_elements::resetDragState (void)
{
	m_dragState = DragNone;
}


// Refreshner.
void drumkv1widget_elements::refresh (void)
{
	if (m_pModel == nullptr)
		return;

	QItemSelectionModel *pSelectionModel = QTreeView::selectionModel();
	const QModelIndex& index = pSelectionModel->currentIndex();

	m_pModel->reset();

	QTreeView::header()->resizeSections(QHeaderView::ResizeToContents);

	pSelectionModel->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
}


// Default size hint.
QSize drumkv1widget_elements::sizeHint (void) const
{
	return QSize(360, 80);
}


// MIDI input status update
void drumkv1widget_elements::midiInLedNote ( int key, int vel )
{
	if (m_pModel)
		m_pModel->midiInLedNote(key, vel);
}


// Direct note-on/off methods.
void drumkv1widget_elements::directNoteOn ( int key )
{
	if (m_pModel == nullptr || key < 0)
		return;

	drumkv1_ui *pDrumkUi = m_pModel->instance();
	if (pDrumkUi == nullptr)
		return;

	m_iDirectNoteOn = key;

	pDrumkUi->directNoteOn(m_iDirectNoteOn, m_iDirectNoteOnVelocity);

	drumkv1_sample *pSample = pDrumkUi->sample();
	if (pSample) {
		const float srate_ms = 0.001f * pSample->sampleRate();
		const int timeout_ms = int(float(pSample->length() >> 1) / srate_ms);
		QTimer::singleShot(timeout_ms, this, SLOT(directNoteOff()));
	}
}


void drumkv1widget_elements::directNoteOff (void)
{
	if (m_pModel == nullptr || m_iDirectNoteOn < 0)
		return;

	drumkv1_ui *pDrumkUi = m_pModel->instance();
	if (pDrumkUi == nullptr)
		return;

	pDrumkUi->directNoteOn(m_iDirectNoteOn, 0); // note-off!

	m_iDirectNoteOn = -1;
}


// Direct note-on velocity accessors.
void drumkv1widget_elements::setDirectNoteOnVelocity ( int vel )
{
	m_iDirectNoteOnVelocity = vel;
}


int drumkv1widget_elements::directNoteOnVelocity (void) const
{
	return m_iDirectNoteOnVelocity;
}


// end of drumkv1widget_elements.cpp
