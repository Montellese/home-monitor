#pragma once
/*
 *  Copyright (C) 2015 Sascha Montellese <sascha.montellese@gmail.com>
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>

#include <stdint.h>

#include "Machine.h"

namespace Poco
{
  namespace XML
  {
    class Element;
  }
}

class Configuration
{
  public:
    Configuration()
     : m_loggingLevel("INFO"),
       m_loggingPattern("%d{dd.MM.yyyy HH:mm:ss.SSS} %-5p [%c] %m%n"),
       m_networkInterface(),
       m_pingTimeout(10),
       m_pingInterval(30)
    { }

    bool Load(const std::string& file);

    const std::string& GetLoggingLevel() const { return m_loggingLevel; }
    const std::string& GetLoggingPattern() const { return m_loggingPattern; }

    const std::string& GetNetworkInterface() const { return m_networkInterface; }

    uint8_t GetPingTimeout() const { return m_pingTimeout; }
    uint16_t GetPingInterval() const { return m_pingInterval; }

    Machine& GetServer() { return m_server; }
    std::vector<Machine>& GetMachines() { return m_machines; }

    const std::string& GetAlwaysOnFile() const { return m_alwaysOnFile; }

  private:
    static bool getString(const Poco::XML::Element* element, std::string& valuu);
    static bool parseMachine(const Poco::XML::Element* machineElement, bool needsCredentials, Machine& machine);

    std::string m_loggingLevel;
    std::string m_loggingPattern;

    std::string m_networkInterface;

    uint8_t m_pingTimeout;
    uint16_t m_pingInterval;

    Machine m_server;
    std::vector<Machine> m_machines;

    std::string m_alwaysOnFile;
};

