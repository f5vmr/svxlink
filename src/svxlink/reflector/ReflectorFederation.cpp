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
 * System Includes
 *
 ****************************************************************************/ 

#include <iostream>

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
  : m_enabled(false),
    m_library_path("/etc/svxlink/federation.json")
{
} /* ReflectorFederation::ReflectorFederation */

ReflectorFederation::~ReflectorFederation(void)
{
} /* ReflectorFederation::~ReflectorFederation */


bool ReflectorFederation::initialize(Async::Config& cfg)
{
  cfg.getValue("FEDERATION", "ENABLE", m_enabled);

  if (!m_enabled)
  {
    return true;
  }

  bool config_ok = true;

  if (!cfg.getValue("FEDERATION", "DOMAIN", m_domain) || m_domain.empty())
  {
    std::cerr << "*** ERROR: FEDERATION/DOMAIN is missing or empty"
              << std::endl;
    config_ok = false;
  }

  if (!cfg.getValue("FEDERATION", "REFLECTOR_ID", m_reflector_id) ||
      m_reflector_id.empty())
  {
    std::cerr << "*** ERROR: FEDERATION/REFLECTOR_ID is missing or empty"
              << std::endl;
    config_ok = false;
  }

  if (!cfg.getValue("FEDERATION", "CALLSIGN", m_callsign) ||
      m_callsign.empty())
  {
    std::cerr << "*** ERROR: FEDERATION/CALLSIGN is missing or empty"
              << std::endl;
    config_ok = false;
  }

  cfg.getValue("FEDERATION", "LIBRARY", m_library_path);
  cfg.getValue("FEDERATION", "PEERS", m_peers);

  if (!config_ok)
  {
    return false;
  }

  std::string library_error;
  if (!m_library.load(m_library_path, m_domain, library_error))
  {
    std::cerr << "*** ERROR: " << library_error << std::endl;
    return false;
  }

  std::cout << "Reflector federation enabled:"
            << " domain=" << m_domain
            << " reflector_id=" << m_reflector_id
            << " callsign=" << m_callsign
            << " peers=" << m_peers.size()
            << " library=" << m_library_path
            << " generation=" << m_library.generation()
            << " routes=" << m_library.routeCount()
            << std::endl;

  return true;
} /* ReflectorFederation::initialize */
