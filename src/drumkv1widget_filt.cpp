// drumkv1widget_filt.cpp
//
/****************************************************************************
   Copyright (C) 2012-2021, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "drumkv1widget_filt.h"

#include <QPainter>

#include <QLinearGradient>

#include <QMouseEvent>
#include <QWheelEvent>

#include <cmath>


// Safe value capping.
inline float safe_value ( float x )
{
	return (x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x));
}


//----------------------------------------------------------------------------
// drumkv1widget_filt -- Custom widget

// Constructor.
drumkv1widget_filt::drumkv1widget_filt ( QWidget *pParent )
	: QFrame(pParent),
		m_fCutoff(0.0f), m_fReso(0.0f), m_iType(LPF), m_iSlope(S12DB),
		m_bDragging(false)
{
//	setMouseTracking(true);
	setMinimumSize(QSize(180, 72));

	QFrame::setFrameShape(QFrame::Panel);
	QFrame::setFrameShadow(QFrame::Sunken);
}


// Destructor.
drumkv1widget_filt::~drumkv1widget_filt (void)
{
}


// Parameter accessors.
void drumkv1widget_filt::setCutoff ( float fCutoff )
{
	if (::fabsf(m_fCutoff - fCutoff) > 0.001f) {
		m_fCutoff = safe_value(fCutoff);
		updatePath();
		emit cutoffChanged(cutoff());
	}
}

float drumkv1widget_filt::cutoff (void) const
{
	return m_fCutoff;
}


void drumkv1widget_filt::setReso ( float fReso )
{
	if (::fabsf(m_fReso - fReso) > 0.001f) {
		m_fReso = safe_value(fReso);
		updatePath();
		emit resoChanged(reso());
	}
}

float drumkv1widget_filt::reso (void) const
{
	return m_fReso;
}


void drumkv1widget_filt::setType ( float fType )
{
	const int iType = int(fType);
	if (m_iType != iType) {
		m_iType  = iType;
		updatePath();
	}
}

float drumkv1widget_filt::type (void) const
{
	return float(m_iType);
}


void drumkv1widget_filt::setSlope ( float fSlope )
{
	const int iSlope = int(fSlope);
	if (m_iSlope != iSlope) {
		m_iSlope  = iSlope;
		updatePath();
	}
}

float drumkv1widget_filt::slope (void) const
{
	return float(m_iSlope);
}


// Draw curve.
void drumkv1widget_filt::paintEvent ( QPaintEvent *pPaintEvent )
{
	QPainter painter(this);

	const QRect& rect = QWidget::rect();
	const int h = rect.height();
	const int w = rect.width();

	const QPalette& pal = palette();
	const bool bDark = (pal.window().color().value() < 0x7f);
	const QColor& rgbLite = (isEnabled() ? Qt::yellow : pal.mid().color());
	const QColor& rgbDark = pal.window().color().darker();

	painter.fillRect(rect, rgbDark);

	QColor rgbLite1(rgbLite);
	QColor rgbDrop1(Qt::black);
	rgbLite1.setAlpha(bDark ? 80 : 120);
	rgbDrop1.setAlpha(80);

	QLinearGradient grad(0, 0, w << 1, h << 1);
	grad.setColorAt(0.0f, rgbLite1);
	grad.setColorAt(1.0f, rgbDrop1);

	painter.setRenderHint(QPainter::Antialiasing, true);

//	painter.setPen(bDark ? Qt::gray : Qt::darkGray);
	painter.setPen(QPen(rgbLite1, 2));
	painter.setBrush(grad);
	painter.drawPath(m_path);

#ifdef CONFIG_DEBUG_0
	painter.drawText(QFrame::rect(),
		Qt::AlignTop|Qt::AlignHCenter,
		tr("Cutoff(%1) Reso(%2)")
		.arg(int(100.0f * cutoff()))
		.arg(int(100.0f * reso())));
#endif

	painter.setRenderHint(QPainter::Antialiasing, false);

	painter.end();

	QFrame::paintEvent(pPaintEvent);
}


// Drag/move curve.
void drumkv1widget_filt::dragCurve ( const QPoint& pos )
{
	const int h  = height();
	const int w  = width();

	const int dx = (pos.x() - m_posDrag.x());
	const int dy = (pos.y() - m_posDrag.y());

	if (dx || dy) {
		const int h2 = (h >> 1);
		const int x = int(cutoff() * float(w));
		const int y = int(reso() * float(h2));
		setCutoff(float(x + dx) / float(w));
		setReso(float(y - dy) / float(h2));
		m_posDrag = pos;
	}
}


// Mouse interaction.
void drumkv1widget_filt::mousePressEvent ( QMouseEvent *pMouseEvent )
{
	if (pMouseEvent->button() == Qt::LeftButton)
		m_posDrag = pMouseEvent->pos();

	QFrame::mousePressEvent(pMouseEvent);
}


void drumkv1widget_filt::mouseMoveEvent ( QMouseEvent *pMouseEvent )
{
	const QPoint& pos = pMouseEvent->pos();
	if (m_bDragging) {
		dragCurve(pos);
	} else { // if ((pos - m_posDrag).manhattanLength() > 4) {
		setCursor(Qt::SizeAllCursor);
		m_bDragging = true;
	}
}


void drumkv1widget_filt::mouseReleaseEvent ( QMouseEvent *pMouseEvent )
{
	QFrame::mouseReleaseEvent(pMouseEvent);

	if (m_bDragging) {
		dragCurve(pMouseEvent->pos());
		m_bDragging = false;
		unsetCursor();
	}
}


void drumkv1widget_filt::wheelEvent ( QWheelEvent *pWheelEvent )
{
	const int delta = (pWheelEvent->angleDelta().y() / 60);

	if (pWheelEvent->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier)) {
		const int h2 = (height() >> 1);
		const int y = int(reso() * float(h2));
		setReso(float(y + delta) / float(h2));
	} else {
		const int w2 = (width() >> 1);
		const int x = int(cutoff() * float(w2));
		setCutoff(float(x + delta) / float(w2));
	}
}


// Resize canvas.
void drumkv1widget_filt::resizeEvent ( QResizeEvent *pResizeEvent )
{
	QFrame::resizeEvent(pResizeEvent);

	updatePath();
}


// Update the drawing polygon.
void drumkv1widget_filt::updatePath (void)
{
	const QRect& rect = QWidget::rect();
	const int h  = rect.height();
	const int w  = rect.width();

	const int h2 = h >> 1;
	const int h4 = h >> 2;
	const int w4 = w >> 2;
	const int w8 = w >> 3;

	const int ws = w8 - (m_iSlope == S24DB ? (w8 >> 1) : 0);

	int x = w8 + int(m_fCutoff * float(w - w4));
	int y = h2 - int(m_fReso   * float(h + h4));

	QPolygon poly(6);
	QPainterPath path;

	const int iType = (m_iSlope == SFORMANT ? L2F : m_iType);
	// Low, Notch
	if (iType == LPF || iType == BRF) {
		if (iType == BRF) x -= w8;
		poly.putPoints(0, 6,
			0, h2,
			x - w8, h2,
			x, h2,
			x, y,
			x + ws, h,
			0, h);
		path.moveTo(poly.at(0));
		path.lineTo(poly.at(1));
		path.cubicTo(poly.at(2), poly.at(3), poly.at(4));
		path.lineTo(poly.at(5));
		if (iType == BRF) x += w8;
	}
	// Band
	if (iType == BPF) {
		const int y2 = (y + h4) >> 1;
		poly.putPoints(0, 6,
			0, h,
			x - w8 - ws, h,
			x - ws, y2,
			x + ws, y2,
			x + w8 + ws, h,
			0, h);
		path.moveTo(poly.at(0));
		path.lineTo(poly.at(1));
		path.cubicTo(poly.at(2), poly.at(3), poly.at(4));
		path.lineTo(poly.at(5));
	}
	// High, Notch
	if (iType == HPF || iType == BRF) {
		if (iType == BRF) { x += w8; y = h2; }
		poly.putPoints(0, 6,
			x - ws, h,
			x, y,
			x, h2,
			x + w8, h2,
			w, h2,
			w, h);
		path.moveTo(poly.at(0));
		path.cubicTo(poly.at(1), poly.at(2), poly.at(3));
		path.lineTo(poly.at(4));
		path.lineTo(poly.at(5));
		if (iType == BRF) x -= w8;
	}
	// Formant
	if (iType == L2F) {
		const int x2 = (x - w4) >> 2;
		const int y2 = (y - h4) >> 2;
		poly.putPoints(0, 6,
			0, h2,
			x2, h2,
			x - ws, h2,
			x, y2,
			x + ws, h,
			0, h);
		path.moveTo(poly.at(0));
	#if 0
		path.lineTo(poly.at(1));
		path.cubicTo(poly.at(2), poly.at(3), poly.at(4));
	#else
		const int n3 = 5; // num.formants
		const int w3 = (x + ws - x2) / n3 - 1;
		const int w6 = (w3 >> 1);
		const int h3 = (h4 >> 1);
		int x3 = x2; int y3 = y2;
		for (int i = 0; i < n3; ++i) {
			poly.putPoints(1, 3,
				x3, h2,
				x3 + w6, y3,
				x3 + w3, y3 + h2);
			path.cubicTo(poly.at(1), poly.at(2), poly.at(3));
			x3 += w3; y3 += h3;
		}
		path.lineTo(poly.at(4));
	#endif
		path.lineTo(poly.at(5));
	}

	// Commit path.
	m_path = path;

	QFrame::update();
}


// end of drumkv1widget_filt.cpp
