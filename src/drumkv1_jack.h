// drumkv1_jack.h
//
/****************************************************************************
   Copyright (C) 2012-2024, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __drumkv1_jack_h
#define __drumkv1_jack_h

#include "drumkv1.h"

#include <jack/jack.h>


#ifdef CONFIG_ALSA_MIDI
#include <jack/ringbuffer.h>
#include <alsa/asoundlib.h>
// forward decls.
class drumkv1_alsa_thread;
#endif


//-------------------------------------------------------------------------
// drumkv1_jack - decl.
//

class drumkv1_jack : public drumkv1
{
public:

	drumkv1_jack(const char *client_name);

	~drumkv1_jack();

	jack_client_t *client() const;

	void open(const char *client_name);
	void close();

	void activate();
	void deactivate();

	int process(jack_nframes_t nframes);

#ifdef CONFIG_ALSA_MIDI
	snd_seq_t *alsa_seq() const;
	void alsa_capture(snd_seq_event_t *ev);
#endif

#ifdef CONFIG_JACK_SESSION
	// JACK session event handler.
	void sessionEvent(void *pvSessionArg);
#endif

	void shutdown();
	void shutdown_close();

protected:

	void updatePreset(bool bDirty);
	void updateParam(drumkv1::ParamIndex index);
	void updateParams();

	void updateSample();

	void updateOffsetRange();

	void selectSample(int key);

	void updateTuning();

private:

	jack_client_t *m_client;

	volatile bool m_activated;

	jack_port_t **m_audio_ins;
	jack_port_t **m_audio_outs;

	float **m_ins;
	float **m_outs;

	float m_params[drumkv1::NUM_PARAMS];

#ifdef CONFIG_JACK_MIDI
	jack_port_t *m_midi_in;
#endif
#ifdef CONFIG_ALSA_MIDI
	snd_seq_t *m_alsa_seq;
//	int m_alsa_client;
	int m_alsa_port;
	snd_midi_event_t *m_alsa_decoder;
	jack_ringbuffer_t *m_alsa_buffer;
	drumkv1_alsa_thread *m_alsa_thread;
#endif
};


//-------------------------------------------------------------------------
// drumkv1_jack_application -- Singleton application instance.
//

#include <QObject>
#include <QStringList>


// forward decls.
class QCoreApplication;
class drumkv1widget_jack;

#ifdef CONFIG_NSM
class drumkv1_nsm;
#endif

#ifdef HAVE_SIGNAL_H
class QSocketNotifier;
#endif

class drumkv1_jack_application : public QObject
{
	Q_OBJECT

public:

	// Constructor.
	drumkv1_jack_application(int& argc, char **argv);

	// Destructor.
	~drumkv1_jack_application();

	// Facade method.
	int exec();

	// JACK shutdown handler.
	void shutdown();

	// Pseudo-singleton accessor.
	static drumkv1_jack_application *getInstance();

signals:

	void shutdown_signal();

protected slots:

#ifdef CONFIG_NSM
	// NSM callback slots.
	void openSession();
	void saveSession();
	void hideSession();
	void showSession();
#endif	// CONFIG_NSM

#ifdef HAVE_SIGNAL_H
	// SIGTERM signal handler.
	void handle_sigterm();
#endif

	void watchdog_slot();
	void shutdown_slot();

protected:

	// Argument parser method.
	bool parse_args();
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
	void show_error(const QString& msg);
#endif

	// Startup method.
	bool setup();

	void watchdog_start();

private:

	// Instance variables.
	QCoreApplication *m_pApp;
	bool m_bGui;

	QString m_sClientName;
	QStringList m_presets;

	drumkv1_jack *m_pDrumk;
	drumkv1widget_jack *m_pWidget;

#ifdef CONFIG_NSM
	drumkv1_nsm *m_pNsmClient;
#endif

#ifdef HAVE_SIGNAL_H
	QSocketNotifier *m_pSigtermNotifier;
#endif

	static drumkv1_jack_application *g_pInstance;
};


#endif// __drumkv1_jack_h

// end of drumkv1_jack.h

