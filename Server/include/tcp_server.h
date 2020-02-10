#ifndef TCP_SERVER_H
#define TCP_SERVER_H

//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2018 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <functional>
#include "asio.hpp"
#include "client.h"


// typedef void (*callback)(client *);

class tcp_server
{
public:
  tcp_server(asio::io_context &io_context, short port, std::function<void(client *)> accept_callback);

private:
  void do_accept();

  asio::ip::tcp::acceptor acceptor_;
  std::function<void(client *)> accept_callback_;
  static int current_id;
};

#endif
