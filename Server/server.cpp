// Code from
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2018 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif

#include <cstdlib>
#include <iostream>
#include <string.h>
#include <memory>
#include <utility>
#include "asio.hpp"
#include "tcp_server.h"
#include "client.h"
#include "JSON_message.h"
#include "spreadsheet_server.h"

using asio::ip::tcp;

void accept_client(client *c);
void echo(client *c);
void handle_client_disconnect(client *c);
void send_server_message(client *c);

client *c_;

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    spreadsheet_server server;
    // Create our spreadsheet_server
    // start it (blocking)
    server.start();
  }
  else
  {
    spreadsheet_server server (std::atoi(argv[1])); 
    server.start();
  }

  std::cout << "SERVER STOPPED. SHUTTING DOWN" << std::endl;
  

  return 0;
}
