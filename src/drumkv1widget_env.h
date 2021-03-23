// drumkv1widget_env.h
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

#ifndef __drumkv1widget_env_h
#define __drumkv1widget_env_h

#include <QFrame>
#include <QPolygon>


//----------------------------------------------------------------------------
// drumkv1widget_env -- Custom widget

class drumkv1widget_env : public QFrame
{
	Q_OBJECT

public:

	// Constructor.
	drumkv1widget_env(QWidget *pParent = nullptr);
	// Destructor.
	~drumkv1widget_env();

	// Parameter getters.
	float attack() const;
	float decay1() const;
	float level2() const;
	float decay2() const;

public slots:

	// Parameter setters.
	void setAttack(float fAttack);
	void setDecay1(float fDecay1);
	void setLevel2(float fLevel2);
	void setDecay2(float fDecay2);

signals:

	// Parameter change signals.
	void attackChanged(float fAttack);
	void decay1Changed(float fDecay1);
	void level2Changed(float fLevel2);
	void decay2Changed(float fDecay2);

protected:

	// Draw canvas.
	void paintEvent(QPaintEvent *);

	// Parameter node indexes.
	enum NodeIndex {
		Idle   = 1,
		Attack = 2,
		Decay1 = 3,
		Decay2 = 4,
		End    = 5
	};

	// Draw rectangular point.
	QRect nodeRect(int iNode) const;
	int nodeIndex(const QPoint& pos) const;

	void dragNode(const QPoint& pos);

	// Mouse interaction.
	void mousePressEvent(QMouseEvent *pMouseEvent);
	void mouseMoveEvent(QMouseEvent *pMouseEvent);
	void mouseReleaseEvent(QMouseEvent *pMouseEvent);

	// Resize canvas.
	void resizeEvent(QResizeEvent *);

	// Update the drawing polygon.
	void updatePolygon();

private:

	// Instance state.
	float m_fAttack;
	float m_fDecay1;
	float m_fLevel2;
	float m_fDecay2;

	// Draw state.
	QPolygon m_poly;

	// Drag state.
	int    m_iDragNode;
	QPoint m_posDrag;
};

#endif	// __drumkv1widget_env_h


// end of drumkv1widget_env.h
