// drumkv1widget_sample.h
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

#ifndef __drumkv1widget_sample_h
#define __drumkv1widget_sample_h

#include <QFrame>

#include <stdint.h>


// Forward decl.
class drumkv1_sample;

class QDragEnterEvent;
class QDropEvent;


//----------------------------------------------------------------------------
// drumkv1widget_sample -- Custom widget

class drumkv1widget_sample : public QFrame
{
	Q_OBJECT

public:

	// Constructor.
	drumkv1widget_sample(QWidget *pParent = nullptr);
	// Destructor.
	~drumkv1widget_sample();

	// Parameter accessors.
	void setSample(drumkv1_sample *pSample);
	drumkv1_sample *sample() const;

	void setSampleName(const QString& sName);
	const QString& sampleName() const;

	// Offset mode.
	void setOffset(bool bOffset);
	bool isOffset() const;

	// Offset getters.
	uint32_t offsetStart() const;
	uint32_t offsetEnd() const;

	// Value/text format converter utilities.
	uint32_t valueFromText (const QString& text) const;
	QString textFromValue (uint32_t value) const;

signals:

	// Load new sample file.
	void loadSampleFile(const QString&);

	// Offset changed.
	void offsetRangeChanged();

public slots:

	// Browse for a new sample.
	void openSample(const QString& sName);

	// Effective sample slot.
	void loadSample(drumkv1_sample *pSample);

	// Offset point setters.
	void setOffsetStart(uint32_t iOffsetStart);
	void setOffsetEnd(uint32_t iOffsetEnd);

protected:

	// Sanitizer helper.
	int safeX(int x) const;

	// Sanitized converters.
	int pixelFromFrames(uint32_t n) const;
	uint32_t framesFromPixel(int x) const;

	// Widget resize handler.
	void resizeEvent(QResizeEvent *);

	// Draw canvas.
	void paintEvent(QPaintEvent *);

	// Mouse interaction.
	void mousePressEvent(QMouseEvent *pMouseEvent);
	void mouseMoveEvent(QMouseEvent *pMouseEvent);
	void mouseReleaseEvent(QMouseEvent *pMouseEvent);

	void mouseDoubleClickEvent(QMouseEvent *pMouseEvent);

	// Trap for escape key.
	void keyPressEvent(QKeyEvent *pKeyEvent);

	// Drag-n-drop (more of the later) support.
	void dragEnterEvent(QDragEnterEvent *pDragEnterEvent);
	void dropEvent(QDropEvent *pDropEvent);

	// Reset drag/select state.
	void resetDragState();

	// Update tool-tip.
	void updateToolTip();

	// Default size hint.
	QSize sizeHint() const;

private:

	// Instance state.
	drumkv1_sample *m_pSample;
	unsigned short m_iChannels;
	QPolygon **m_ppPolyg;

	QString m_sName;

	// Drag state.
	enum DragState {
		DragNone = 0, DragStart,
		DragOffsetRange, DragOffsetStart, DragOffsetEnd
	} m_dragState, m_dragCursor;

	QPoint m_posDrag;

	int m_iDragOffsetStartX;
	int m_iDragOffsetEndX;

	drumkv1_sample *m_pDragSample;

	// Offset state.
	bool     m_bOffset;
	uint32_t m_iOffsetStart;
	uint32_t m_iOffsetEnd;
};

#endif	// __drumkv1widget_sample_h


// end of drumkv1widget_sample.h
