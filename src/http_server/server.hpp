//
// Copyright Copyright 2009-2021, AMT – The Association For Manufacturing Technology (“AMT”)
// All rights reserved.
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//

#pragma once

#include <boost/bind/bind.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http/status.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <regex>
#include "configuration/config_options.hpp"

#include "file_cache.hpp"
#include "response.hpp"
#include "routing.hpp"
#include "utilities.hpp"
#include "session.hpp"

namespace mtconnect
{
  namespace http_server
  {
    class Server
    {
    public:
      Server(boost::asio::io_context &context, unsigned short port = 5000, const std::string &inter = "0.0.0.0", const ConfigOptions &options = {})
      : m_context(context), m_port(port), m_acceptor(context)
      {
        if (inter.empty())
        {
          m_address = boost::asio::ip::make_address("0.0.0.0");
        }
        else
        {
          m_address = boost::asio::ip::make_address(inter);
        }
        const auto fields = GetOption<StringList>(options, configuration::HttpHeaders);
        m_errorFunction = [](SessionPtr session, status st, const std::string &msg) {
          Response response(st, msg, "text/plain");
          session->writeResponse(response);
          return true;
        };
      }

      // Start the http server
      void start();

      // Shutdown
      void stop(){m_run=false;};

      void listen();

      void setHttpHeaders(const StringList &fields)
      {
        m_fields = fields;
      }
      
      auto getPort() const { return m_port; }

      // PUT and POST handling
      bool isListening() const { return m_listening; }
      bool arePutsAllowed() const { return m_allowPuts; }
      bool allowPutFrom(const std::string &host);
      void allowPuts(bool allow = true)
      {
        m_allowPuts = allow;
      }
      bool isPutAllowedFrom(boost::asio::ip::address &addr) const
      {
        return m_allowPutsFrom.find(addr) != m_allowPutsFrom.end();
      }

      bool dispatch(RequestPtr request)
      {
        for (auto &r : m_routings)
        {
          if (r.matches(request))
            return true;
        }

        return false;
      }

      void accept(boost::system::error_code ec, boost::asio::ip::tcp::socket soc);
      void fail(boost::system::error_code ec, char const *what);
      void addRouting(const Routing &routing) { m_routings.emplace_back(routing); }
      void setErrorFunction(const ErrorFunction &func) { m_errorFunction = func; }

    protected:
      boost::asio::io_context &m_context;
      
      boost::asio::ip::address m_address;
      unsigned short m_port{5000};

      bool m_run{false};
      bool m_enableSSL{false};
      bool m_listening{false};
      
      ConfigOptions m_options;
      // Put handling controls
      bool m_allowPuts{false};
      std::set<boost::asio::ip::address> m_allowPutsFrom;
      std::list<Routing> m_routings;
      std::unique_ptr<FileCache> m_fileCache;
      ErrorFunction m_errorFunction;
      StringList m_fields;
      
      boost::asio::ip::tcp::acceptor m_acceptor;
    };
  }  // namespace http_server
}  // namespace mtconnect
