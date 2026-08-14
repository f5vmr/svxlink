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

  std::vector<PeerConfig> candidate_peer_configs;

  for (std::vector<std::string>::const_iterator it=m_peers.begin();
       it!=m_peers.end(); ++it)
  {
    if (it->empty())
    {
      std::cerr << "*** ERROR: FEDERATION/PEERS contains an empty peer"
                << std::endl;
      return false;
    }

    if (!m_library.hasPeerPolicy(*it))
    {
      std::cerr << "*** ERROR: Federation peer " << *it
                << " has no entry in the library peer_policy"
                << std::endl;
      return false;
    }

    const std::string section("FEDERATION_PEER_" + *it);
    PeerConfig peer;
    peer.name = *it;

    if (!cfg.getValue(section, "HOST", peer.host) ||
        peer.host.empty())
    {
      std::cerr << "*** ERROR: " << section
                << "/HOST is missing or empty"
                << std::endl;
      return false;
    }

    // The stable federation identity defaults to the current endpoint.
    peer.reflector_id = peer.host;
    cfg.getValue(section, "REFLECTOR_ID", peer.reflector_id);

    if (peer.reflector_id.empty())
    {
      std::cerr << "*** ERROR: " << section
                << "/REFLECTOR_ID must not be empty"
                << std::endl;
      return false;
    }

    unsigned port = peer.port;
    cfg.getValue(section, "PORT", port);

    if ((port == 0) || (port > 65535))
    {
      std::cerr << "*** ERROR: " << section
                << "/PORT must be between 1 and 65535"
                << std::endl;
      return false;
    }
    peer.port = static_cast<std::uint16_t>(port);

    cfg.getValue(section, "PROTOCOL", peer.protocol);
    if (peer.protocol != 2)
    {
      std::cerr << "*** ERROR: " << section
                << "/PROTOCOL must currently be 2"
                << std::endl;
      return false;
    }

    if (!cfg.getValue(section, "AUTH_KEY", peer.auth_key) ||
        peer.auth_key.empty())
    {
      std::cerr << "*** ERROR: " << section
                << "/AUTH_KEY is missing or empty"
                << std::endl;
      return false;
    }

    cfg.getValue(section, "CONNECT", peer.connect);
    candidate_peer_configs.push_back(peer);
  }

  m_peer_configs.swap(candidate_peer_configs);

  std::cout << "Reflector federation enabled:"
            << " domain=" << m_domain
            << " reflector_id=" << m_reflector_id
            << " callsign=" << m_callsign
            << " peers=" << m_peers.size()
            << " library=" << m_library_path
            << " generation=" << m_library.generation()
            << " routes=" << m_library.routeCount()
            << std::endl;
  for (std::vector<PeerConfig>::const_iterator
          it=m_peer_configs.begin();
      it!=m_peer_configs.end(); ++it)
  {
    std::cout << "  Federation peer:"
              << " name=" << it->name
              << " reflector_id=" << it->reflector_id
              << " host=" << it->host
              << " port=" << it->port
              << " protocol=" << it->protocol
              << " connect=" << (it->connect ? "yes" : "no")
              << std::endl;
  }

  return true;
} /* ReflectorFederation::initialize */
