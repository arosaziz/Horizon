#ifndef SPREADSHEET_SERVER_H
#define SPREADSHEET_SERVER_H

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif

#define SAVE_INTERVAL 5

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <mutex>
#include <vector>
#include "spreadsheet.h"
#include "asio.hpp"
#include "tcp_server.h"
#include "command.h"

class spreadsheet_server
{
private:
  tcp_server *server;
  std::unordered_map<std::string, spreadsheet> sheets;
  std::unordered_map<std::string, std::unordered_set<client *>> sprd_conns;
  // This will probably have to be changed to a map later on
  // so that we can track what client is connected to what spreadsheet
  std::unordered_set<client *> clients;
  // Usernames mapped to passwords (security is an issue but we're not concerned)
  std::unordered_map<std::string, std::string> logins;
  std::mutex lock;
  std::mutex io_lock;
  bool is_running;
  std::thread saver_thread;
  client *admin;

  // Callbacks will be prefaced with handle_
  asio::io_context io_context;
  void handle_first_contact(client *c);
  void handle_client_disconnect(client *c);
  void handle_client_login(client *c);
  void handle_edits(client *c);
  void handle_admin(client *c);
  void handle_admin_disconnect(client *c);

  // Non-callbacks
  void save_sprd_names();
  bool check_login(std::string username, std::string password);
  std::vector<std::string> get_spreadsheet_names();
  void open_all_spreadsheets();
  void modify_user(user_command *cmd);
  bool modify_sheets(sheet_command *cmd);
  void save_logins();
  void shutdown_server();

public:
  spreadsheet_server();
  spreadsheet_server(int port);
  ~spreadsheet_server();
  void start();
  void save_spreadsheets();
  bool currently_running() const;

  static void spreadsheet_saver(spreadsheet_server *s);
};

#endif