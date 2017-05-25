// drumkv1widget_elements.h
//
/****************************************************************************
   Copyright (C) 2012-2017, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include <QTreeView>
#include <QAbstractItemModel>


// Forwards.
class drumkv1_ui;

class drumkv1_element;
class drumkv1_sample;

class QPixmap;

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;


//----------------------------------------------------------------------------
// drumkv1widget_elements_model -- List model.

class drumkv1widget_elements_model : public QAbstractItemModel
{
	Q_OBJECT

public:

	// Constructor.
	drumkv1widget_elements_model(drumkv1_ui *pDrumkUi, QObject *pParent = NULL);

	// Destructor.
	~drumkv1widget_elements_model();

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
	drumkv1_ui *instance() const;

	void midiInLedNote(int key, int vel);

protected slots:

	void midiInLedTimeout();

protected:

	// Other specifics
	drumkv1_element *elementFromIndex(const QModelIndex& index) const;

	QString itemDisplay(const QModelIndex& index) const;
	QString itemToolTip(const QModelIndex& index) const;

	int columnAlignment(int column) const;

	void midiInLedUpdate(int key);

private:

	// Model variables.
	QPixmap    *m_pixmaps[2];
	QStringList m_headers;

	drumkv1_ui *m_pDrumkUi;

	static const int MAX_NOTES = 128;
	int m_notes_on[MAX_NOTES];
	QList<int> m_notes_off;
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
	void setInstance(drumkv1_ui *pDrumkUi);
	drumkv1_ui *instance() const;

	// Current element accessors.
	void setCurrentIndex(int row);
	int currentIndex() const;

	// Refreshener.
	void refresh();

	// MIDI input status update
	void midiInLedNote(int key, int vel);

	// Direct note-on methods.
	void directNoteOn(int key);

signals:

	// Emitted signals.
	void itemActivated(int);
	void itemDoubleClicked(int);

	// Load new sample file on current item.
	void itemLoadSampleFile(const QString&, int);

protected slots:

	// Internal slot handlers.
	void currentRowChanged(const QModelIndex&, const QModelIndex&);
	void doubleClicked(const QModelIndex&);

	// Direct note-off/timeout methods.
	void directNoteOff();

protected:

	// Mouse interaction.
	void mousePressEvent(QMouseEvent *pMouseEvent);
	void mouseMoveEvent(QMouseEvent *pMouseEvent);
	void mouseReleaseEvent(QMouseEvent *pMouseEvent);

	// Drag-n-drop (more of the later) support.
	void dragEnterEvent(QDragEnterEvent *pDragEnterEvent);
	void dragMoveEvent(QDragMoveEvent *pDragMoveEvent);
	void dropEvent(QDropEvent *pDropEvent);

	// Reset drag/select state.
	void resetDragState();

	// Default size hint.
	QSize sizeHint() const;

private:

	// Instance variables.
	drumkv1widget_elements_model *m_pModel;

	// Drag state.
	enum DragState { DragNone = 0, DragStart } m_dragState;

	QPoint m_posDrag;

	drumkv1_sample *m_pDragSample;

	int m_iDirectNoteOn;
};


#endif  // __drumkv1widget_elements_h

// end of drumkv1widget_elements.h
