/**
@file   ReflectorFederation.cpp
@brief  Federation support for SVXReflector
@author Chris Jackson / G4NAB
@date   2026-08-14

\verbatim
Copyright (C) 2026 Chris Jackson / G4NAB

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ReflectorFederation.h"


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

ReflectorFederation::ReflectorFederation(void)
  : m_enabled(false)
{
} /* ReflectorFederation::ReflectorFederation */


ReflectorFederation::~ReflectorFederation(void)
{
} /* ReflectorFederation::~ReflectorFederation */


bool ReflectorFederation::initialize(Async::Config& cfg)
{
  cfg.getValue("FEDERATION", "ENABLE", m_enabled);
  return true;
} /* ReflectorFederation::initialize */
