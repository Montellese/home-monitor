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

#include <iostream>

#include <crafter.h>

#include <libssh/libssh.h>

#include <log4cxx/logger.h>

#include <Poco/NumberParser.h>
#include <Poco/StringTokenizer.h>

#include "Networking.h"
#include "Machine.h"

using namespace std;

static log4cxx::LoggerPtr logger(log4cxx::Logger::getLogger("Networking"));

Networking::Networking(const std::string& interface)
  : m_interface(interface),
    m_ip(),
    m_mac()
{
  m_ip = Crafter::GetMyIP(m_interface);
  m_mac = Crafter::GetMyMAC(m_interface);
}

std::vector<Machine> Networking::Ping(const std::vector<Machine>& machines, uint8_t timeout)
{
  std::vector<Machine> available;
  if (machines.empty())
    return available;

  // prepare an IP header
  Crafter::IP ipHeader;
  ipHeader.SetSourceIP(m_ip);

  // prepare an ICMP header
  Crafter::ICMP icmpHeader;
  icmpHeader.SetType(Crafter::ICMP::EchoRequest);
  icmpHeader.SetPayload("home-monitor");

  // go through all machines
  std::map<std::string, Machine> machineMap;
  std::vector<Crafter::Packet*> pingPackets;
  for (std::vector<Machine>::const_iterator machine = machines.begin(); machine != machines.end(); ++machine)
  {
    std::string machineIp = machine->GetIpAddress();
    machineMap.insert(std::make_pair(machineIp, *machine));

    ipHeader.SetDestinationIP(machineIp);
    icmpHeader.SetIdentifier(Crafter::RNG16());

    pingPackets.push_back(new Crafter::Packet(ipHeader / icmpHeader));
    LOG4CXX_DEBUG(logger, "Preparing to ping " << machine->GetName() << " (" << machineIp << ")...");
  }

  // send the ICMP packets and wait for responses
  std::vector<Crafter::Packet*> pongPackets(pingPackets.size());
  LOG4CXX_DEBUG(logger, "Pinging " << pingPackets.size() << " machines on " << m_interface << " with a timeout of " << static_cast<uint32_t>(timeout) << " seconds...");
  Crafter::SendRecv(pingPackets.begin(), pingPackets.end(), pongPackets.begin(), m_interface, timeout, 1, 48);
  LOG4CXX_DEBUG(logger, "Ping response received for " << pongPackets.size() << " of " << pingPackets.size() << " machines.");

  // go through all the received responses
  for (std::vector<Crafter::Packet*>::const_iterator pong = pongPackets.begin(); pong != pongPackets.end(); ++pong)
  {
    Crafter::Packet* pongPacket = *pong;
    if (pongPacket == NULL)
    {
      LOG4CXX_TRACE(logger, "Invalid PONG packet received");
      continue;
    }

    Crafter::ICMP* icmpLayer = pongPacket->GetLayer<Crafter::ICMP>();
    if (icmpLayer == NULL || icmpLayer->GetType() != Crafter::ICMP::EchoReply)
    {
      LOG4CXX_WARN(logger, "PONG packet not of type ICMP EchoReply received");
      continue;
    }

    Crafter::IP* ipLayer = pongPacket->GetLayer<Crafter::IP>();
    if (ipLayer == NULL)
    {
      LOG4CXX_WARN(logger, "PONG packet without an IP layer received");
      continue;
    }

    std::string machineIp = ipLayer->GetSourceIP();
    if (machineIp.empty())
    {
      LOG4CXX_WARN(logger, "PONG packet with empty source IP address received");
      continue;
    }

    std::map<std::string, Machine>::const_iterator machine = machineMap.find(machineIp);
    if (machine == machineMap.end())
    {
      LOG4CXX_WARN(logger, "PONG packet for unknown machine at " << machineIp << " received");
      continue;
    }

    available.push_back(machine->second);
    LOG4CXX_DEBUG(logger, "PONG packet for " << machine->second.GetName() << " (" << machineIp << ") received");
  }

  for (std::vector<Crafter::Packet*>::const_iterator ping = pingPackets.begin(); ping != pingPackets.end(); ++ping)
    delete *ping;

  for (std::vector<Crafter::Packet*>::const_iterator pong = pongPackets.begin(); pong != pongPackets.end(); ++pong)
    delete *pong;

  return available;
}

