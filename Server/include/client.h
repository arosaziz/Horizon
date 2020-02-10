#ifndef CLIENT_H
#define CLIENT_H

// Code from
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2018 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <functional>
#include <string>
#include "asio.hpp"

class client;
//typedef void (*callback)(client *);

using asio::ip::tcp;

class client
    : public std::enable_shared_from_this<client>
{
public:
  client(asio::ip::tcp::socket socket, int id);
  // Gets data and puts it into the data buffer. MAY BE AN INCOMPLETE MESSAGE
  void get_data();
  void write_data(char *data, std::size_t length);
  void write_data(std::string data);
  int get_id();
  void disconnect_client();

  enum
  {
    max_length = 2048
  };
  char message[max_length];
  char buffer[max_length];
  std::string stream;
  int message_length;
  int newlines;
  std::string connected_spreadsheet;

  // callback callback_func;
  // callback disconnect_func;
  // std::function<callback> callback_func;
  // std::function<callback> disconnect_func;
  std::function<void(client *)> callback_func;
  std::function<void(client *)> disconnect_func;

private:
  asio::ip::tcp::socket socket_;
  int id_;
};

#endif
