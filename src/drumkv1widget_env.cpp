// drumkv1widget_env.cpp
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

#include "drumkv1widget_env.h"

#include <QPainter>
#include <QPainterPath>

#include <QLinearGradient>

#include <QMouseEvent>

#include <cmath>


// Safe value capping.
inline float safe_value ( float x )
{
	return (x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x));
}


//----------------------------------------------------------------------------
// drumkv1widget_env -- Custom widget

// Constructor.
drumkv1widget_env::drumkv1widget_env ( QWidget *pParent )
	: QFrame(pParent),
		m_fAttack(0.0f), m_fDecay1(0.0f), m_fLevel2(0.0f), m_fDecay2(0.0f),
		m_poly(6), m_iDragNode(-1)
{
	setMouseTracking(true);
	setMinimumSize(QSize(120, 72));

	QFrame::setFrameShape(QFrame::Panel);
	QFrame::setFrameShadow(QFrame::Sunken);
}


// Destructor.
drumkv1widget_env::~drumkv1widget_env (void)
{
}


// Parameter accessors.
void drumkv1widget_env::setAttack ( float fAttack )
{
	if (::fabsf(m_fAttack - fAttack) > 0.001f) {
		m_fAttack = safe_value(fAttack);
		updatePolygon();
		emit attackChanged(attack());
	}
}

float drumkv1widget_env::attack (void) const
{
	return m_fAttack;
}


void drumkv1widget_env::setDecay1 ( float fDecay1 )
{
	if (::fabsf(m_fDecay1 - fDecay1) > 0.001f) {
		m_fDecay1 = safe_value(fDecay1);
		updatePolygon();
		emit decay1Changed(decay1());
	}
}

float drumkv1widget_env::decay1 (void) const
{
	return m_fDecay1;
}


void drumkv1widget_env::setLevel2 ( float fLevel2 )
{
	if (::fabsf(m_fLevel2 - fLevel2) > 0.001f) {
		m_fLevel2 = safe_value(fLevel2);
		updatePolygon();
		emit level2Changed(level2());
	}
}

float drumkv1widget_env::level2 (void) const
{
	return m_fLevel2;
}


void drumkv1widget_env::setDecay2 ( float fDecay2 )
{
	if (::fabsf(m_fDecay2 - fDecay2) > 0.001f) {
		m_fDecay2 = safe_value(fDecay2);
		updatePolygon();
		emit decay2Changed(decay2());
	}
}

float drumkv1widget_env::decay2 (void) const
{
	return m_fDecay2;
}


// Draw curve.
void drumkv1widget_env::paintEvent ( QPaintEvent *pPaintEvent )
{
	QPainter painter(this);

	const QRect& rect = QFrame::rect();
	const int h  = rect.height();
	const int w  = rect.width();

	QPainterPath path;
//	path.addPolygon(m_poly);
	QPoint p1, p2, p3;
	path.moveTo(m_poly.at(0));
	p1 = m_poly.at(Idle);
	path.lineTo(p1);
	p2 = p1;
	p2.setY(h >> 1);
	p3 = m_poly.at(Attack);
	path.cubicTo(p1, p2, p3);
	p1 = p2 = p3;
	p3 = m_poly.at(Decay1);
	p2.setY((p3.y() >> 1) + 1);
	path.cubicTo(p1, p2, p3);
//	path.lineTo(m_poly.at(Level2));
	p1 = p2 = p3;
	p2.setY(p1.y() + ((h - p1.y()) >> 1) - 1);
	p3 = m_poly.at(Decay2);
	path.cubicTo(p1, p2, p3);
	path.lineTo(m_poly.at(End));
	path.lineTo(m_poly.at(0));

	const QPalette& pal = palette();
	const bool bDark = (pal.window().color().value() < 0x7f);
	const QColor& rgbLite = (isEnabled() ? Qt::yellow : pal.mid().color());
	const QColor& rgbDark = pal.window().color().darker();

	painter.fillRect(rect, rgbDark);

	QColor rgbLite1(rgbLite);
	QColor rgbDrop1(Qt::black);
	rgbLite1.setAlpha(bDark ? 80 : 180);
	rgbDrop1.setAlpha(80);

	QLinearGradient grad(0, 0, w << 1, h << 1);
	grad.setColorAt(0.0f, rgbLite1);
	grad.setColorAt(1.0f, rgbDrop1);

	painter.setRenderHint(QPainter::Antialiasing, true);

//	painter.setPen(bDark ? Qt::gray : Qt::darkGray);
	painter.setPen(QPen(rgbLite1, 2));
	painter.setBrush(grad);
	painter.drawPath(path);

	painter.setPen(rgbDrop1);
	painter.setBrush(rgbDrop1.lighter());
	painter.drawRect(nodeRect(Idle));
	painter.setPen(rgbLite1.lighter());
	painter.setBrush(rgbLite1);
	painter.drawRect(nodeRect(Attack));
	painter.drawRect(nodeRect(Decay1));
	painter.drawRect(nodeRect(Decay2));

#ifdef CONFIG_DEBUG_0
	painter.drawText(QFrame::rect(),
		Qt::AlignTop|Qt::AlignHCenter,
		tr("A(%1) D1(%2) L2(%3) D2(%4)")
		.arg(int(100.0f * attack()))
		.arg(int(100.0f * decay1()))
		.arg(int(100.0f * level2()))
		.arg(int(100.0f * decay2())));
#endif

	painter.setRenderHint(QPainter::Antialiasing, false);

	painter.end();

	QFrame::paintEvent(pPaintEvent);
}


