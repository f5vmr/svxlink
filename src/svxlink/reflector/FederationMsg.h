/**
@file   FederationMsg.h
@brief  SVXReflector federation protocol messages
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

#ifndef FEDERATION_MSG_INCLUDED
#define FEDERATION_MSG_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cstdint>
#include <string>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ReflectorMsg.h"


/****************************************************************************
 *
 * Protocol definitions
 *
 ****************************************************************************/

namespace FederationProtocol
{
  enum
  {
    VERSION_MAJOR = 1,
    VERSION_MINOR = 0,

    CAP_MULTIPLEXED_OPUS = 1U << 0
  };
}


/****************************************************************************
 *
 * TCP message definitions
 *
 ****************************************************************************/

/**
@brief  Advertise an authenticated reflector's federation identity
*/
class MsgFederationHello : public ReflectorMsgBase<200>
{
  public:
    MsgFederationHello(void)
      : m_major(FederationProtocol::VERSION_MAJOR),
        m_minor(FederationProtocol::VERSION_MINOR),
        m_generation(0),
        m_capabilities(FederationProtocol::CAP_MULTIPLEXED_OPUS)
    {
    }

    MsgFederationHello(std::uint16_t major,
                       std::uint16_t minor,
                       const std::string& reflector_id,
                       const std::string& domain,
                       std::uint64_t generation,
                       std::uint32_t capabilities)
      : m_major(major),
        m_minor(minor),
        m_reflector_id(reflector_id),
        m_domain(domain),
        m_generation(generation),
        m_capabilities(capabilities)
    {
    }

    std::uint16_t major(void) const { return m_major; }
    std::uint16_t minor(void) const { return m_minor; }

    const std::string& reflectorId(void) const
    {
      return m_reflector_id;
    }

    const std::string& domain(void) const
    {
      return m_domain;
    }

    std::uint64_t generation(void) const
    {
      return m_generation;
    }

    std::uint32_t capabilities(void) const
    {
      return m_capabilities;
    }

    ASYNC_MSG_MEMBERS(m_major,
                      m_minor,
                      m_reflector_id,
                      m_domain,
                      m_generation,
                      m_capabilities)

  private:
    std::uint16_t m_major;
    std::uint16_t m_minor;
    std::string   m_reflector_id;
    std::string   m_domain;
    std::uint64_t m_generation;
    std::uint32_t m_capabilities;
}; /* MsgFederationHello */


/**
@brief  Accept a federation hello and report negotiated capabilities
*/
class MsgFederationHelloAck : public ReflectorMsgBase<201>
{
  public:
    MsgFederationHelloAck(void)
      : m_major(FederationProtocol::VERSION_MAJOR),
        m_minor(FederationProtocol::VERSION_MINOR),
        m_capabilities(FederationProtocol::CAP_MULTIPLEXED_OPUS)
    {
    }

    MsgFederationHelloAck(std::uint16_t major,
                          std::uint16_t minor,
                          const std::string& reflector_id,
                          const std::string& domain,
                          std::uint32_t capabilities)
      : m_major(major),
        m_minor(minor),
        m_reflector_id(reflector_id),
        m_domain(domain),
        m_capabilities(capabilities)
    {
    }

    std::uint16_t major(void) const { return m_major; }
    std::uint16_t minor(void) const { return m_minor; }

    const std::string& reflectorId(void) const
    {
      return m_reflector_id;
    }

    const std::string& domain(void) const
    {
      return m_domain;
    }

    std::uint32_t capabilities(void) const
    {
      return m_capabilities;
    }

    ASYNC_MSG_MEMBERS(m_major,
                      m_minor,
                      m_reflector_id,
                      m_domain,
                      m_capabilities)

  private:
    std::uint16_t m_major;
    std::uint16_t m_minor;
    std::string   m_reflector_id;
    std::string   m_domain;
    std::uint32_t m_capabilities;
}; /* MsgFederationHelloAck */


#endif /* FEDERATION_MSG_INCLUDED */
