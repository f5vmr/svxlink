/**
@file   FederationLibrary.h
@brief  Hot-reloadable talkgroup library for reflector federation
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

#ifndef FEDERATION_LIBRARY_INCLUDED
#define FEDERATION_LIBRARY_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cstddef>
#include <cstdint>
#include <string>


/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief  Load and validate a federation talkgroup library

A failed load leaves the previously accepted library metadata unchanged.
*/
class FederationLibrary
{
  public:
    FederationLibrary(void);
    ~FederationLibrary(void);

    bool load(const std::string& path,
              const std::string& expected_domain,
              std::string& error);

    unsigned schema(void) const { return m_schema; }
    std::uint64_t generation(void) const { return m_generation; }
    const std::string& domain(void) const { return m_domain; }
    std::size_t routeCount(void) const { return m_route_count; }

  private:
    unsigned       m_schema;
    std::uint64_t  m_generation;
    std::string    m_domain;
    std::size_t    m_route_count;

    FederationLibrary(const FederationLibrary&);
    FederationLibrary& operator=(const FederationLibrary&);
};  /* class FederationLibrary */


#endif /* FEDERATION_LIBRARY_INCLUDED */