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
#include "client.h"

using asio::ip::tcp;

client::client(tcp::socket socket, int id)
    : socket_(std::move(socket)), id_(id)
{
    message_length = 0;
    newlines = 0;
}

void client::get_data()
{
    socket_.async_read_some(asio::buffer(this->message, max_length),
                            [this](std::error_code ec, std::size_t length) {
                                if (!ec)
                                {
                                    char buf[2048];
                                    snprintf(buf, length, message);
                                    buf[length] = 0;
                                    std::cout << "recieved id: " << this->id_
                                              << " w/ length: " << length << ": " << buf << std::endl;
                                    message_length = length;

                                    for(int i = length; i < max_length; i++)
                                        message[i] = '\0';
                                    
                                    callback_func(this);
                                }
                                else
                                {
                                    disconnect_func(this);
                                    // Now, provided there are no more references or pointers to the socket, the
                                    // socket will fall out of scope and the destructor will clean everything up
                                }
                            });
}

// void client::get_data()
// {
//     socket_.async_read_some(asio::buffer(this->buffer, max_length),
//                             [this](std::error_code ec, std::size_t length) {
//                                 if (!ec)
//                                 {
//                                     // char buf[2048];
//                                     // snprintf(buf, length, message);
//                                     // buf[length] = 0;
//                                     // std::cout << buf << std::endl;

//                                     // If the message recieved filled the buffer
//                                     if (length == max_length)
//                                     {
//                                         stream.append(buffer);
//                                         this->get_data();
//                                         return;
//                                     }

//                                     // Check and make sure the message ends in two newlines
//                                     if(buffer[max_length - 2] != '\n')
//                                         newlines++;
//                                     if(buffer[max_length - 1] != '\n')
//                                         newlines++;

//                                     if (newlines == 2)
//                                     {
//                                         // Didn't end in two newlines, add the data to the message, then get more data
//
//                                         this->get_data();
//                                     }
//                                     else
//                                     {
//                                         message_length = length;
//                                         callback_func(this);
//                                     }
//                                 }
//                                 else
//                                 {
//                                     disconnect_func(this);
//                                     // Now, provided there are no more references or pointers to the socket, the
//                                     // socket will fall out of scope and the destructor will clean everything up
//                                 }
//                             });
// }

void client::write_data(char *data, std::size_t length)
{
    asio::async_write(socket_, asio::buffer(data, length),
                      [this](std::error_code ec, std::size_t length) {
                          if (!ec)
                          {
                              // do nothing - no callback
                              // do_read();
                          }
                          else
                          {
                              this->disconnect_client();
                              // Now, provided there are no more references or pointers to the socket, the
                              // socket will fall out of scope and the destructor will clean everything up
                          }
                      });
}

void client::write_data(std::string data)
{
    // std::cout << "data: " << data << std::endl;

    asio::async_write(socket_, asio::buffer(data),
                      [this](std::error_code ec, std::size_t length) {
                          if (!ec)
                          {
                              // do nothing - no callback
                              // do_read();
                          }
                          else
                          {
                              this->disconnect_client();
                              // Now, provided there are no more references or pointers to the socket, the
                              // socket will fall out of scope and the destructor will clean everything up
                          }
                      });
}

/*
 * Call the disconnect callback function
 * then free the client.
 */
void client::disconnect_client()
{
    disconnect_func(this);
    delete (this);
}

int client::get_id()
{
    return this->id_;
}
