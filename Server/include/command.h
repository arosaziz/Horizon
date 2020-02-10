#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>

class command
{
protected:
  std::string type;

public:
  command(
      const std::string &type);

  ~command();

  /// <summary>
  /// Returns the type of the command object as a string. Inherited to all child classes
  /// </summary>
  const std::string get_type() const;
};

class open_command : public command
{
private:
  std::string name;
  std::string username;
  std::string password;

public:
  /// <summary>
  /// Constructor for an Open command.
  /// </summary>
  /// <param name="name">The filename of the spreadsheet to open</param>
  /// <param name="username">The name of the user attempting to open the spreadsheet</param>
  /// <param name="password">The password of the user attempting to open the spreadsheet</param>
  open_command(
      const std::string &name,
      const std::string &username,
      const std::string &password);

  /// <summary>
  /// Destructor for an Open command.
  /// </summary>
  ~open_command();

  /// <summary>
  /// Returns the name of the spreadsheet to be opened as a string
  /// </summary>
  const std::string get_name() const;
  /// <summary>
  /// Returns the name of the user attempting to open the spreadsheet as a string
  /// </summary>
  const std::string get_username() const;
  /// <summary>
  /// Returns the password of the user attempting to open the spreadsheet as a string
  /// </summary>
  const std::string get_password() const;
};

class edit_command : public command
{
private:
  std::string cell;
  std::string value;
  std::vector<std::string> dependencies;

public:
  /// <summary>
  /// Constructor for an Edit command.
  /// </summary>
  /// <param name="cell">The name of the cell to edit on the spreadsheet</param>
  /// <param name="value">The "value" (technically contents) to set the given cell to</param>
  /// <param name="dependencies">A list of all direct dependents of the given cell as a vector </param>
  edit_command(
      const std::string &cell,
      const std::string &value,
      const std::vector<std::string> &dependencies);

  /// <summary>
  /// Destructor for an Edit command.
  /// </summary>
  ~edit_command();

  /// <summary>
  /// Returns the name of the cell that is to be modified.
  /// </summary>
  const std::string get_cell() const;
  /// <summary>
  /// Returns the "value" (contents) that the cell will be set to when modified.
  /// </summary>
  const std::string get_value() const;
  /// <summary>
  /// Returns a vector containing all of the direct dependents the cell will have once modified.
  /// </summary>
  const std::vector<std::string> get_dependencies() const;
};

class undo_command : public command
{
public:
  /// <summary>
  /// Constructor for an Edit command.
  /// </summary>
  undo_command();

  /// <summary>
  /// Destructor for an Edit command.
  /// </summary>
  ~undo_command();
};

class revert_command : public command
{
private:
  std::string cell;

public:
  /// <summary>
  /// Constructor for an Edit command.
  /// <param name="cell">The name of the cell to revert on the spreadsheet</param>
  /// </summary>
  revert_command(
      const std::string &cell);

  /// <summary>
  /// Destructor for an Edit command.
  /// </summary>
  ~revert_command();

  /// <summary>
  /// Returns the name of the cell that is to be reverted.
  /// </summary>
  const std::string get_cell() const;
};

class admin_command : public command
{
public:
  /// <summary>
  /// Constructor for an admin command.
  /// </summary>
  admin_command();
  ~admin_command();
};

class close_command : public command
{
public:
  /// <summary>
  /// Constructor for a close command.
  /// </summary>
  close_command();
  ~close_command();
};

class user_command : public command
{
private:
  // Order possibilities = new, delete, change
  std::string order;
  std::string username;
  std::string password;

public:
  /// <summary>
  /// Constructor for an Edit command.
  /// <param name="order">The action for this command - new (new user), delete (delete user), change (change user password)</param>
  /// <param name="username">The username of the client</param>
  /// <param name="password">The password of the client</param>
  /// </summary>
  user_command(std::string order, std::string username, std::string password);
  ~user_command();
  std::string get_order();
  std::string get_username();
  std::string get_password();
};

class sheet_command : public command
{
private:
  // Order possibilities = new, delete
  std::string order;
  std::string name;

public:
  /// <summary>
  /// Constructor for an Edit command.
  /// <param name="order">The action for this command - new (new spreadsheet), delete (delete spreadsheet)</param>
  /// <param name="name">Name of the sheet</param>
  /// </summary>
  sheet_command(std::string order, std::string name);
  ~sheet_command();
  std::string get_order();
  std::string get_name();
};

#endif
