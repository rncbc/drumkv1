// drumkv1widget_sample.cpp
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

#include "drumkv1widget_sample.h"

#include "drumkv1_config.h"
#include "drumkv1_sample.h"

#include "drumkv1_ui.h"

#include "drumkv1widget_spinbox.h"

#include <sndfile.h>

#include <QPainter>

#include <QLinearGradient>

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
drumkv1widget_sample::drumkv1widget_sample ( QWidget *pParent )
	: QFrame(pParent), m_pSample(nullptr), m_iChannels(0), m_ppPolyg(nullptr)
{
	QFrame::setMouseTracking(true);
	QFrame::setFocusPolicy(Qt::ClickFocus);

	QFrame::setMinimumSize(QSize(380, 80));
	QFrame::setSizePolicy(
		QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

	QFrame::setAcceptDrops(true);

	QFrame::setFrameShape(QFrame::Panel);
	QFrame::setFrameShadow(QFrame::Sunken);

	m_bOffset = false;
	m_iOffsetStart = m_iOffsetEnd = 0;

	m_dragCursor  = DragNone;
	m_pDragSample = nullptr;

	resetDragState();
}


// Destructor.
drumkv1widget_sample::~drumkv1widget_sample (void)
{
	setSample(nullptr);
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

//	m_bOffset = 0;
//	m_iOffsetStart = m_iOffsetEnd = 0;

	m_pDragSample = nullptr;

	if (m_pSample)
		m_iChannels = m_pSample->channels();
	if (m_iChannels > 0 && m_ppPolyg == 0) {
		const int h = height();
		const int w = width() & 0x7ffe; // force even.
		const int w2 = (w >> 1);
		const uint32_t nframes = m_pSample->length();
		const uint32_t nperiod = nframes / w2;
		const int h0 = h / m_iChannels;
		const int h1 = (h0 >> 1);
		int y0 = h1;
		m_ppPolyg = new QPolygon* [m_iChannels];
		for (uint16_t k = 0; k < m_iChannels; ++k) {
			m_ppPolyg[k] = new QPolygon(w);
			const float *pframes = m_pSample->frames(k);
			float vmax = 0.0f;
			float vmin = 0.0f;
			int n = 0;
			int x = 1;
			uint32_t j = 0;
			for (uint32_t i = 0; i < nframes; ++i) {
				const float v = *pframes++;
				if (vmax < v || j == 0)
					vmax = v;
				if (vmin > v || j == 0)
					vmin = v;
				if (++j > nperiod) {
					m_ppPolyg[k]->setPoint(n, x, y0 - int(vmax * h1));
					m_ppPolyg[k]->setPoint(w - n - 1, x, y0 - int(vmin * h1));
					vmax = vmin = 0.0f;
					++n; x += 2; j = 0;
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


// Offset mode.
void drumkv1widget_sample::setOffset ( bool bOffset )
{
	m_bOffset = bOffset;

	updateToolTip();
	update();
}

bool drumkv1widget_sample::isOffset (void) const
{
	return m_bOffset;
}


void drumkv1widget_sample::setOffsetStart ( uint32_t iOffsetStart )
{
	m_iOffsetStart = iOffsetStart;

	updateToolTip();
	update();
}

uint32_t drumkv1widget_sample::offsetStart (void) const
{
	return m_iOffsetStart;
}


void drumkv1widget_sample::setOffsetEnd ( uint32_t iOffsetEnd )
{
	m_iOffsetEnd = iOffsetEnd;

	updateToolTip();
	update();
}

uint32_t drumkv1widget_sample::offsetEnd (void) const
{
	return m_iOffsetEnd;
}


// Sanitizer helper.
int drumkv1widget_sample::safeX ( int x ) const
{
	const int w = QFrame::width();
	return (x < w ? x : w);
}


// Sanitized converters.
int drumkv1widget_sample::pixelFromFrames ( uint32_t n ) const
{
	const uint32_t nframes = m_pSample->length();
	if (nframes == 0)
		return 0;

	const int w = QFrame::width();
	const int x = (uint64_t(w) * uint64_t(n)) / uint64_t(nframes);
	return (x < w ? x : w);
}


uint32_t drumkv1widget_sample::framesFromPixel ( int x ) const
{
	const int w = QFrame::width();
	if (w == 0)
		return 0;

	const uint32_t nframes = m_pSample->length();
	const uint32_t n = (uint64_t(x) * uint64_t(nframes)) / uint64_t(w);
	return (n < nframes ? n : nframes);
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
		} else {
			if (m_bOffset) {
				m_iDragOffsetStartX = pixelFromFrames(m_iOffsetStart);
				m_iDragOffsetEndX   = pixelFromFrames(m_iOffsetEnd);
				m_dragState = m_dragCursor;
			}
		}
	}

	QFrame::mousePressEvent(pMouseEvent);
}


void drumkv1widget_sample::mouseMoveEvent ( QMouseEvent *pMouseEvent )
{
	const int x = pMouseEvent->pos().x();

	switch (m_dragState) {
	case DragNone:
		if (m_pSample) {
			const int dx = QApplication::startDragDistance();
			if (m_bOffset) {
				int x0 = pixelFromFrames(m_iOffsetEnd);
				if (qAbs(x0 - x) < dx) {
					m_dragCursor = DragOffsetEnd;
					QFrame::setCursor(QCursor(Qt::SizeHorCursor));
					QToolTip::showText(
						QCursor::pos(),
						tr("Offset end: %1")
							.arg(textFromValue(m_iOffsetEnd)), this);
					break;
				}
				x0 = pixelFromFrames(m_iOffsetStart);
				if (qAbs(x0 - x) < dx) {
					m_dragCursor = DragOffsetStart;
					QFrame::setCursor(QCursor(Qt::SizeHorCursor));
					QToolTip::showText(
						QCursor::pos(),
						tr("Offset start: %1")
							.arg(textFromValue(m_iOffsetStart)), this);
					break;
				}
			}
			if (m_dragCursor != DragNone) {
				m_dragCursor  = DragNone;
				QFrame::unsetCursor();
			}
		}
		break;
	case DragOffsetStart:
		if (m_pSample) {
			m_iDragOffsetStartX = safeX(x);
			if (m_iDragOffsetStartX > m_iDragOffsetEndX)
				m_iDragOffsetStartX = m_iDragOffsetEndX;
			update();
			const uint32_t iOffsetStart
				= framesFromPixel(m_iDragOffsetStartX);
			QToolTip::showText(
				QCursor::pos(),
				tr("Offset start: %1")
					.arg(textFromValue(iOffsetStart)), this);
		}
		break;
	case DragOffsetEnd:
		if (m_pSample) {
			m_iDragOffsetEndX = safeX(x);
			if (m_iDragOffsetEndX < m_iDragOffsetStartX)
				m_iDragOffsetEndX = m_iDragOffsetStartX;
			update();
			const uint32_t iOffsetEnd
				= framesFromPixel(m_iDragOffsetEndX);
			QToolTip::showText(
				QCursor::pos(),
				tr("Offset end: %1")
					.arg(textFromValue(iOffsetEnd)), this);
		}
		break;
	case DragOffsetRange:
		// Rubber-band offset selection...
		if (m_pSample) {
			const QRect& rect = QRect(m_posDrag, pMouseEvent->pos()).normalized();
			m_iDragOffsetStartX = safeX(rect.left());
			m_iDragOffsetEndX   = safeX(rect.right());
			update();
			const uint32_t iOffsetStart
				= framesFromPixel(m_iDragOffsetStartX);
			const uint32_t iOffsetEnd
				= framesFromPixel(m_iDragOffsetEndX);
			QToolTip::showText(
				QCursor::pos(),
				tr("Offset start: %1, end: %2")
					.arg(textFromValue(iOffsetStart))
					.arg(textFromValue(iOffsetEnd)), this);
		}
		break;
	case DragStart:
		// Rubber-band starting...
		if ((m_posDrag - pMouseEvent->pos()).manhattanLength()
			> QApplication::startDragDistance()) {
			// Start dragging alright...
			if (m_dragCursor != DragNone)
				m_dragState = m_dragCursor;
			else
			if (m_bOffset && (pMouseEvent->modifiers()
				& (Qt::ShiftModifier | Qt::ControlModifier))) {
				m_dragState = m_dragCursor = DragOffsetRange;
				m_iDragOffsetStartX = m_iDragOffsetEndX = m_posDrag.x();
				QFrame::setCursor(QCursor(Qt::SizeHorCursor));
			}
			else
			if (m_pSample && m_pSample->filename()) {
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
	case DragOffsetStart:
		if (m_pSample && m_iDragOffsetStartX < m_iDragOffsetEndX) {
			m_iOffsetStart = framesFromPixel(m_iDragOffsetStartX);
			emit offsetRangeChanged();
		}
		break;
	case DragOffsetEnd:
		if (m_pSample && m_iDragOffsetStartX < m_iDragOffsetEndX) {
			m_iOffsetEnd = framesFromPixel(m_iDragOffsetEndX);
			emit offsetRangeChanged();
		}
		break;
	case DragOffsetRange:
		if (m_pSample && m_iDragOffsetStartX < m_iDragOffsetEndX) {
			m_iOffsetStart = framesFromPixel(m_iDragOffsetStartX);
			m_iOffsetEnd   = framesFromPixel(m_iDragOffsetEndX);
			emit offsetRangeChanged();
		}
		// Fall thru...
	default:
		break;
	}

	m_pDragSample = nullptr;
	resetDragState();

	updateToolTip();
	update();
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
		m_pDragSample = nullptr;
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

	m_iDragOffsetStartX = m_iDragOffsetEndX = 0;

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
	const QColor& rgbLite = (isEnabled() ? Qt::yellow : pal.mid().color());
	const QColor& rgbDark = pal.window().color().darker();

	painter.fillRect(rect, rgbDark);

	if (m_pSample && m_ppPolyg) {
		const bool bEnabled = isEnabled();
		QColor rgbLite1(rgbLite);
		QColor rgbDrop1(Qt::black);
		rgbLite1.setAlpha(bDark ? 120 : 180);
		rgbDrop1.setAlpha(80);
		const int w2 = (w << 1);
		painter.setRenderHint(QPainter::Antialiasing, true);
		// Sample waveform...
		QLinearGradient grad(0, 0, w2, h);
		painter.setPen(rgbLite1.darker(120));
		grad.setColorAt(0.0f, rgbLite1);
		grad.setColorAt(1.0f, rgbDrop1);
		painter.setBrush(grad);
		for (unsigned short k = 0; k < m_iChannels; ++k)
			painter.drawPolygon(*m_ppPolyg[k]);
		// Offset line...
		if (m_bOffset && bEnabled) {
			int x1 = 0, x2 = 0;
			if (m_dragState == DragOffsetStart ||
				m_dragState == DragOffsetEnd   ||
				m_dragState == DragOffsetRange) {
				x1 = m_iDragOffsetStartX;
				x2 = m_iDragOffsetEndX;
			} else {
				x1 = pixelFromFrames(m_iOffsetStart);
				x2 = pixelFromFrames(m_iOffsetEnd);
			}
			QColor rgbOver(rgbDark.darker(220));
			rgbOver.setAlpha(120);
			painter.setPen(rgbLite1.darker(160));
			painter.setBrush(rgbLite1.darker());
			QPolygon polyg(3);
		//	polyg.putPoints(0, 3, x1 + 8, 0, x1, 8, x1, 0);
		//	painter.drawPolygon(polyg);
			polyg.putPoints(0, 3, x1 + 8, h, x1, h - 8, x1, h);
			painter.drawPolygon(polyg);
			painter.fillRect(0, 0, x1, h, rgbOver);
			painter.drawLine(x1, 0, x1, h - 8);
			painter.drawLine(x2, 8, x2, h);
			polyg.putPoints(0, 3, x2 - 8, 0, x2, 8, x2, 0);
			painter.drawPolygon(polyg);
		//	polyg.putPoints(0, 3, x2 - 8, h, x2, h - 8, x2, h);
		//	painter.drawPolygon(polyg);
			painter.fillRect(x2, 0, w, h, rgbOver);
		}
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
	if (pConfig == nullptr)
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
		::sf_command(nullptr, SFC_GET_FORMAT_MAJOR_COUNT, &iCount, sizeof(int));
		for (int i = 0 ; i < iCount; ++i) {
			sffinfo.format = i;
			::sf_command(nullptr, SFC_GET_FORMAT_MAJOR, &sffinfo, sizeof(sffinfo));
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

	const QString& sTitle  = tr("Open Sample [%1]").arg(sName);
	const QString& sFilter = s_filters.join(";;");
	QWidget *pParentWidget = nullptr;
	QFileDialog::Options options;
	if (pConfig->bDontUseNativeDialogs) {
		options |= QFileDialog::DontUseNativeDialog;
		pParentWidget = QWidget::window();
	}
#if 1//QT_VERSION < QT_VERSION_CHECK(4, 4, 0)
	sFilename = QFileDialog::getOpenFileName(pParentWidget,
		sTitle, sFilename, sFilter, nullptr, options);
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


// Value/text format converter utilities.
uint32_t drumkv1widget_sample::valueFromText ( const QString& text ) const
{
	drumkv1widget_spinbox::Format format = drumkv1widget_spinbox::Frames;
	drumkv1_config *pConfig = drumkv1_config::getInstance();
	if (pConfig)
		format = drumkv1widget_spinbox::Format(pConfig->iFrameTimeFormat);
	const float srate = (m_pSample ? m_pSample->sampleRate() : 44100.0f);
	return drumkv1widget_spinbox::valueFromText(text, format, srate);
}


QString drumkv1widget_sample::textFromValue ( uint32_t value ) const
{
	drumkv1widget_spinbox::Format format = drumkv1widget_spinbox::Frames;
	drumkv1_config *pConfig = drumkv1_config::getInstance();
	if (pConfig)
		format = drumkv1widget_spinbox::Format(pConfig->iFrameTimeFormat);
	const float srate = (m_pSample ? m_pSample->sampleRate() : 44100.0f);
	return drumkv1widget_spinbox::textFromValue(value, format, srate);
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
		QString suffix;
		drumkv1widget_spinbox::Format format = drumkv1widget_spinbox::Frames;
		drumkv1_config *pConfig = drumkv1_config::getInstance();
		if (pConfig)
			format = drumkv1widget_spinbox::Format(pConfig->iFrameTimeFormat);
		if (format == drumkv1widget_spinbox::Frames)
			suffix = tr(" frames");
		sToolTip += tr("%1\n%2%3, %4 channels, %5 Hz")
			.arg(QFileInfo(pszSampleFile).completeBaseName())
			.arg(m_pSample->length()).arg(suffix)
			.arg(m_pSample->channels())
			.arg(m_pSample->rate());
	}

	if (m_bOffset && m_iOffsetStart < m_iOffsetEnd) {
		if (!sToolTip.isEmpty()) sToolTip += '\n';
		sToolTip += tr("Offset start: %1, end: %2")
			.arg(textFromValue(m_iOffsetStart))
			.arg(textFromValue(m_iOffsetEnd));
	}

	setToolTip(sToolTip);
}


// Default size hint.
QSize drumkv1widget_sample::sizeHint (void) const
{
	return QSize(380, 80);
}


// end of drumkv1widget_sample.cpp
