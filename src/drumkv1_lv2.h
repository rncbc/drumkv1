// drumkv1_lv2.h
//
/****************************************************************************
   Copyright (C) 2012-2015, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __drumkv1_lv2_h
#define __drumkv1_lv2_h

#include "drumkv1.h"

#include "lv2.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"

#define DRUMKV1_LV2_URI "http://drumkv1.sourceforge.net/lv2"
#define DRUMKV1_LV2_PREFIX DRUMKV1_LV2_URI "#"


//-------------------------------------------------------------------------
// drumkv1_lv2 - decl.
//

class drumkv1_lv2 : public drumkv1
{
public:

	drumkv1_lv2(double sample_rate, const LV2_Feature *const *host_features);

	~drumkv1_lv2();

	enum PortIndex {

		MidiIn = 0,
		AudioInL,
		AudioInR,
		AudioOutL,
		AudioOutR,
		ParamBase
	};

	void connect_port(uint32_t port, void *data);

	void run(uint32_t nframes);

	void activate();
	void deactivate();

	uint32_t urid_map(const char *uri) const;

private:

	LV2_URID_Map *m_urid_map;

	struct lv2_urids
	{
		LV2_URID atom_Blank;
		LV2_URID atom_Object;
		LV2_URID atom_Float;
		LV2_URID time_Position;
		LV2_URID time_beatsPerMinute;
		LV2_URID midi_MidiEvent;

	} m_urids;

	LV2_Atom_Sequence *m_atom_sequence;

	float **m_ins;
	float **m_outs;
};


#endif// __drumkv1_lv2_h

// end of drumkv1_lv2.h
