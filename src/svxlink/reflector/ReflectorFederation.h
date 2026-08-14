/**
@file   ReflectorFederation.h
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

#ifndef REFLECTOR_FEDERATION_INCLUDED
#define REFLECTOR_FEDERATION_INCLUDED

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "FederationLibrary.h"

/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief  Manage federation between autonomous SVXReflectors

This class will own federation peers, talkgroup routing policy and runtime
federation state. The initial implementation is deliberately inert.
*/
class ReflectorFederation
{
  public:
    struct PeerConfig
    {
      std::string   name;
      std::string   reflector_id;
      std::string   host;
      std::uint16_t port;
      unsigned      protocol;
      std::string   auth_key;
      bool          connect;

      PeerConfig(void)
        : port(5300), protocol(2), connect(true)
      {
      }
    };

    struct TrustEntry
    {
      std::string callsign;
      std::string peer;
    };

    ReflectorFederation(void);
    ~ReflectorFederation(void);

    bool initialize(Async::Config& cfg);
    bool isEnabled(void) const { return m_enabled; }

    const std::string& domain(void) const { return m_domain; }
    const std::string& reflectorId(void) const { return m_reflector_id; }
    const std::string& callsign(void) const { return m_callsign; }
    const std::string& libraryPath(void) const { return m_library_path; }

    const std::vector<std::string>& peers(void) const
    {
      return m_peers;
    }

    const std::vector<PeerConfig>& peerConfigs(void) const
    {
      return m_peer_configs;
    }

    const std::vector<TrustEntry>& trustEntries(void) const
    {
      return m_trust_entries;
    }

    const TrustEntry* findTrustByCallsign(
        const std::string& callsign) const;

    const TrustEntry* findTrustByPeer(
        const std::string& peer) const;

    std::uint64_t libraryGeneration(void) const
    {
      return m_library.generation();
    }

    std::size_t routeCount(void) const
    {
      return m_library.routeCount();
    }

    bool mayImport(const std::string& peer, std::uint32_t tg) const
    {
      return m_enabled && m_library.mayImport(peer, tg);
    }

    bool mayExport(const std::string& peer, std::uint32_t tg) const
    {
      return m_enabled && m_library.mayExport(peer, tg);
    }

  private:
    bool                      m_enabled;
    std::string               m_domain;
    std::string               m_reflector_id;
    std::string               m_callsign;
    std::string               m_library_path;
    std::vector<std::string>  m_peers;
    std::vector<PeerConfig>   m_peer_configs;
    std::vector<TrustEntry>   m_trust_entries;
    FederationLibrary         m_library;

    ReflectorFederation(const ReflectorFederation&);
    ReflectorFederation& operator=(const ReflectorFederation&);
};  /* class ReflectorFederation */


#endif /* REFLECTOR_FEDERATION_INCLUDED */