// Draw rectangular point.
QRect drumkv1widget_env::nodeRect ( int iNode ) const
{
	const QPoint& pos = m_poly.at(iNode);
	return QRect(pos.x() - 4, pos.y() - 4, 8, 8); 
}


int drumkv1widget_env::nodeIndex ( const QPoint& pos ) const
{
	if (nodeRect(Decay2).contains(pos))
		return Decay2;

	if (nodeRect(Decay1).contains(pos))
		return Decay1;

	if (nodeRect(Attack).contains(pos))
		return Attack; // Attack

	return -1;
}


void drumkv1widget_env::dragNode ( const QPoint& pos )
{
	const int h  = height();
	const int w  = width();

	const int w3 = (w - 12) / 3;

	const int dx = (pos.x() - m_posDrag.x());
	const int dy = (pos.y() - m_posDrag.y());

	if (dx || dy) {
		int x, y;
		switch (m_iDragNode) {
		case Attack:
			x = int(attack() * float(w3));
			setAttack(float(x + dx) / float(w3));
			break;
		case Decay1: // Level2
			x = int(decay1() * float(w3));
			setDecay1(float(x + dx) / float(w3));
			y = int(level2() * float(h - 12));
			setLevel2(float(y - dy) / float(h - 12));
			break;
		case Decay2:
			x = int(decay2() * float(w3));
			setDecay2(float(x + dx) / float(w3));
			break;
		}
		m_posDrag = m_poly.at(m_iDragNode);
	//	m_posDrag = pos;
	}
}


// Mouse interaction.
void drumkv1widget_env::mousePressEvent ( QMouseEvent *pMouseEvent )
{
	if (pMouseEvent->button() == Qt::LeftButton) {
		const QPoint& pos = pMouseEvent->pos();
		const int iDragNode = nodeIndex(pos);
		if (iDragNode >= 0) {
			switch (iDragNode) {
			case Attack:
			case Decay2:
				setCursor(Qt::SizeHorCursor);
				break;
			case Decay1: // Level2
				setCursor(Qt::SizeAllCursor);
				break;
			default:
				break;
			}
			m_iDragNode = iDragNode;
			m_posDrag = pos;
		}
	}

	QFrame::mousePressEvent(pMouseEvent);
}


void drumkv1widget_env::mouseMoveEvent ( QMouseEvent *pMouseEvent )
{
	const QPoint& pos = pMouseEvent->pos();
	if (m_iDragNode > Idle)
		dragNode(pos);
	else if (nodeIndex(pos) > Idle)
		setCursor(Qt::PointingHandCursor);
	else
		unsetCursor();
}


void drumkv1widget_env::mouseReleaseEvent ( QMouseEvent *pMouseEvent )
{
	QFrame::mouseReleaseEvent(pMouseEvent);

	if (m_iDragNode > Idle) {
		dragNode(pMouseEvent->pos());
		m_iDragNode = -1;
		unsetCursor();
	}
}


// Resize canvas.
void drumkv1widget_env::resizeEvent ( QResizeEvent *pResizeEvent )
{
	QFrame::resizeEvent(pResizeEvent);

	updatePolygon();
}


// Update the drawing polygon.
void drumkv1widget_env::updatePolygon (void)
{
	const QRect& rect = QFrame::rect();
	const int h  = rect.height();
	const int w  = rect.width();

	const int w3 = (w - 10) / 3;

	const int x1 = int(m_fAttack * float(w3)) + 5;
	const int x2 = int(m_fDecay1 * float(w3)) + x1;
	const int x3 = int(m_fDecay2 * float(w3)) + x2;

	const int y2 = h - int(m_fLevel2 * float(h - 10)) - 5;

	m_poly.putPoints(0, 6,
		5,  h,
		5,  h - 5, // Idle
		x1, 5,     // Attack
		x2, y2,    // Decay1/Level2
		x3, h - 5, // Decay2
		x3, h);

	QFrame::update();
}


// end of drumkv1widget_env.cpp
