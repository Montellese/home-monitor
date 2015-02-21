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

#include <fstream>

#include <log4cxx/logger.h>

#include <Poco/AutoPtr.h>
#include <Poco/NumberParser.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
#include <Poco/SAX/InputSource.h>
#include <Poco/SAX/SAXException.h>

#include "Configuration.h"

using namespace Poco;

static log4cxx::LoggerPtr logger(log4cxx::Logger::getLogger("Configuration"));

bool Configuration::getString(const XML::Element* element, std::string& value)
{
  if (element == NULL)
    return false;

  switch (element->nodeType())
  {
    case XML::Node::TEXT_NODE:
    {
      value = element->nodeValue();
      return true;
    }

    case XML::Node::ELEMENT_NODE:
    {
      XML::Node* child = element->firstChild();
      if (child == NULL || child->nodeType() != XML::Node::TEXT_NODE)
        return false;

      value = child->nodeValue();
      return true;
    }
  }

  return false;
}

bool Configuration::Load(const std::string& file)
{
  if (file.empty())
  {
    LOG4CXX_ERROR(logger, "Trying to load invalid file");
    return false;
  }

  LOG4CXX_DEBUG(logger, "Loading configuration from " << file << "...");
  std::ifstream fileStream(file.c_str());
  XML::InputSource fileSource(fileStream);

  XML::DOMParser xmlParser;
  AutoPtr<XML::Document> xmlDoc;
  LOG4CXX_DEBUG(logger, "Parsing the XML document using a DOM parser...");
  try
  {
    xmlDoc = xmlParser.parse(&fileSource); 
  }
  catch (Poco::XML::SAXParseException &e)
  {
    LOG4CXX_ERROR(logger, "Failed to parse XML document at line " << e.getLineNumber() << " column " << e.getColumnNumber() << ":");
    LOG4CXX_ERROR(logger, e.displayText());
    return false;
  }
 if (xmlDoc.isNull())
    return false;

  XML::Element* root = xmlDoc->documentElement();
  if (root == NULL || root->nodeName().compare("settings") != 0)
  {
    LOG4CXX_ERROR(logger, "No or unexpected XML root element found");
    return false;
  }

  XML::Element* loggingElement = root->getChildElement("logging");
  if (loggingElement != NULL)
  {
    XML::Element* loggingLevelElement = loggingElement->getChildElement("level");
    getString(loggingLevelElement, m_loggingLevel);

    XML::Element* loggingPatternElement = loggingElement->getChildElement("pattern");
    getString(loggingPatternElement, m_loggingPattern);
  }

  XML::Element* networkElement = root->getChildElement("network");
  if (networkElement == NULL)
  {
    LOG4CXX_ERROR(logger, "Missing <network> tag");
    return false;
  }

  XML::Element* networkInterfaceElement = networkElement->getChildElement("interface");
  if (!getString(networkInterfaceElement, m_networkInterface) || m_networkInterface.empty())
  {
    LOG4CXX_ERROR(logger, "Invalid <interface> element of type " << networkInterfaceElement->nodeType());
    return false;
  }

  XML::Element* pingElement = root->getChildElement("ping");
  if (pingElement != NULL)
  {
    XML::Element* pingIntervalElement = pingElement->getChildElement("interval");
    std::string strPingInterval;
    if (getString(pingIntervalElement, strPingInterval))
    {
      try
      {
        m_pingInterval = static_cast<uint16_t>(NumberParser::parseUnsigned(strPingInterval));
      }
      catch (SyntaxException &e)
      {
        LOG4CXX_WARN(logger, "Invalid <ping><interval> configuration value");
      }
    }

    XML::Element* pingTimeoutElement = pingElement->getChildElement("timeout");
    std::string strPingTimeout;
    if (getString(pingTimeoutElement, strPingTimeout))
    {
      try
      {
        m_pingTimeout = static_cast<uint8_t>(NumberParser::parseUnsigned(strPingTimeout));
      }
      catch (SyntaxException &e)
      {
        LOG4CXX_WARN(logger, "Invalid <ping><timeout> configuration value");
      }
    }
  }

  XML::Element* serverElement = root->getChildElement("server");
  if (serverElement == NULL)
  {
    LOG4CXX_ERROR(logger, "Missing <server> tag");
    return false;
  }

  if (!parseMachine(serverElement, true, m_server))
  {
    LOG4CXX_ERROR(logger, "Invalid <server> tag");
    return false;
  }

  XML::Element* machinesElement = root->getChildElement("machines");
  if (machinesElement == NULL)
  {
    LOG4CXX_ERROR(logger, "Missing <machines> tag");
    return false;
  }

  XML::Element* machineElement = machinesElement->getChildElement("machine");
  while (machineElement != NULL)
  {
    if (machineElement->nodeType() == XML::Node::ELEMENT_NODE)
    {
      if (machineElement->nodeName().compare("machine") == 0)
      {
        Machine machine;
        if (parseMachine(machineElement, false, machine))
          m_machines.push_back(machine);
        else
          LOG4CXX_ERROR(logger, "Invalid <machine> tag");
      }
      else if (!machineElement->nodeName().empty())
        LOG4CXX_WARN(logger, "Unexpected <" << machineElement->nodeName() << "> element, expected <machine>");
    }
 
    machineElement = static_cast<XML::Element*>(machineElement->nextSibling());
  }

  XML::Element* filesElement = root->getChildElement("files");
  if (filesElement != NULL)
  {
    XML::Element* alwaysOnFile = filesElement->getChildElement("alwayson");
    getString(alwaysOnFile, m_alwaysOnFile);
  }

  return true;
}

bool Configuration::parseMachine(const Poco::XML::Element* machineElement, bool needsCredentials, Machine& machine)
{
  XML::Element* nameElement = machineElement->getChildElement("name");
  XML::Element* macAddressElement = machineElement->getChildElement("mac");
  XML::Element* ipAddressElement = machineElement->getChildElement("ip");
  XML::Element* timeoutElement = machineElement->getChildElement("timeout");

  std::string name, macAddress, ipAddress, strTimeout;
  if (!getString(nameElement, name) || name.empty() ||
      !getString(macAddressElement, macAddress) || macAddress.empty() ||
      !getString(ipAddressElement, ipAddress) || ipAddress.empty() ||
      !getString(timeoutElement, strTimeout) || strTimeout.empty())
  {
    LOG4CXX_ERROR(logger, "Missing or invalid <machine><name>/<mac>/<ip>/<timeout> tag");
    return false;
  }

  std::string username, password;
  if (needsCredentials)
  {
    XML::Element* usernameElement = machineElement->getChildElement("username");
    XML::Element* passwordElement = machineElement->getChildElement("password");

    if (!getString(usernameElement, username) || username.empty() ||
        !getString(passwordElement, password))
    {
      LOG4CXX_ERROR(logger, "Missing or invalid <machine><username> and/or <machine><password> tag");
      return false;
    }
  }

  try
  {
     machine =  Machine(name, macAddress, ipAddress, username, password, static_cast<uint16_t>(NumberParser::parseUnsigned(strTimeout)));
     return true;
  }
  catch (SyntaxException &e)
  {
    LOG4CXX_ERROR(logger, "Invalid <machine><timeout> tag");
    return false;
  }

  return false;
}

