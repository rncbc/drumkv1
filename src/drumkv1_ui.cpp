// drumkv1_ui.cpp
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

#include "drumkv1_ui.h"


//-------------------------------------------------------------------------
// drumkv1_ui - decl.
//

drumkv1_ui::drumkv1_ui ( drumkv1 *pDrumk ) : m_pDrumk(pDrumk)
{
}


drumkv1 *drumkv1_ui::instance (void) const
{
	return m_pDrumk;
}


drumkv1_element *drumkv1_ui::addElement ( int key )
{
	return m_pDrumk->addElement(key);
}

drumkv1_element *drumkv1_ui::element ( int key ) const
{
	return m_pDrumk->element(key);
}

void drumkv1_ui::removeElement ( int key )
{
	m_pDrumk->removeElement(key);
}


void drumkv1_ui::setCurrentElement ( int key )
{
	m_pDrumk->setCurrentElement(key);
}

int drumkv1_ui::currentElement (void) const
{
	return m_pDrumk->currentElement();
}


void drumkv1_ui::clearElements (void)
{
	m_pDrumk->clearElements();
}


void drumkv1_ui::setSampleFile ( const char *pszSampleFile )
{
	m_pDrumk->setSampleFile(pszSampleFile);
}

const char *drumkv1_ui::sampleFile (void) const
{
	return m_pDrumk->sampleFile();
}


drumkv1_sample *drumkv1_ui::sample (void) const
{
	return m_pDrumk->sample();
}


void drumkv1_ui::setReverse ( bool bReverse )
{
	m_pDrumk->setReverse(bReverse);
}

bool drumkv1_ui::isReverse (void) const
{
	return m_pDrumk->isReverse();
}


void drumkv1_ui::setParamValue ( drumkv1::ParamIndex index, float fValue )
{
	m_pDrumk->setParamValue(index, fValue);
}

float drumkv1_ui::paramValue ( drumkv1::ParamIndex index ) const
{
	return m_pDrumk->paramValue(index);
}


drumkv1_programs *drumkv1_ui::programs (void) const
{
	return m_pDrumk->programs();
}


void drumkv1_ui::resetParamValues ( bool bSwap )
{
	return m_pDrumk->resetParamValues(bSwap);
}


void drumkv1_ui::reset (void)
{
	return m_pDrumk->reset();
}


// end of drumkv1_ui.cpp
