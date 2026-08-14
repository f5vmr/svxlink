/**
@file   FederationLibrary.cpp
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


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <fstream>
#include <json/json.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "FederationLibrary.h"


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

FederationLibrary::FederationLibrary(void)
  : m_schema(0), m_generation(0), m_route_count(0)
{
} /* FederationLibrary::FederationLibrary */


FederationLibrary::~FederationLibrary(void)
{
} /* FederationLibrary::~FederationLibrary */


bool FederationLibrary::load(const std::string& path,
                             const std::string& expected_domain,
                             std::string& error)
{
  std::ifstream stream(path.c_str());
  if (!stream.is_open())
  {
    error = "Could not open federation library: " + path;
    return false;
  }

  Json::CharReaderBuilder builder;
  builder["collectComments"] = false;

  Json::Value root;
  std::string parse_errors;
  if (!Json::parseFromStream(builder, stream, &root, &parse_errors))
  {
    error = "Could not parse federation library: " + parse_errors;
    return false;
  }

  if (!root.isObject())
  {
    error = "Federation library root must be a JSON object";
    return false;
  }

  if (!root["schema"].isUInt() || (root["schema"].asUInt() != 1))
  {
    error = "Federation library schema must be 1";
    return false;
  }

  if (!root["generation"].isUInt64() ||
      (root["generation"].asUInt64() == 0))
  {
    error = "Federation library generation must be a positive integer";
    return false;
  }

  if (!root["domain"].isString() || root["domain"].asString().empty())
  {
    error = "Federation library domain is missing or empty";
    return false;
  }

  const std::string candidate_domain(root["domain"].asString());
  if (!expected_domain.empty() && (candidate_domain != expected_domain))
  {
    error = "Federation library domain does not match FEDERATION/DOMAIN";
    return false;
  }

  if (!root["routes"].isArray())
  {
    error = "Federation library routes must be an array";
    return false;
  }

  // Commit metadata only after the complete candidate has passed validation.
  m_schema = root["schema"].asUInt();
  m_generation = root["generation"].asUInt64();
  m_domain = candidate_domain;
  m_route_count = root["routes"].size();

  error.clear();
  return true;
} /* FederationLibrary::load */
