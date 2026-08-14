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

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <sstream>
#include <json/json.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "FederationLibrary.h"

/****************************************************************************
 *
 * Local functions
 *
 ****************************************************************************/

namespace {
  bool parsePositiveUint32(const std::string& text, std::uint32_t& value)
  {
    if (text.empty())
    {
      return false;
    }

    for (std::string::const_iterator it=text.begin(); it!=text.end(); ++it)
    {
      if ((*it < '0') || (*it > '9'))
      {
        return false;
      }
    }

    errno = 0;
    char *end = 0;
    const unsigned long long parsed = std::strtoull(text.c_str(), &end, 10);

    if ((errno != 0) || (end == 0) || (*end != '\0') ||
        (parsed == 0) ||
        (parsed > std::numeric_limits<std::uint32_t>::max()))
    {
      return false;
    }

    value = static_cast<std::uint32_t>(parsed);
    return true;
  }


  bool validateRoute(const Json::Value& route,
                     Json::Value::ArrayIndex index,
                     std::string& error)
  {
    std::ostringstream prefix;
    prefix << "Federation route " << index << ": ";

    if (!route.isObject())
    {
      error = prefix.str() + "entry must be a JSON object";
      return false;
    }

    if (!route["type"].isString())
    {
      error = prefix.str() + "type is missing or invalid";
      return false;
    }

    const std::string type(route["type"].asString());

    if (type == "exact")
    {
      if (!route["value"].isUInt() || (route["value"].asUInt() == 0))
      {
        error = prefix.str() +
                "exact value must be a positive talkgroup number";
        return false;
      }
    }
    else if (type == "prefix")
    {
      std::uint32_t value = 0;
      if (!route["value"].isString() ||
          !parsePositiveUint32(route["value"].asString(), value))
      {
        error = prefix.str() +
                "prefix value must be a string containing digits";
        return false;
      }
      (void)value;  // Silence unused variable warning
    }
    else if (type == "range")
    {
      if (!route["value"].isString())
      {
        error = prefix.str() +
                "range value must use the form \"first-last\"";
        return false;
      }

      const std::string range(route["value"].asString());
      const std::string::size_type separator = range.find('-');

      if ((separator == std::string::npos) ||
          (range.find('-', separator + 1) != std::string::npos))
      {
        error = prefix.str() +
                "range value must use the form \"first-last\"";
        return false;
      }

      std::uint32_t first = 0;
      std::uint32_t last = 0;

      if (!parsePositiveUint32(range.substr(0, separator), first) ||
          !parsePositiveUint32(range.substr(separator + 1), last) ||
          (first > last))
      {
        error = prefix.str() + "range value is invalid";
        return false;
      }
    }
    else
    {
      error = prefix.str() + "unknown route type \"" + type + "\"";
      return false;
    }

    if (!route["home"].isString() || route["home"].asString().empty())
    {
      error = prefix.str() + "home is missing or empty";
      return false;
    }

    if (!route["scope"].isString() || route["scope"].asString().empty())
    {
      error = prefix.str() + "scope is missing or empty";
      return false;
    }

    if (route.isMember("service_anchor") &&
        (!route["service_anchor"].isString() ||
         route["service_anchor"].asString().empty()))
    {
      error = prefix.str() + "service_anchor must be a non-empty string";
      return false;
    }

    if (route.isMember("description") &&
        !route["description"].isString())
    {
      error = prefix.str() + "description must be a string";
      return false;
    }

    return true;
  }
} /* namespace */


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

  const Json::Value& routes(root["routes"]);
  for (Json::Value::ArrayIndex index=0; index<routes.size(); ++index)
  {
    if (!validateRoute(routes[index], index, error))
    {
      return false;
    }
  }

  // Commit metadata only after the complete candidate has passed validation.
  m_schema = root["schema"].asUInt();
  m_generation = root["generation"].asUInt64();
  m_domain = candidate_domain;
  m_route_count = root["routes"].size();

  error.clear();
  return true;
} /* FederationLibrary::load */
