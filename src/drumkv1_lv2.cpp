// drumkv1_lv2.cpp
//
/****************************************************************************
   Copyright (C) 2012-2016, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "drumkv1_lv2.h"
#include "drumkv1_config.h"

#include "drumkv1_sched.h"
#include "drumkv1_param.h"

#include "drumkv1_programs.h"
#include "drumkv1_controls.h"

#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"

#include "lv2/lv2plug.in/ns/ext/state/state.h"

#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"

#include <stdlib.h>
#include <math.h>

#include <QDomDocument>


//-------------------------------------------------------------------------
// drumkv1_lv2_map_path - abstract/absolute path functors.
//

class drumkv1_lv2_map_path : public drumkv1_param::map_path
{
public:

	drumkv1_lv2_map_path(const LV2_Feature *const *features)
		: m_map_path(NULL)
	{
		for (int i = 0; features && features[i]; ++i) {
			if (::strcmp(features[i]->URI, LV2_STATE__mapPath) == 0) {
				m_map_path = (LV2_State_Map_Path *) features[i]->data;
				break;
			}
		}
	}

	QString absolutePath(const QString& sAbstractPath) const
	{
		QString sAbsolutePath(sAbstractPath);
		if (m_map_path) {
			const char *pszAbsolutePath
				= m_map_path->absolute_path(
					m_map_path->handle, sAbstractPath.toUtf8().constData());
			if (pszAbsolutePath) {
				sAbsolutePath = pszAbsolutePath;
				::free((void *) pszAbsolutePath);
			}
		}
		return sAbsolutePath;
	}
	
	QString abstractPath(const QString& sAbsolutePath) const
	{
		QString sAbstractPath(sAbsolutePath);
		if (m_map_path) {
			const char *pszAbstractPath
				= m_map_path->abstract_path(
					m_map_path->handle, sAbsolutePath.toUtf8().constData());
			if (pszAbstractPath) {
				sAbstractPath = pszAbstractPath;
				::free((void *) pszAbstractPath);
			}
		}
		return sAbstractPath;
	}

private:

	LV2_State_Map_Path *m_map_path;
};


//-------------------------------------------------------------------------
// drumkv1_lv2 - impl.
//

drumkv1_lv2::drumkv1_lv2 (
	double sample_rate, const LV2_Feature *const *host_features )
	: drumkv1(2, float(sample_rate))
{
	m_urid_map = NULL;
	m_atom_sequence = NULL;

	const LV2_Options_Option *const *host_options = NULL;

	for (int i = 0; host_features && host_features[i]; ++i) {
		if (::strcmp(host_features[i]->URI, LV2_URID_MAP_URI) == 0) {
			m_urid_map = (LV2_URID_Map *) host_features[i]->data;
			if (m_urid_map) {
				m_urids.atom_Blank = m_urid_map->map(
					m_urid_map->handle, LV2_ATOM__Blank);
				m_urids.atom_Object = m_urid_map->map(
					m_urid_map->handle, LV2_ATOM__Object);
				m_urids.atom_Float = m_urid_map->map(
					m_urid_map->handle, LV2_ATOM__Float);
				m_urids.atom_Int = m_urid_map->map(
					m_urid_map->handle, LV2_ATOM__Int);
				m_urids.time_Position = m_urid_map->map(
					m_urid_map->handle, LV2_TIME__Position);
				m_urids.time_beatsPerMinute = m_urid_map->map(
					m_urid_map->handle, LV2_TIME__beatsPerMinute);
				m_urids.midi_MidiEvent = m_urid_map->map(
					m_urid_map->handle, LV2_MIDI__MidiEvent);
				m_urids.bufsz_minBlockLength = m_urid_map->map(
					m_urid_map->handle, LV2_BUF_SIZE__minBlockLength);
				m_urids.bufsz_maxBlockLength = m_urid_map->map(
					m_urid_map->handle, LV2_BUF_SIZE__maxBlockLength);
				m_urids.bufsz_nominalBlockLength = m_urid_map->map(
					m_urid_map->handle, LV2_BUF_SIZE__nominalBlockLength);
			}
		}
		else
		if (::strcmp(host_features[i]->URI, LV2_OPTIONS__options) == 0)
			host_options = (const LV2_Options_Option *const *) host_features[i]->data;
	}

	uint32_t buffer_size = 1024; // safe default?

	for (int i = 0; host_options && host_options[i]; ++i) {
		const LV2_Options_Option *host_option = host_options[i];
		if (host_option->type == m_urids.atom_Int) {
			uint32_t block_length = 0;
			if (host_option->key == m_urids.bufsz_minBlockLength)
				block_length = *(int *) host_option->value;
			else
			if (host_option->key == m_urids.bufsz_maxBlockLength)
				block_length = *(int *) host_option->value;
			else
			if (host_option->key == m_urids.bufsz_nominalBlockLength)
				block_length = *(int *) host_option->value;
			// choose the lengthier...
			if (buffer_size < block_length)
				buffer_size = block_length;
		}
	}

	drumkv1::setBufferSize(buffer_size);

	const uint16_t nchannels = drumkv1::channels();
	m_ins  = new float * [nchannels];
	m_outs = new float * [nchannels];
	for (uint16_t k = 0; k < nchannels; ++k)
		m_ins[k] = m_outs[k] = NULL;

	drumkv1::programs()->optional(true);
	drumkv1::controls()->optional(true);
}


drumkv1_lv2::~drumkv1_lv2 (void)
{
	delete [] m_outs;
	delete [] m_ins;
}


void drumkv1_lv2::connect_port ( uint32_t port, void *data )
{
	switch(PortIndex(port)) {
	case MidiIn:
		m_atom_sequence = (LV2_Atom_Sequence *) data;
		break;
	case AudioInL:
		m_ins[0] = (float *) data;
		break;
	case AudioInR:
		m_ins[1] = (float *) data;
		break;
	case AudioOutL:
		m_outs[0] = (float *) data;
		break;
	case AudioOutR:
		m_outs[1] = (float *) data;
		break;
	default:
		drumkv1::setParamPort(drumkv1::ParamIndex(port - ParamBase), (float *) data);
		break;
	}
}


void drumkv1_lv2::run ( uint32_t nframes )
{
	const uint16_t nchannels = drumkv1::channels();
	float *ins[nchannels], *outs[nchannels];
	for (uint16_t k = 0; k < nchannels; ++k) {
		ins[k]  = m_ins[k];
		outs[k] = m_outs[k];
	}

	uint32_t ndelta = 0;

	if (m_atom_sequence) {
		LV2_ATOM_SEQUENCE_FOREACH(m_atom_sequence, event) {
			if (event == NULL)
				continue;
			if (event->body.type == m_urids.midi_MidiEvent) {
				uint8_t *data = (uint8_t *) LV2_ATOM_BODY(&event->body);
				const uint32_t nread = event->time.frames - ndelta;
				if (nread > 0) {
					drumkv1::process(ins, outs, nread);
					for (uint16_t k = 0; k < nchannels; ++k) {
						ins[k]  += nread;
						outs[k] += nread;
					}
				}
				ndelta = event->time.frames;
				drumkv1::process_midi(data, event->body.size);
			}
			else
			if (event->body.type == m_urids.atom_Blank ||
				event->body.type == m_urids.atom_Object) {
				const LV2_Atom_Object *object
					= (LV2_Atom_Object *) &event->body;
				if (object->body.otype == m_urids.time_Position) {
					LV2_Atom *atom = NULL;
					lv2_atom_object_get(object,
						m_urids.time_beatsPerMinute, &atom, NULL);
					if (atom && atom->type == m_urids.atom_Float) {
						const float host_bpm = ((LV2_Atom_Float *) atom)->body;
						if (drumkv1::paramValue(drumkv1::LFO1_BPMSYNC) > 0.0f) {
						#ifdef CONFIG_LFO_BPMRATEX_0
							const float rate_bpm = drumkv1::lfo_rate_bpm(host_bpm);
							const float rate = drumkv1::paramValue(drumkv1::LFO1_RATE);
							if (::fabsf(rate_bpm - rate) > 0.01f)
								drumkv1::setParamValue(drumkv1::LFO1_RATE, rate_bpm);
						#else
							const float bpm = drumkv1::paramValue(drumkv1::LFO1_BPM);
							if (::fabsf(host_bpm - bpm) > 0.01f)
								drumkv1::setParamValue(drumkv1::LFO1_BPM, host_bpm);
						#endif
						}
						if (drumkv1::paramValue(drumkv1::DEL1_BPMSYNC) > 0.0f) {
							const float bpm = drumkv1::paramValue(drumkv1::DEL1_BPM);
							if (bpm > 0.0f && ::fabsf(host_bpm - bpm) > 0.01f)
								drumkv1::setParamValue(drumkv1::DEL1_BPM, host_bpm);
						}
					}
				}
			}
		}
	//	m_atom_sequence = NULL;
	}

	drumkv1::process(ins, outs, nframes - ndelta);
}


void drumkv1_lv2::activate (void)
{
	drumkv1::reset();
}


void drumkv1_lv2::deactivate (void)
{
	drumkv1::reset();
}


uint32_t drumkv1_lv2::urid_map ( const char *uri ) const
{
	return (m_urid_map ? m_urid_map->map(m_urid_map->handle, uri) : 0);
}


//-------------------------------------------------------------------------
// drumkv1_lv2 - LV2 State interface.
//

static LV2_State_Status drumkv1_lv2_state_save ( LV2_Handle instance,
	LV2_State_Store_Function store, LV2_State_Handle handle,
	uint32_t flags, const LV2_Feature *const *features )
{
	drumkv1_lv2 *pPlugin = static_cast<drumkv1_lv2 *> (instance);
	if (pPlugin == NULL)
		return LV2_STATE_ERR_UNKNOWN;

	const uint32_t key = pPlugin->urid_map(DRUMKV1_LV2_PREFIX "state");
	if (key == 0)
		return LV2_STATE_ERR_NO_PROPERTY;

	const uint32_t type = pPlugin->urid_map(LV2_ATOM__Chunk);
	if (type == 0)
		return LV2_STATE_ERR_BAD_TYPE;
#if 0
	if ((flags & (LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE)) == 0)
		return LV2_STATE_ERR_BAD_FLAGS;
#else
	flags |= (LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
#endif
	drumkv1_lv2_map_path mapPath(features);

	QDomDocument doc(DRUMKV1_TITLE);
	QDomElement eElements = doc.createElement("elements");
	drumkv1_param::saveElements(pPlugin, doc, eElements, mapPath);
	doc.appendChild(eElements);

	const QByteArray data(doc.toByteArray());
	const char *value = data.constData();
	size_t size = data.size();

	return (*store)(handle, key, value, size, type, flags);
}


static LV2_State_Status drumkv1_lv2_state_restore ( LV2_Handle instance,
	LV2_State_Retrieve_Function retrieve, LV2_State_Handle handle,
	uint32_t flags, const LV2_Feature *const *features )
{
	drumkv1_lv2 *pPlugin = static_cast<drumkv1_lv2 *> (instance);
	if (pPlugin == NULL)
		return LV2_STATE_ERR_UNKNOWN;

	const uint32_t key = pPlugin->urid_map(DRUMKV1_LV2_PREFIX "state");
	if (key == 0)
		return LV2_STATE_ERR_NO_PROPERTY;

	const uint32_t chunk_type = pPlugin->urid_map(LV2_ATOM__Chunk);
	if (chunk_type == 0)
		return LV2_STATE_ERR_BAD_TYPE;

	size_t size = 0;
	uint32_t type = 0;
//	flags = 0;

	const char *value
		= (const char *) (*retrieve)(handle, key, &size, &type, &flags);

	if (size < 2)
		return LV2_STATE_ERR_UNKNOWN;

	if (type != chunk_type)
		return LV2_STATE_ERR_BAD_TYPE;

	if ((flags & (LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE)) == 0)
		return LV2_STATE_ERR_BAD_FLAGS;

	if (value == NULL)
		return LV2_STATE_ERR_UNKNOWN;

	drumkv1_lv2_map_path mapPath(features);

	QDomDocument doc(DRUMKV1_TITLE);
	if (doc.setContent(QByteArray(value, size))) {
		QDomElement eElements = doc.documentElement();
		if (eElements.tagName() == "elements")
			drumkv1_param::loadElements(pPlugin, eElements, mapPath);
	}

	drumkv1_sched::sync_notify(pPlugin, drumkv1_sched::Sample, 1);

	return LV2_STATE_SUCCESS;
}


static const LV2_State_Interface drumkv1_lv2_state_interface =
{
	drumkv1_lv2_state_save,
	drumkv1_lv2_state_restore
};


#ifdef CONFIG_LV2_PROGRAMS

#include "drumkv1_programs.h"

const LV2_Program_Descriptor *drumkv1_lv2::get_program ( uint32_t index )
{
	drumkv1_programs *pPrograms = drumkv1::programs();
	const drumkv1_programs::Banks& banks = pPrograms->banks();
	drumkv1_programs::Banks::ConstIterator bank_iter = banks.constBegin();
	const drumkv1_programs::Banks::ConstIterator& bank_end = banks.constEnd();
	for (uint32_t i = 0; bank_iter != bank_end; ++bank_iter) {
		drumkv1_programs::Bank *pBank = bank_iter.value();
		const drumkv1_programs::Progs& progs = pBank->progs();
		drumkv1_programs::Progs::ConstIterator prog_iter = progs.constBegin();
		const drumkv1_programs::Progs::ConstIterator& prog_end = progs.constEnd();
		for ( ; prog_iter != prog_end; ++prog_iter, ++i) {
			drumkv1_programs::Prog *pProg = prog_iter.value();
			if (i >= index) {
				m_aProgramName = pProg->name().toUtf8();
				m_program.bank = pBank->id();
				m_program.program = pProg->id();
				m_program.name = m_aProgramName.constData();
				return &m_program;
			}
		}
	}

	return NULL;
}

void drumkv1_lv2::select_program ( uint32_t bank, uint32_t program )
{
	drumkv1::programs()->select_program(bank, program);
}

#endif	// CONFIG_LV2_PROGRAMS


//-------------------------------------------------------------------------
// drumkv1_lv2 - LV2 desc.
//

static LV2_Handle drumkv1_lv2_instantiate (
	const LV2_Descriptor *, double sample_rate, const char *,
	const LV2_Feature *const *host_features )
{
	return new drumkv1_lv2(sample_rate, host_features);
}


static void drumkv1_lv2_connect_port (
	LV2_Handle instance, uint32_t port, void *data )
{
	drumkv1_lv2 *pPlugin = static_cast<drumkv1_lv2 *> (instance);
	if (pPlugin)
		pPlugin->connect_port(port, data);
}


static void drumkv1_lv2_run ( LV2_Handle instance, uint32_t nframes )
{
	drumkv1_lv2 *pPlugin = static_cast<drumkv1_lv2 *> (instance);
	if (pPlugin)
		pPlugin->run(nframes);
}


static void drumkv1_lv2_activate ( LV2_Handle instance )
{
	drumkv1_lv2 *pPlugin = static_cast<drumkv1_lv2 *> (instance);
	if (pPlugin)
		pPlugin->activate();
}


static void drumkv1_lv2_deactivate ( LV2_Handle instance )
{
	drumkv1_lv2 *pPlugin = static_cast<drumkv1_lv2 *> (instance);
	if (pPlugin)
		pPlugin->deactivate();
}


static void drumkv1_lv2_cleanup ( LV2_Handle instance )
{
	drumkv1_lv2 *pPlugin = static_cast<drumkv1_lv2 *> (instance);
	if (pPlugin)
		delete pPlugin;
}


#ifdef CONFIG_LV2_PROGRAMS

static const LV2_Program_Descriptor *drumkv1_lv2_programs_get_program (
	LV2_Handle instance, uint32_t index )
{
	drumkv1_lv2 *pPlugin = static_cast<drumkv1_lv2 *> (instance);
	if (pPlugin)
		return pPlugin->get_program(index);
	else
		return NULL;
}

static void drumkv1_lv2_programs_select_program (
	LV2_Handle instance, uint32_t bank, uint32_t program )
{
	drumkv1_lv2 *pPlugin = static_cast<drumkv1_lv2 *> (instance);
	if (pPlugin)
		pPlugin->select_program(bank, program);
}

static const LV2_Programs_Interface drumkv1_lv2_programs_interface =
{
	drumkv1_lv2_programs_get_program,
	drumkv1_lv2_programs_select_program,
};

#endif	// CONFIG_LV2_PROGRAMS

static const void *drumkv1_lv2_extension_data ( const char *uri )
{
#ifdef CONFIG_LV2_PROGRAMS
	if (::strcmp(uri, LV2_PROGRAMS__Interface) == 0)
		return (void *) &drumkv1_lv2_programs_interface;
	else
#endif
	if (::strcmp(uri, LV2_STATE__interface))
		return NULL;

	return &drumkv1_lv2_state_interface;
}


static const LV2_Descriptor drumkv1_lv2_descriptor =
{
	DRUMKV1_LV2_URI,
	drumkv1_lv2_instantiate,
	drumkv1_lv2_connect_port,
	drumkv1_lv2_activate,
	drumkv1_lv2_run,
	drumkv1_lv2_deactivate,
	drumkv1_lv2_cleanup,
	drumkv1_lv2_extension_data
};


LV2_SYMBOL_EXPORT const LV2_Descriptor *lv2_descriptor ( uint32_t index )
{
	return (index == 0 ? &drumkv1_lv2_descriptor : NULL);
}


// end of drumkv1_lv2.cpp
