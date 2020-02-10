// Code from
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2018 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include "asio.hpp"
#include "tcp_server.h"
#include "client.h"

using asio::ip::tcp;

tcp_server::tcp_server(asio::io_context& io_context, short port, std::function<void(client *)> accept_callback)
: acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), accept_callback_(accept_callback)
{
    do_accept();
}

/* When a new TCP connection is made, createa a new client, 
 * assign it a new ID (just count up), call the accept_callback 
 * then wait for more connections
 */
void tcp_server::do_accept()
{
    acceptor_.async_accept(
            [this](std::error_code ec, tcp::socket socket)
            {
                if (!ec)
                {
                    client *c = new client(std::move(socket), current_id++);
                    accept_callback_(c);
                }

                do_accept();
            });
}

// Static instantiation
int tcp_server::current_id = 0;


