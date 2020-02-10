/* This JSON_Message class used to build JSON_message strings on the server side
 *
 * Aros Aziz
 * April 11, 2019
 */
#ifndef JSON_MESSAGE_H
#define JSON_MESSAGE_H

#include "command.h"
#include "spreadsheet.h"
#include <unordered_map>

/**
 * Enum to hold the error type for our error message method 
 **/
enum ERROR_TYPE
{
  INVALID_USER_PASS = 1,
  CIRC_DEP = 2
};

namespace JSON_message
{
command *get_type(char const *const data);
std::string full_send_message(const spreadsheet &s);
std::string full_send_message(const spreadsheet &s, const std::string &cell_name);
std::string error_message(ERROR_TYPE, std::string bad_cell);
std::string spreadsheet_list_message(std::vector<std::string> list);
std::string save_spreadsheet(spreadsheet &s);
spreadsheet open_spreadsheet(const std::string &filename);
std::string state_message(std::unordered_map<std::string, std::string> users);
std::string send_message(std::string message);
std::unordered_map<std::string, std::string> deserialize_users();

} // namespace JSON_message
#endif
