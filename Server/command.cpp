#include "command.h"

command::command(const std::string &type)
{
	this->type = type;
}

command::~command()
{
}

const std::string command::get_type() const
{
	return type;
}

// ======== Open ========
open_command::open_command(const std::string &name, const std::string &username, const std::string &password)
	: command("open")
{
	this->type = "open";
	this->name = name;
	this->username = username;
	this->password = password;
}

open_command::~open_command()
{
}

const std::string open_command::get_name() const
{
	return name;
}

const std::string open_command::get_username() const
{
	return username;
}

const std::string open_command::get_password() const
{
	return password;
}

// ======== Edit ========
edit_command::edit_command(const std::string &cell, const std::string &value, const std::vector<std::string> &dependencies)
	: command("edit")
{
	this->type = "edit";
	this->cell = cell;
	this->value = value;
	this->dependencies = dependencies;
}

edit_command::~edit_command()
{
}

const std::string edit_command::get_cell() const
{
	return cell;
}

const std::string edit_command::get_value() const
{
	return value;
}

const std::vector<std::string> edit_command::get_dependencies() const
{
	return dependencies;
}

// ======== Undo ========
undo_command::undo_command()
	: command("undo")
{
	this->type = "undo";
}

undo_command::~undo_command()
{
}

// ======== Revert ========
revert_command::revert_command(const std::string &cell)
	: command("revert")
{
	this->type = "revert";
	this->cell = cell;
}

revert_command::~revert_command()
{
}

const std::string revert_command::get_cell() const
{
	return cell;
}

// ======== Admin ========
admin_command::admin_command()
	: command("admin")
{
}

admin_command::~admin_command()
{
}

// ======== Close ========
close_command::close_command()
	: command("close")
{
}

close_command::~close_command()
{
}

// ======== User ========
user_command::user_command(std::string order, std::string username, std::string password)
	: command("user")
{
	this->order = order;
	this->username = username;
	this->password = password;
}

user_command::~user_command()
{
}

std::string user_command::get_order()
{
	return order;
}
std::string user_command::get_username()
{
	return username;
}
std::string user_command::get_password()
{
	return password;
}

// ======== Sheet ========
sheet_command::sheet_command(std::string order, std::string name)
	: command("sheet")
{
	this->order = order;
	this->name = name;
}

sheet_command::~sheet_command()
{
}

std::string sheet_command::get_order()
{
	return order;
}
std::string sheet_command::get_name()
{
	return name;
}