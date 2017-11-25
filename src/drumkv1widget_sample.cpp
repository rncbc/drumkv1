// drumkv1widget_sample.cpp
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

#include "drumkv1widget_sample.h"

#include "drumkv1_config.h"
#include "drumkv1_sample.h"

#include <sndfile.h>

#include <QPainter>

#include <QApplication>
#include <QFileDialog>
#include <QMimeData>
#include <QDrag>
#include <QUrl>

#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>

#include <QToolTip>


//----------------------------------------------------------------------------
// drumkv1widget_sample -- Custom widget

// Constructor.
drumkv1widget_sample::drumkv1widget_sample (
	QWidget *pParent, Qt::WindowFlags wflags )
	: QFrame(pParent, wflags), m_pSample(0), m_iChannels(0), m_ppPolyg(0)
{
	QFrame::setMouseTracking(true);
	QFrame::setFocusPolicy(Qt::ClickFocus);

	QFrame::setMinimumSize(QSize(380, 80));
	QFrame::setSizePolicy(
		QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

	QFrame::setAcceptDrops(true);

	QFrame::setFrameShape(QFrame::Panel);
	QFrame::setFrameShadow(QFrame::Sunken);

	m_bLoop = false;
	m_iLoopStart = m_iLoopEnd = 0;

	m_dragCursor  = DragNone;
	m_pDragSample = NULL;

	resetDragState();
}


// Destructor.
drumkv1widget_sample::~drumkv1widget_sample (void)
{
	setSample(NULL);
}


// Parameter accessors.
void drumkv1widget_sample::setSample ( drumkv1_sample *pSample )
{
	if (m_pSample && m_ppPolyg) {
		for (unsigned short k = 0; k < m_iChannels; ++k)
			delete m_ppPolyg[k];
		delete [] m_ppPolyg;
		m_ppPolyg = 0;
		m_iChannels = 0;
	}

	m_pSample = pSample;

	m_pDragSample = NULL;

	if (m_pSample)
		m_iChannels = m_pSample->channels();
	if (m_iChannels > 0 && m_ppPolyg == 0) {
		const int h = height();
		const int w = width() & 0x7ffe; // force even.
		const int w2 = (w >> 1);
		const unsigned int nframes = m_pSample->length();
		const float period = float(nframes) / float(w2);
		const int h0 = h / m_iChannels;
		const float h1 = float(h0 >> 1);
		int y0 = h1;
		m_ppPolyg = new QPolygon* [m_iChannels];
		for (unsigned short k = 0; k < m_iChannels; ++k) {
			m_ppPolyg[k] = new QPolygon(w);
			const float *pframes = m_pSample->frames(k);
			float vmax = 0.0f;
			float vmin = 0.0f;
			int n = 0;
			int x = 1;
			unsigned int j = 0;
			for (unsigned int i = 0; i < nframes; ++i) {
				const float v = *pframes++;
				if (vmax < v)
					vmax = v;
				if (vmin > v)
					vmin = v;
				if (i > j) {
					m_ppPolyg[k]->setPoint(n, x, y0 - int(vmax * h1));
					m_ppPolyg[k]->setPoint(w - n - 1, x, y0 - int(vmin * h1));
					vmax = vmin = 0.0f;
					++n; x += 2; j = (unsigned int) (float(n) * period);
				}
			}
			while (n < w2) {
				m_ppPolyg[k]->setPoint(n, x, y0);
				m_ppPolyg[k]->setPoint(w - n - 1, x, y0);
				++n; x += 2;
			}
			y0 += h0;
		}
	}

	updateToolTip();
	update();
}

drumkv1_sample *drumkv1widget_sample::sample (void) const
{
	return m_pSample;
}


void drumkv1widget_sample::setSampleName ( const QString& sName )
{
	m_sName = sName;

	updateToolTip();
}

const QString& drumkv1widget_sample::sampleName (void) const
{
	return m_sName;
}


void drumkv1widget_sample::setLoop ( bool bLoop )
{
	m_bLoop = bLoop;

	updateToolTip();
	update();
}

bool drumkv1widget_sample::isLoop (void) const
{
	return m_bLoop;
}


void drumkv1widget_sample::setLoopStart ( uint32_t iLoopStart )
{
	m_iLoopStart = iLoopStart;

	updateToolTip();
	update();
}

uint32_t drumkv1widget_sample::loopStart (void) const
{
	return m_iLoopStart;
}


void drumkv1widget_sample::setLoopEnd ( uint32_t iLoopEnd )
{
	m_iLoopEnd = iLoopEnd;

	updateToolTip();
	update();
}

uint32_t drumkv1widget_sample::loopEnd (void) const
{
	return m_iLoopEnd;
}


// Sanitizer helper;
int drumkv1widget_sample::safeX ( int x ) const
{
	if (x < 0)
		return 0;

	const int w = QFrame::width();
	if (x > w)
		return w;
	else
		return x;
}


// Widget resize handler.
void drumkv1widget_sample::resizeEvent ( QResizeEvent * )
{
	setSample(m_pSample);	// reset polygon...
}


// Mouse interaction.
void drumkv1widget_sample::mousePressEvent ( QMouseEvent *pMouseEvent )
{
	if (pMouseEvent->button() == Qt::LeftButton) {
		if (m_dragCursor == DragNone) {
			m_dragState = DragStart;
			m_posDrag = pMouseEvent->pos();
		} else if (m_bLoop) {
			const int w = QFrame::width();
			const uint32_t nframes = m_pSample->length();
			if (nframes > 0) {
				m_iDragStartX = safeX((m_iLoopStart * w) / nframes);
				m_iDragEndX   = safeX((m_iLoopEnd   * w) / nframes);
				m_dragState   = m_dragCursor;
			}
		}
	}

	QFrame::mousePressEvent(pMouseEvent);
}


void drumkv1widget_sample::mouseMoveEvent ( QMouseEvent *pMouseEvent )
{
	const int x = pMouseEvent->pos().x();

	switch (m_dragState) {
	case DragNone: {
		if (m_bLoop && m_pSample) {
			const int w = QFrame::width();
			const uint32_t nframes = m_pSample->length();
			if (nframes > 0) {
				const int x1 = (m_iLoopStart * w) / nframes;
				const int x2 = (m_iLoopEnd   * w) / nframes;
				if (abs(x1 - x) < QApplication::startDragDistance()) {
					m_dragCursor = DragLoopStart;
					QFrame::setCursor(QCursor(Qt::SizeHorCursor));
				}
				else
				if (abs(x2 - x) < QApplication::startDragDistance()) {
					m_dragCursor = DragLoopEnd;
					QFrame::setCursor(QCursor(Qt::SizeHorCursor));
				}
				else
				if (m_dragCursor != DragNone) {
					m_dragCursor =  DragNone;
					QFrame::unsetCursor();
				}
			}
		}
		break;
	}
	case DragLoopStart: {
		if (m_iDragEndX > x) {
			m_iDragStartX = safeX(x);
			update();
			if (m_pSample) {
				const int w = QFrame::width();
				if (w > 0) {
					const uint32_t nframes = m_pSample->length();
					QToolTip::showText(
						QCursor::pos(),
						tr("Loop start: %1")
							.arg((m_iDragStartX * nframes) / w), this);
				}
			}
		}
		break;
	}
	case DragLoopEnd: {
		if (m_iDragStartX < x) {
			m_iDragEndX = safeX(x);
			update();
			if (m_pSample) {
				const int w = QFrame::width();
				if (w > 0) {
					const uint32_t nframes = m_pSample->length();
					QToolTip::showText(
						QCursor::pos(),
						tr("Loop end: %1")
							.arg((m_iDragEndX * nframes) / w), this);
				}
			}
		}
		break;
	}
	case DragSelect: {
		// Rubber-band selection...
		const QRect& rect = QRect(m_posDrag, pMouseEvent->pos()).normalized();
		m_iDragStartX = safeX(rect.left());
		m_iDragEndX   = safeX(rect.right());
		update();
		if (m_pSample) {
			const int w = QFrame::width();
			if (w > 0) {
				const uint32_t nframes = m_pSample->length();
				QToolTip::showText(
					QCursor::pos(),
					tr("Loop start: %1, end: %2")
						.arg((m_iDragStartX * nframes) / w)
						.arg((m_iDragEndX   * nframes) / w), this);
			}
		}
		break;
	}
	case DragStart:
		// Rubber-band starting...
		if ((m_posDrag - pMouseEvent->pos()).manhattanLength()
			> QApplication::startDragDistance()) {
			// Start dragging alright...
			if (m_bLoop && (pMouseEvent->modifiers()
				& (Qt::ShiftModifier | Qt::ControlModifier))) {
				m_dragState = m_dragCursor = DragSelect;
				m_iDragStartX = m_iDragEndX = m_posDrag.x();
				QFrame::setCursor(QCursor(Qt::SizeHorCursor));
			} else if (m_pSample && m_pSample->filename()) {
				QList<QUrl> urls;
				m_pDragSample = m_pSample;
				urls.append(QUrl::fromLocalFile(m_pDragSample->filename()));
				QMimeData *pMimeData = new QMimeData();
				pMimeData->setUrls(urls);;
				QDrag *pDrag = new QDrag(this);
				pDrag->setMimeData(pMimeData);
				pDrag->exec(Qt::CopyAction);
				resetDragState();
			}
		}
		// Fall thru...
	default:
		break;
	}

	QFrame::mouseMoveEvent(pMouseEvent);
}


void drumkv1widget_sample::mouseReleaseEvent ( QMouseEvent *pMouseEvent )
{
	QFrame::mouseReleaseEvent(pMouseEvent);

	switch (m_dragState) {
	case DragSelect: {
		const int w = QFrame::width();
		if (m_pSample && w > 0) {
			const uint32_t nframes = m_pSample->length();
			m_iLoopStart = (m_iDragStartX * nframes) / w;
			m_iLoopEnd   = (m_iDragEndX   * nframes) / w;
			emit loopRangeChanged();
			updateToolTip();
			update();
		}
		break;
	}
	case DragLoopStart: {
		const int w = QFrame::width();
		if (m_pSample && w > 0) {
			const uint32_t nframes = m_pSample->length();
			m_iLoopStart = (m_iDragStartX * nframes) / w;
			emit loopRangeChanged();
			updateToolTip();
			update();
		}
		break;
	}
	case DragLoopEnd: {
		const int w = QFrame::width();
		if (m_pSample && w > 0) {
			const uint32_t nframes = m_pSample->length();
			m_iLoopEnd = (m_iDragEndX * nframes) / w;
			emit loopRangeChanged();
			updateToolTip();
			update();
		}
		break;
	}
	default:
		break;
	}

	m_pDragSample = NULL;
	resetDragState();
}


void drumkv1widget_sample::mouseDoubleClickEvent ( QMouseEvent */*pMouseEvent*/ )
{
	openSample(m_sName);
}


// Trap for escape key.
void drumkv1widget_sample::keyPressEvent ( QKeyEvent *pKeyEvent )
{
	switch (pKeyEvent->key()) {
	case Qt::Key_Escape:
		m_pDragSample = NULL;
		resetDragState();
		update();
		break;
	default:
		QFrame::keyPressEvent(pKeyEvent);
		break;
	}
}


// Drag-n-drop (more of the later) support.
void drumkv1widget_sample::dragEnterEvent ( QDragEnterEvent *pDragEnterEvent )
{
	QFrame::dragEnterEvent(pDragEnterEvent);

	if (m_pDragSample && m_pDragSample == sample())
		return;

	if (pDragEnterEvent->mimeData()->hasUrls())
		pDragEnterEvent->acceptProposedAction();
}


void drumkv1widget_sample::dropEvent ( QDropEvent *pDropEvent )
{
	QFrame::dropEvent(pDropEvent);

	const QMimeData *pMimeData = pDropEvent->mimeData();
	if (pMimeData->hasUrls()) {
		const QString& sFilename
			= QListIterator<QUrl>(pMimeData->urls()).peekNext().toLocalFile();
		if (!sFilename.isEmpty())
			emit loadSampleFile(sFilename);
	}
}


// Reset drag/select state.
void drumkv1widget_sample::resetDragState (void)
{
	if (m_dragCursor != DragNone)
		QFrame::unsetCursor();

	m_iDragStartX = m_iDragEndX = 0;

	m_dragState = m_dragCursor = DragNone;
}


// Draw curve.
void drumkv1widget_sample::paintEvent ( QPaintEvent *pPaintEvent )
{
	QPainter painter(this);

	const QRect& rect = QFrame::rect();
	const int h = rect.height();
	const int w = rect.width();

	const QPalette& pal = palette();
	const bool bDark = (pal.window().color().value() < 0x7f);
	const QColor& rgbLite = (isEnabled()
		? (bDark ? Qt::darkYellow : Qt::yellow) : pal.mid().color());
    const QColor& rgbDark = pal.window().color().darker(180);

    painter.fillRect(rect, rgbDark);

	if (m_pSample && m_ppPolyg) {
		const int w2 = (w << 1);
		painter.setRenderHint(QPainter::Antialiasing, true);
		if (m_bLoop && isEnabled()) {
			int x1, x2;
			if (m_iDragStartX < m_iDragEndX) {
				x1 = m_iDragStartX;
				x2 = m_iDragEndX;
			} else {
				const uint32_t nframes = m_pSample->length();
				if (nframes > 0) {
					x1 = (m_iLoopStart * w) / nframes;
					x2 = (m_iLoopEnd   * w) / nframes;
				}
				else x1 = x2 = 0;
			}
			QLinearGradient grad1(0, 0, w2, h);
			painter.setPen(pal.highlight().color());
			grad1.setColorAt(0.0f, rgbLite.darker());
			grad1.setColorAt(0.5f, pal.dark().color());
			painter.fillRect(x1, 0, x2 - x1, h, grad1);
			painter.drawLine(x1, 0, x1, h);
			painter.drawLine(x2, 0, x2, h);
		}
		QLinearGradient grad(0, 0, w2, h);
		painter.setPen(bDark ? Qt::gray : Qt::darkGray);
		grad.setColorAt(0.0f, rgbLite);
		grad.setColorAt(1.0f, Qt::black);
		painter.setBrush(grad);
		for (unsigned short k = 0; k < m_iChannels; ++k)
			painter.drawPolygon(*m_ppPolyg[k]);
		painter.setRenderHint(QPainter::Antialiasing, false);
	} else {
		painter.setPen(pal.midlight().color());
		painter.drawText(rect, Qt::AlignCenter,
			tr("(double-click or drop to load new sample...)"));
	}

	QString sTitle = m_sName;
	if (m_pSample && m_pSample->filename()) {
		if (!sTitle.isEmpty()) {
			sTitle += ' ';
			sTitle += '-';
			sTitle += ' ';
		}
		sTitle += QFileInfo(
			QString::fromUtf8(m_pSample->filename())
		).completeBaseName();
	}
	if (!sTitle.isEmpty()) {
		painter.setPen(pal.midlight().color());
		painter.drawText(rect.adjusted(2, 0, -2, -0), Qt::AlignLeft, sTitle);
	}

	painter.end();

	QFrame::paintEvent(pPaintEvent);
}


// Browse for a new sample.
void drumkv1widget_sample::openSample ( const QString& sName )
{
	drumkv1_config *pConfig = drumkv1_config::getInstance();
	if (pConfig == NULL)
		return;

	QString sFilename = pConfig->sSampleDir;
	if (m_pSample && m_pSample->filename())
		sFilename = QString::fromUtf8(m_pSample->filename());

	// Cache supported file-types stuff from libsndfile...
	static QStringList s_filters;
	if (s_filters.isEmpty()) {
		const QString sExtMask("*.%1");
		const QString sFilterMask("%1 (%2)");
		QStringList exts;
		SF_FORMAT_INFO sffinfo;
		int iCount = 0;
		::sf_command(NULL, SFC_GET_FORMAT_MAJOR_COUNT, &iCount, sizeof(int));
		for (int i = 0 ; i < iCount; ++i) {
			sffinfo.format = i;
			::sf_command(NULL, SFC_GET_FORMAT_MAJOR, &sffinfo, sizeof(sffinfo));
			const QString sFilterName = QString(sffinfo.name)
				.replace('/', '-') // Replace some illegal characters.
				.remove('(').remove(')');
			const QString sExtension(sffinfo.extension);
			QString sExt = sExtMask.arg(sExtension);
			QString sExts = sExt;
			exts.append(sExt);
			if (sExtension.length() > 3) {
				sExt = sExtMask.arg(sExtension.left(3));
				sExts += ' ' + sExt;
				exts.append(sExt);
			}
			s_filters.append(sFilterMask.arg(sFilterName).arg(sExts));
		}
		s_filters.prepend(sFilterMask.arg(tr("Audio files")).arg(exts.join(" ")));
		s_filters.append(sFilterMask.arg(tr("All files")).arg("*.*"));
	}

	const QString& sTitle  = tr("Open Sample [%1]").arg(sName) + " - " DRUMKV1_TITLE;
	const QString& sFilter = s_filters.join(";;");
	QWidget *pParentWidget = NULL;
	QFileDialog::Options options = 0;
	if (pConfig->bDontUseNativeDialogs) {
		options |= QFileDialog::DontUseNativeDialog;
		pParentWidget = parentWidget();
	}
#if 1//QT_VERSION < 0x040400
	sFilename = QFileDialog::getOpenFileName(pParentWidget,
		sTitle, sFilename, sFilter, NULL, options);
#else
	QFileDialog fileDialog(pParentWidget, sTitle, sFilename, sFilter);
	fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
	fileDialog.setFileMode(QFileDialog::ExistingFile);
	QList<QUrl> urls(fileDialog.sidebarUrls());
	urls.append(QUrl::fromLocalFile(pConfig->sSampleDir));
	fileDialog.setSidebarUrls(urls);
	fileDialog.setOptions(options);
	if (fileDialog.exec())
		sFilename = fileDialog.selectedFiles().first();
#endif
	if (!sFilename.isEmpty()) {
		pConfig->sSampleDir = QFileInfo(sFilename).absolutePath();
		emit loadSampleFile(sFilename);
	}
}


// Effective sample slot.
void drumkv1widget_sample::loadSample ( drumkv1_sample *pSample )
{
	setSample(pSample);
}


// Update tool-tip.
void drumkv1widget_sample::updateToolTip (void)
{
	QString sToolTip;

	if (!m_sName.isEmpty())
		sToolTip += '[' + m_sName + ']';

	const char *pszSampleFile = (m_pSample ? m_pSample->filename() : 0);
	if (pszSampleFile) {
		if (!sToolTip.isEmpty()) sToolTip += '\n';
		sToolTip += tr("%1\n%2 frames, %3 channels, %4 Hz")
			.arg(QFileInfo(pszSampleFile).completeBaseName())
			.arg(m_pSample->length())
			.arg(m_pSample->channels())
			.arg(m_pSample->rate());
	}

	if (m_bLoop && m_iLoopStart < m_iLoopEnd) {
		if (!sToolTip.isEmpty()) sToolTip += '\n';
		sToolTip += tr("Loop start: %1, end: %2")
			.arg(m_iLoopStart)
			.arg(m_iLoopEnd);
	}

	setToolTip(sToolTip);
}


// Default size hint.
QSize drumkv1widget_sample::sizeHint (void) const
{
	return QSize(380, 80);
}


// end of drumkv1widget_sample.cpp