bool Networking::Wake(const Machine& machine)
{
  const std::string& mac = machine.GetMacAddress();
  if (mac.empty())
  {
    LOG4CXX_ERROR(logger, "Invalid MAC address (" << mac << ")");
    return false;
  }

  std::vector<uint8_t> macParts;
  Poco::StringTokenizer tokenizer(mac, ":", Poco::StringTokenizer::TOK_TRIM);
  if (tokenizer.count() != 6)
  {
    LOG4CXX_ERROR(logger, "Invalid MAC address (" << mac << ")");
    return false;
  }

  for (Poco::StringTokenizer::Iterator token = tokenizer.begin(); token != tokenizer.end(); ++token)
  {
    try
    {
      unsigned int tok = Poco::NumberParser::parseHex(*token);
      if (tok > 255)
      {
        LOG4CXX_ERROR(logger, "Invalid MAC address (" << mac << ")");
        return false;
      }

      macParts.push_back(static_cast<uint8_t>(tok));
    }
    catch (Poco::SyntaxException &e)
    {
      LOG4CXX_ERROR(logger, "Invalid MAC address (" << mac << ")");
      return false;
    }
  }

  // prepare the payload of the ethernet Magic PAcket
  // 6x 0XFF
  // 16 * 6x MAC address
  uint8_t payload[102];
  memset(payload, 0xFF, 6);
  size_t index = 6;
  for (int num = 0; num < 16; ++num)
  {
    for (std::vector<uint8_t>::const_iterator it = macParts.begin(); it != macParts.end(); ++it)
    {
      payload[index] = *it;
      ++index;
    }
  }

  Crafter::Ethernet ether;
  ether.SetSourceMAC(m_mac);
  ether.SetDestinationMAC(mac);
  ether.SetPayload(payload, sizeof(payload));

  Crafter::Packet packet = ether;

  LOG4CXX_DEBUG(logger, "Sending Wake-on-LAN magic packet to " << mac << "...");
  return packet.Send(m_interface) > 0;
}

bool Networking::Shutdown(const Machine& machine)
{
  const std::string& ip = machine.GetIpAddress();
  const std::string& username = machine.GetUsername();
  const std::string& password = machine.GetPassword();
  if (ip.empty() || username.empty())
  {
    LOG4CXX_ERROR(logger, "Missing IP address (" << ip << ") or username (" << username << ")");
    return false;
  }

  ssh_session ssh;
  ssh_channel channel;
  
  ssh = ssh_new();
  if (ssh == NULL)
  {
    LOG4CXX_ERROR(logger, "libssh error (ssh_new)");
    return false;
  }

  // set the remote host and the user to be used
  ssh_options_set(ssh, SSH_OPTIONS_HOST, ip.c_str());
  ssh_options_set(ssh, SSH_OPTIONS_USER, username.c_str());

  LOG4CXX_DEBUG(logger, "Connecting to " << machine.GetName() << " at " << ip << " as " << username << " over SSH...");
  int rc = ssh_connect(ssh);
  if (rc != SSH_OK)
  {
    LOG4CXX_ERROR(logger, "Unable to connect to " << machine.GetName() << " at " << ip << " as " << username << " (" << rc << ")");
    goto session_free;
  }

  if (!password.empty())
  {
    LOG4CXX_DEBUG(logger, "Authenticating as " << username << "on " << machine.GetName() << " at " << ip << " over SSH...");
    rc = ssh_userauth_password(ssh, NULL, password.c_str());
    if (rc != SSH_AUTH_SUCCESS)
    {
      LOG4CXX_ERROR(logger, "Authentication as " << username << " failed on " << machine.GetName() << " at " << ip << " (" << rc << ")");
      goto session_disconnect;
    }
  }

  channel = ssh_channel_new(ssh);
  if (channel == NULL)
  {
    LOG4CXX_ERROR(logger, "libssh error (ssh_channel_new)");
    goto session_disconnect;
  }

  rc = ssh_channel_open_session(channel);
  if (rc != SSH_OK)
  {
    LOG4CXX_ERROR(logger, "Unable to open a new SSH channel on " << machine.GetName() << " at " << ip << " (" << rc << ")");
    goto channel_free;
  }

  LOG4CXX_DEBUG(logger, "Executing 'shutdown -h now' on " << machine.GetName() << " at " << ip << " as " << username << "...");
  rc = ssh_channel_request_exec(channel, "shutdown -h now");
  if (rc != SSH_OK)
    LOG4CXX_ERROR(logger, "Failed to execute 'shutdown -h now' on " << machine.GetName() << " at " << ip << " as " << username << " (" << rc << ")");

  channel_close(channel);

channel_free:
  ssh_channel_free(channel);
  
session_disconnect:
  ssh_disconnect(ssh);

session_free:
  ssh_free(ssh);

  return rc == SSH_OK;
}

