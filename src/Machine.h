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

#include <Poco/Timestamp.h>

class Machine
{
  public:
    Machine()
    { }
    Machine(const std::string& name,
            const std::string& macAddress,
            const std::string& ipAddress,
            const std::string& username,
            const std::string &password,
            uint16_t timeout)
      : m_name(name),
        m_macAddress(macAddress),
        m_ipAddress(ipAddress),
        m_username(username),
        m_password(password),
        m_timeout(timeout),
        m_online(false),
        m_lastOnline()
    { }

    const std::string& GetName() const { return m_name; }
    const std::string& GetMacAddress() const { return m_macAddress; }
    const std::string& GetIpAddress() const { return m_ipAddress; }
    const std::string& GetUsername() const { return m_username; }
    const std::string& GetPassword() const { return m_password; }
    const uint16_t GetTimeout() const { return m_timeout; }

    const bool IsOnline() const { return m_online; }
    void SetOnline(bool online)
    {
      m_online = online;
      if (m_online)
        m_lastOnline.update();
    }

    const Poco::Timestamp& GetLastOnline() const { return m_lastOnline; }

  private:
    std::string m_name;
    std::string m_macAddress;
    std::string m_ipAddress;
    std::string m_username;
    std::string m_password;
    uint16_t m_timeout;
    bool m_online;
    Poco::Timestamp m_lastOnline;
};

