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
#include <iostream>

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/fileappender.h>
#include <log4cxx/patternlayout.h>
#include <log4cxx/helpers/exception.h>

#include <Poco/File.h>
#include <Poco/Path.h>

#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "Configuration.h"
#include "Networking.h"

#define APPLICATION             "home-monitor"

#define LOGGING_PATH            "/var/log/" APPLICATION
#define CONFIGURATION_PATH      "/etc/opt/" APPLICATION
#define CONFIGURATION_FILENAME  APPLICATION ".xml"

#define CHANGE_TIMEOUT          120

#define SECONDS_TO_MICROSECONDS 1000000

using namespace std;

using Poco::File;
using Poco::Path;

typedef enum ManualMode
{
  ManualModeNone = 0,
  ManualModeWakeup,
  ManualModeShutdown
} ManualMode;

static log4cxx::LoggerPtr logger(log4cxx::Logger::getLogger(APPLICATION));

static bool abortRequested = false;

static void signalHandler(int signal)
{
  switch (signal)
  {
    case SIGTERM:
      LOG4CXX_INFO(logger, "Received SIGTERM --> terminating...");
      abortRequested = true;
      break;

    case SIGINT:
      LOG4CXX_INFO(logger, "Received SIGINT --> terminating...");
      abortRequested = true;
      break;

    default:
      LOG4CXX_WARN(logger, "Received signal " << signal);
      break;
  }
}

void printUsage()
{
  cout << APPLICATION " [OPTION]" << endl;
  cout << "\t-v, --verbose\tLog to standard output." << endl;
  cout << "\t-s, --shutdown\tShut the server down." << endl;
  cout << "\t-w, --wake\tWake the server up." << endl;
  cout << endl;
  cout << "Passing no option starts the daemon mode which monitors the network for activity of certain machines and either wakes the server up or shuts it down." << endl;
}

