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
#include <stdint.h>

class Machine;

class Networking
{
  public:
    Networking(const std::string& interface);

    const std::string& GetInterface() const { return m_interface; }
    const std::string& GetIpAddress() const { return m_ip; }
    const std::string& GetMacAddress() const {return m_mac; }

    std::vector<Machine> Ping(const std::vector<Machine>& machines, uint8_t timeout);

    bool Wake(const Machine& machine);
    bool Shutdown(const Machine& machine);

  private:
    std::string m_interface;
    std::string m_ip;
    std::string m_mac;
};