int main(int argc, char** argv)
{
  bool verboseLogging = false;
  ManualMode manualMode = ManualModeNone;

  // parse any command line options
  for (int argIndex = 1; argIndex < argc; ++argIndex)
  {
    if (argv[argIndex] == NULL)
    {
      printUsage();
      return 4;
    }

    std::string arg(argv[argIndex]);
    if (arg.compare("-v") == 0 || arg.compare("--verbose") == 0)
      verboseLogging = true;
    else if (arg.compare("-s") == 0 || arg.compare("--shutdown") == 0)
      manualMode = ManualModeShutdown;
    else if (arg.compare("-w") == 0 || arg.compare("--wakeup") == 0)
      manualMode = ManualModeWakeup;
    else
    {
      printUsage();
      return 4;
    }
  }

  Configuration config;

  // setup log4cxx logging
  log4cxx::LayoutPtr loggingLayout(new log4cxx::PatternLayout(config.GetLoggingPattern()));
  log4cxx::AppenderPtr loggingAppender(verboseLogging ?
    static_cast<log4cxx::Appender*>(new log4cxx::ConsoleAppender(loggingLayout)) :
    static_cast<log4cxx::Appender*>(new log4cxx::FileAppender(loggingLayout, LOGGING_PATH, true)));
  loggingAppender->setName(APPLICATION);
  try
  {
    log4cxx::BasicConfigurator::configure(loggingAppender);
    log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::toLevel(config.GetLoggingLevel()));
  }
  catch (log4cxx::helpers::Exception &e)
  {
    cout << "Failed to configure logging!" << endl;
  }

  LOG4CXX_INFO(logger, "Starting " APPLICATION "...");

  Path configPath(Poco::Path::current());
  configPath.append(CONFIGURATION_FILENAME);

  std::string configFileLocation = configPath.toString();
  File configFile(configFileLocation);
  if (!configFile.exists())
  {
    configPath = CONFIGURATION_PATH;
    configPath.append(CONFIGURATION_FILENAME);

    configFileLocation = configPath.toString();
    configFile = configFileLocation;
    if (!configFile.exists())
    {
      LOG4CXX_FATAL(logger, "Configuration file missing at " << configFileLocation << "!");
      return 1;
    }
  }

  LOG4CXX_INFO(logger, "Reading configuration from " << configFileLocation << "...");
  if (!config.Load(configFileLocation))
  {
    LOG4CXX_FATAL(logger, "Invalid configuration file at " << configFileLocation << "!");
    return 2;
  }

  // reset the logging configuration and apply the loaded settings
  loggingLayout = log4cxx::LayoutPtr(new log4cxx::PatternLayout(config.GetLoggingPattern()));
  log4cxx::Logger::getRootLogger()->getAppender(APPLICATION)->setLayout(loggingLayout);
  log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::toLevel(config.GetLoggingLevel()));

  Networking network(config.GetNetworkInterface());
  if (network.GetInterface().empty() ||
      network.GetIpAddress().empty() ||
      network.GetMacAddress().empty())
  {
    LOG4CXX_FATAL(logger, "Invalid network configuration!");
    return 3;
  }

  LOG4CXX_INFO(logger, "Network");
  LOG4CXX_INFO(logger, "\tInterface: " << network.GetInterface());
  LOG4CXX_INFO(logger, "\tMAC address: " << network.GetMacAddress());
  LOG4CXX_INFO(logger, "\tIP address: " << network.GetIpAddress());
  LOG4CXX_INFO(logger, "");

  Machine& server = config.GetServer();
  LOG4CXX_INFO(logger, "Server");
  LOG4CXX_INFO(logger, "\t" << server.GetName() << " (" << server.GetUsername() << "): " << server.GetMacAddress() << " / " << server.GetIpAddress());
  LOG4CXX_INFO(logger, "");

  // check if an option has been provided
  if (manualMode == ManualModeWakeup)
  {
    cout << "Waking up " << server.GetName() << "... " << flush;
    if (network.Wake(server))
    {
      cout << "working" << endl;
      return 0;
    }
    else
    {
      cout << "failed" << endl;
      return 5;
    }
  }
  else if (manualMode == ManualModeShutdown)
  {
    cout << "Shutting down " << server.GetName() << "... " << flush;
    if (network.Shutdown(server))
    {
      cout << "working" << endl;
      return 0;
    }
    else
    {
      cout << "failed" << endl;
      return 5;
    }
  }

  LOG4CXX_INFO(logger, "Ping");
  LOG4CXX_INFO(logger, "\tInterval: " << config.GetPingInterval());
  LOG4CXX_INFO(logger, "\tTimeout: " << static_cast<uint32_t>(config.GetPingTimeout()));
  LOG4CXX_INFO(logger, "");

  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = signalHandler;
  sigaction(SIGTERM, &action, NULL);
  sigaction(SIGINT, &action, NULL);

  std::vector<Machine>& machines = config.GetMachines();
  if (machines.empty())
  {
    LOG4CXX_FATAL(logger, "No machine definitions found!");
    return 1;
  }

  LOG4CXX_INFO(logger, "Machines (" << machines.size() << ")");
  for (std::vector<Machine>::const_iterator machine = machines.begin(); machine != machines.end(); ++machine)
    LOG4CXX_INFO(logger, "\t" << machine->GetName() << ": " << machine->GetMacAddress() << " / " << machine->GetIpAddress() << " (" << machine->GetTimeout() << "s)");
  LOG4CXX_INFO(logger, "");

  LOG4CXX_INFO(logger, "Files");
  LOG4CXX_INFO(logger, "\tConfiguration: " << configFileLocation);
  if (verboseLogging) {
    LOG4CXX_INFO(logger, "\tLogging: console");
  } else {
    LOG4CXX_INFO(logger, "\tLogging: " << LOGGING_PATH);
  }
  LOG4CXX_INFO(logger, "\tAlways On: " << config.GetAlwaysOnFile());

  LOG4CXX_INFO(logger, "");
  LOG4CXX_INFO(logger, "Monitoring the network for activity..."); 
  Poco::Timestamp lastPing;
  Poco::Timestamp lastChange;
  bool alwaysOn = false;
  File alwaysOnFile(config.GetAlwaysOnFile());

  while (!abortRequested)
  {
    // check if the always on file exists
    bool alwaysOnExists = alwaysOnFile.exists();
    if (alwaysOnExists != alwaysOn)
    {
      if (alwaysOnExists) {
        LOG4CXX_INFO(logger, "Always ON has been enabled");
      } else {
        LOG4CXX_INFO(logger, "Always ON has been disabled");
      }

      alwaysOn = alwaysOnExists;
    }

    // check if the server is online
    if (lastPing.elapsed() > config.GetPingInterval() * SECONDS_TO_MICROSECONDS)
    {
      lastPing.update();
      
      // ping the server
      std::vector<Machine> servers; servers.push_back(server);
      std::vector<Machine> serversAvailable = network.Ping(servers, config.GetPingTimeout());
      // check if the server hasn't been online for a while
      bool wasServerAvailable = server.IsOnline();
      if (serversAvailable.empty())
      {
        if (server.IsOnline())
        {
          if (server.GetLastOnline().elapsed() >= server.GetTimeout() * SECONDS_TO_MICROSECONDS)
            server.SetOnline(false);
        }
      }
      else
        server.SetOnline(true);

      if (server.IsOnline() != wasServerAvailable)
      {
        if (server.IsOnline()) {
          LOG4CXX_INFO(logger, server.GetName() << " is now available");
        } else {
          LOG4CXX_INFO(logger, server.GetName() << " is not available aynmore");
        }
      }

      // ping the machines
      std::vector<Machine> machinesAvailable = network.Ping(machines, config.GetPingTimeout());
      for (std::vector<Machine>::iterator machine = machines.begin(); machine != machines.end(); ++machine)
      {
        std::string machineIp = machine->GetIpAddress();
        bool available = false;
        for (std::vector<Machine>::const_iterator machineAvailable = machinesAvailable.begin(); machineAvailable != machinesAvailable.end(); ++machineAvailable)
        {
          if (machineIp.compare(machineAvailable->GetIpAddress()) == 0)
          {
            available = true;
            break;
          }
        }

        bool wasAvailable = machine->IsOnline();
        if (available)
          machine->SetOnline(true);
        else
        {
          // check if the machine hasn't been online for a while
          Poco::Timestamp::TimeDiff elapsed = machine->GetLastOnline().elapsed();
          if (elapsed >= machine->GetTimeout() * SECONDS_TO_MICROSECONDS)
            machine->SetOnline(false);
        }

        if (machine->IsOnline() != wasAvailable)
        {
          if (machine->IsOnline()) {
            LOG4CXX_INFO(logger, machine->GetName() << " is now available");
          } else {
            LOG4CXX_INFO(logger, machine->GetName() << " is not available aynmore");
          }
        }
      }
    }

    // check if any machine is online
    bool machineOnline = false;
    for (std::vector<Machine>::const_iterator machine = machines.begin(); machine != machines.end(); ++machine)
    {
      if (machine->IsOnline())
      {
        machineOnline = true;
        break;
      }
    }

    if (alwaysOn || lastChange.elapsed() >= CHANGE_TIMEOUT * SECONDS_TO_MICROSECONDS)
    {
      if ((alwaysOn || machineOnline) && !server.IsOnline())
      {
        LOG4CXX_INFO(logger, "Waking up " << server.GetName() << "...");
        if (network.Wake(server))
          lastChange.update();
        else
          LOG4CXX_ERROR(logger, "Waking up " << server.GetName() << " failed");
      }
      else if (!alwaysOn && !machineOnline && server.IsOnline())
      {
        LOG4CXX_INFO(logger, "Shutting down " << server.GetName() << "...");
        if (network.Shutdown(server))
          lastChange.update();
        else
          LOG4CXX_ERROR(logger, "Shutting down " << server.GetName() << " failed");
      }
    }

    sleep(1);
  }

  return 0;
}

