#include <iostream>
#include "spreadsheet_server.h"
#include "JSON_message.h"
#include <functional>
#include <vector>
#include <thread>
#include <chrono>
#include <fstream>

#define SET_CALLBACK(callback) (std::bind(&spreadsheet_server::callback, this, std::placeholders::_1))

/*
 * Create a spreadsheet_server on port 2112. 
 * A spreadsheet_server handles network connections with spreadsheet clients, 
 * stores spreadsheet clients, stores spreadsheets and handles any interactions
 * between spreadsheet clients and this server. This object follows the Sendit 
 * protocol version 1.1.1.
 */
spreadsheet_server::spreadsheet_server()
{
    // Default username and password
    // logins["admin"] = "password";

    // Create and add some test spreadsheets for the time being
    // TODO remove later
    spreadsheet sheet("Populated Sheet no formulas");
    sheet.setCellContents("A1", "1", std::vector<std::string>());
    sheet.setCellContents("B1", "meow", std::vector<std::string>());
    //sheet.setCellContents("C1", "=A1*5", std::vector<std::string>(1, "A1"));
    sheet.setCellContents("D1", "2", std::vector<std::string>());

    spreadsheet sheet3("Populated Sheet w formula");
    sheet3.setCellContents("A1", "1", std::vector<std::string>());
    sheet3.setCellContents("B1", "meow", std::vector<std::string>());
    sheet3.setCellContents("C1", "=A1*5", std::vector<std::string>(1, "A1"));
    sheet3.setCellContents("D1", "2", std::vector<std::string>());

    spreadsheet sheet2("Empty Sheet");
    // sheet2.setCellContents("A2", "", std::vector<std::string>());

    // sheets[sheet.getName()] = sheet;
    // sheets[sheet2.getName()] = sheet2;
    // sheets[sheet3.getName()] = sheet3;

    is_running = true;
    open_all_spreadsheets();
    logins = JSON_message::deserialize_users();
    admin = NULL;

    // Start spreadsheet saver thread
    saver_thread = std::thread(spreadsheet_saver, this); //.detach();

    try
    {
        // accept_client is a callback function, called when a TCP connection is made
        // with a new client. Callbacks must be bound in this format
        // default port for a server is 2112
        auto acceptor = SET_CALLBACK(handle_first_contact);
        server = new tcp_server(this->io_context, 2112, acceptor);
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

/*
 * Create a spreadsheet_server with the specified port number. Everything else is the
 * same as the default constructor
 * 
 * Parameters:
 *      int port - the port to run the spreadsheet_server on
 */
spreadsheet_server::spreadsheet_server(int port)
{
    try
    {
        // accept_client is a callback function, called when a TCP connection is made
        // with a new client
        // default port for a server is 2112
        auto acceptor = std::bind(&spreadsheet_server::handle_first_contact, this, std::placeholders::_1);
        server = new tcp_server(this->io_context, port, acceptor);
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

spreadsheet_server::~spreadsheet_server()
{
    // Free the tcp_server
    delete (server);
}

/*
 * Run the server. If an exception is thrown, it's printed to standard error.
 */
void spreadsheet_server::start()
{
    try
    {
        std::cout << "Server up and running" << std::endl;
        this->io_context.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

/*
 * First contact with a client. This is where the client is saved and 
 * the initial list of spreadsheets is sent to the client.
 * This function also sets the client's disconnect function and calls 
 * for more data from the client.
 */
void spreadsheet_server::handle_first_contact(client *c)
{
    std::cout << "Received connection, assigning ID: " << c->get_id() << std::endl;
    // Add the client to the list of clients
    lock.lock();
    this->clients.insert(c);
    lock.unlock();

    // Set the callback function for when data is recieved
    c->callback_func = std::bind(&spreadsheet_server::handle_client_login, this, std::placeholders::_1);
    // Set the callback function for when a client disconnects
    c->disconnect_func = std::bind(&spreadsheet_server::handle_client_disconnect, this, std::placeholders::_1);

    // Send list of spreadsheets
    c->write_data(JSON_message::spreadsheet_list_message(this->get_spreadsheet_names()));

    // returns immediately
    // asynchronously waits for data to come from the socket, then calls
    // c->callback_func
    c->client::get_data();
}

/*
 * This function should be called when the client sends a 
 * username, password and spreadsheet name.
 * 
 * If these items are not sent, the server disconnects from the client.
 * 
 * If the username doesn't exist, it's added to the database and 
 * the requested spreadsheet is sent.
 * 
 * If the password is wrong, the server sends back "Bad Password" error, 
 * the list of spreadsheets is sent again, and the callback is not modified.
 * 
 * If the password is correct, the server sends the requested spreadsheet.
 */
void spreadsheet_server::handle_client_login(client *c)
{
    if (admin != NULL)
        admin->write_data(c->message);

    command *cmd = JSON_message::get_type(c->message);

    // If the the data that the client sent isn't a proper JSON open command
    // then disconnect the client
    if (cmd == NULL || (cmd->get_type() != "open" && cmd->get_type() != "admin"))
    {
        c->disconnect_client();
        return;
    }

    //handle administrator
    if (cmd->get_type() == "admin")
    {
        c->callback_func = SET_CALLBACK(handle_admin);
        c->disconnect_func = SET_CALLBACK(handle_admin_disconnect);
        lock.lock();
        admin = c;
        c->write_data(JSON_message::state_message(logins));
        lock.unlock();
        // c->disconnect_func() =  // TODO set up admin disconnect
        c->get_data();
        return;
    }

    std::string username = ((open_command *)(cmd))->get_username();
    std::string password = ((open_command *)(cmd))->get_password();
    std::string sprd_name = ((open_command *)(cmd))->get_name();

    delete (cmd);

    if (check_login(username, password))
    {
        lock.lock();

        // If the spreadsheet doesn't currently exist
        if (this->sheets.find(sprd_name) == this->sheets.end())
        {
            std::replace(sprd_name.begin(), sprd_name.end(), '/', '_');
            sheets[sprd_name] = spreadsheet(sprd_name); // Add a new spreadsheet to the database

            // Now that there is a new spreadsheet, save all of the names to a file
        }

        c->write_data(JSON_message::full_send_message(this->sheets[sprd_name]));

        // Associate spreadsheet with this client
        if (sprd_conns.find(sprd_name) == sprd_conns.end()) // didn't find a set
        {
            std::unordered_set<client *> clients;
            clients.insert(c);
            sprd_conns[sprd_name] = clients;
        }
        else
        {
            sprd_conns[sprd_name].insert(c);
        }

        lock.unlock();
        if (admin != NULL)
            admin->write_data(JSON_message::spreadsheet_list_message(this->get_spreadsheet_names()));

        save_sprd_names();
        save_logins();

        c->connected_spreadsheet = sprd_name;
        c->callback_func = std::bind(&spreadsheet_server::handle_edits, this, std::placeholders::_1);
        c->get_data();
    }
    else // If the username is stored but the password doesn't match
    {
        c->write_data(JSON_message::error_message(INVALID_USER_PASS, ""));

        // Send the list of spreadsheets back and let them try again
        c->write_data(JSON_message::spreadsheet_list_message(this->get_spreadsheet_names()));

        // callback should still be this function
        c->get_data();
    }
}

/* Handles edits from the client
 */
void spreadsheet_server::handle_edits(client *c)
{
    if (admin != NULL)
        admin->write_data(c->message);

    command *cmd = JSON_message::get_type(c->message);

    if (cmd == NULL)
    {
        c->disconnect_client();
        return;
    }

    // If the command is an edit command
    if (cmd->get_type() == "edit")
    {
        std::string cellName = ((edit_command *)(cmd))->get_cell();
        std::string contents = ((edit_command *)(cmd))->get_value();
        std::vector<std::string> dependencies = ((edit_command *)(cmd))->get_dependencies();

        lock.lock();

        //Make sure the contents can be set, if they can be, full send the spreadsheet to the connected clients
        if (sheets[c->connected_spreadsheet].setCellContents(cellName, contents, dependencies))
        {
            std::string full_send = JSON_message::full_send_message(sheets[c->connected_spreadsheet]);
            // After the edit is made, go through all the clients connected to this
            // spreadshseet and send them the new spreadsheet
            // for (auto it = sprd_conns[c->connected_spreadsheet].cbegin(); it != sprd_conns[c->connected_spreadsheet].cbegin(); ++it)
            for (const auto &elem : sprd_conns[c->connected_spreadsheet])
            {
                elem->write_data(full_send);
            }
            lock.unlock();
        }
        else // If there's a circular dependency error when trying to add the cell
        {
            lock.unlock();
            c->write_data(JSON_message::error_message(CIRC_DEP, cellName));
        }
    }
    else if (cmd->get_type() == "revert")
    {
        std::string cellName = ((revert_command *)(cmd))->get_cell();
        // if we do not get a circ dep
        lock.lock();
        if (sheets[c->connected_spreadsheet].revertCell(cellName))
        {
            std::string full_send = JSON_message::full_send_message(sheets[c->connected_spreadsheet]);
            // After the edit is made, go through all the clients connected to this
            // spreadshseet and send them the new spreadsheet
            // for (auto it = sprd_conns[c->connected_spreadsheet].cbegin(); it != sprd_conns[c->connected_spreadsheet].cbegin(); ++it)
            for (const auto &elem : sprd_conns[c->connected_spreadsheet])
            {
                elem->write_data(full_send);
            }
            lock.unlock();
        }
        //otherwise send a circ dep
        else
        {
            lock.unlock();
            c->write_data(JSON_message::error_message(CIRC_DEP, cellName));
        }
    }

    else if (cmd->get_type() == "undo")
    {
        // if we do not get a circ dep
        std::string undo_cell = "";
        lock.lock();
        UNDO_STATUS status = sheets[c->connected_spreadsheet].undo(undo_cell);

        if (status == UNDO_SUCCESS)
        {
            std::string full_send = JSON_message::full_send_message(sheets[c->connected_spreadsheet]);
            // After the edit is made, go through all the clients connected to this
            // spreadshseet and send them the new spreadsheet
            // for (auto it = sprd_conns[c->connected_spreadsheet].cbegin(); it != sprd_conns[c->connected_spreadsheet].cbegin(); ++it)
            for (const auto &elem : sprd_conns[c->connected_spreadsheet])
            {
                elem->write_data(full_send);
            }
            lock.unlock();
        }
        else if (status == UNDO_FAIL)
        {
            lock.unlock();
            c->write_data(JSON_message::error_message(CIRC_DEP, undo_cell));
        }
        else if (status == UNDO_EMPTY)
        {
            std::string full_send = JSON_message::full_send_message(sheets[c->connected_spreadsheet]);
            // After the edit is made, go through all the clients connected to this
            // spreadshseet and send them the new spreadsheet
            // for (auto it = sprd_conns[c->connected_spreadsheet].cbegin(); it != sprd_conns[c->connected_spreadsheet].cbegin(); ++it)
            for (const auto &elem : sprd_conns[c->connected_spreadsheet])
            {
                elem->write_data(full_send);
            }
            lock.unlock();
        }
        else
            lock.unlock();
    }

    delete (cmd);

    c->get_data();
}

void spreadsheet_server::handle_client_disconnect(client *c)
{
    lock.lock();
    // Erase pointer to client from connected spreadsheets
    sprd_conns[c->connected_spreadsheet].erase(c);

    // Let go of the pointer and let the destructor clean everything up
    this->clients.erase(c);
    lock.unlock();
    std::cout << "DISCONNECTED: " << c->get_id() << std::endl;
}

void spreadsheet_server::handle_admin_disconnect(client *c)
{
    lock.lock();
    // Erase the admin
    admin = NULL;

    // Let go of the pointer and let the destructor clean everything up
    this->clients.erase(c);
    lock.unlock();
    std::cout << "DISCONNECTED: " << c->get_id() << std::endl;
}

void spreadsheet_server::handle_admin(client *c)
{
    command *cmd = JSON_message::get_type(c->message);

    if (cmd == NULL)
    {
        c->disconnect_client();
        return;
    }

    //Handle admin commands
    std::string cmd_type = cmd->get_type();
    if (cmd_type == "admin")
    {
        // State of the spreadsheet
        lock.lock();
        c->write_data(JSON_message::state_message(logins));
        lock.unlock();

        c->write_data(JSON_message::spreadsheet_list_message(this->get_spreadsheet_names()));
    }
    else if (cmd_type == "close")
    {
        c->write_data("1");
        shutdown_server();
        // this->io_context.reset();
        this->io_context.stop();
        return;
    }
    else if (cmd_type == "user")
    {
        modify_user((user_command *)(cmd));
        // State of the spreadsheet
        lock.lock();
        c->write_data(JSON_message::state_message(logins));
        lock.unlock();
        save_logins();
    }
    else if (cmd_type == "sheet")
    {
        if (!modify_sheets((sheet_command *)(cmd)))
        {
            c->write_data("0");
            // JSON_message::send_message("Unable to delete spreadsheet :" +
            // ((sheet_command *)(cmd))->get_name() + ", currently active");
        }
        c->write_data(JSON_message::spreadsheet_list_message(this->get_spreadsheet_names()));
    }

    delete (cmd);

    c->get_data();
}

/*
 * Check the username and password against the stored logins.
 * Returns true if the username exists and the password matches.
 * Returns true if the username doesn't exist and stores the username
 * and password.
 * 
 * 
 * Returns false if the username exists and the password doesn't match.
 */
bool spreadsheet_server::check_login(std::string username, std::string password)
{
    lock.lock();
    if (this->logins.find(username) == this->logins.end())
    {
        // Username doesn't exist - add it
        this->logins[username] = password;
        if (admin != NULL)
            admin->write_data(JSON_message::state_message(logins));
    }
    else
    {
        // Check if the password matches the one in the database
        if (this->logins[username] != password)
        {
            lock.unlock(); // return the lock before returning
            return false;
        }
    }
    lock.unlock();

    return true;
}

/*
 * Returns the names of the stored spreadsheets
 * in a vector of strings.
 * Thread safe.
 */
std::vector<std::string> spreadsheet_server::get_spreadsheet_names()
{
    std::vector<std::string> list_of_sheets;
    lock.lock();
    for (const auto &sheet : this->sheets)
        list_of_sheets.push_back(sheet.first);
    lock.unlock();
    return list_of_sheets;
}

void spreadsheet_server::save_spreadsheets()
{
    std::vector<std::string> list = get_spreadsheet_names();
    lock.lock();
    for (unsigned int i = 0; i < list.size(); i++)
    {

        //if the spreadsheet status has changed
        if (sheets[list[i]].getSaveStatus())
        {
            //save the spreadsheet to the file
            sheets[list[i]].saveSpreadsheet();
        }
    }
    lock.unlock();
}

/*
 * Saves spreadsheet names to a file with each
 * name on a new line
 * 
 *  THREAD SAFE 
 */
void spreadsheet_server::save_sprd_names()
{
    // Spreadsheet files wil have .sprd extensions and stored in the
    // spreadsheets directory
    std::vector<std::string> list = get_spreadsheet_names();

    io_lock.lock();
    std::ofstream names_file;

    std::string path = "spreadsheets/sprd_names";
    names_file.open(path.c_str());

    if (names_file.is_open())
    {
        // Save a document with each spreadsheet name on a new line
        for (unsigned int i = 0; i < list.size(); i++)
        {
            names_file << list[i];
            names_file << "\n";
        }
    }


    names_file.close();
    io_lock.unlock();
}

void spreadsheet_server::save_logins()
{
    lock.lock();
    std::string users = JSON_message::state_message(logins);
    lock.unlock();

    io_lock.lock();
    std::ofstream logins_file;

    std::string path = "spreadsheets/users";
    logins_file.open(path.c_str());
    logins_file << users;
    logins_file.close();
    io_lock.unlock();
}

void spreadsheet_server::spreadsheet_saver(spreadsheet_server *s)
{
    while (s->currently_running())
    {
        s->save_spreadsheets();
        std::this_thread::sleep_for(std::chrono::seconds(SAVE_INTERVAL));
    }
}

void spreadsheet_server::open_all_spreadsheets()
{
    std::ifstream names_file;
    std::string path = "spreadsheets/sprd_names";
    std::string sprd_name;

    names_file.open(path.c_str(), std::fstream::in);
    if (names_file.is_open())
    {
        while (getline(names_file, sprd_name))
        {
            sheets[sprd_name] = JSON_message::open_spreadsheet(sprd_name);
        }

        names_file.close();
    }
    else
    {
        // Names file doesn't exist, don't read in spreadsheets
    }
}

void spreadsheet_server::modify_user(user_command *cmd)
{
    std::string order = cmd->get_order();
    std::string username = cmd->get_username();

    lock.lock();

    if (order == "new" || order == "change")
    {
        logins[username] = cmd->get_password();
    }
    else if (order == "delete")
    {
        logins.erase(username);
    }

    lock.unlock();
}

bool spreadsheet_server::modify_sheets(sheet_command *cmd)
{
    std::string order = cmd->get_order();
    std::string sprd_name = cmd->get_name();

    if (order == "new")
    {
        lock.lock();
        sheets[sprd_name] = spreadsheet(sprd_name);
        lock.unlock();

        save_sprd_names();
        return true;
    }
    else if (order == "delete")
    {
        lock.lock();
        if (sprd_conns[sprd_name].empty())
        {
            // Spreadsheet has no active clients
            sheets.erase(sprd_name);
            lock.unlock();
            save_sprd_names();
            return true;
        }
        else
        {
            // The spreadsheet has active clients, return false
            lock.unlock();
            return false;
        }
    }
    else
    {
        std::cout << "Bad admin command" << std::endl;
        return false;
    }
}

void spreadsheet_server::shutdown_server()
{
    std::vector<client *> copy_clients;
    // disconnect all clients
    for (auto &elem : clients)
    {
        copy_clients.push_back(elem);
    }

    for (auto &elem : copy_clients)
    {
        elem->disconnect_client();
    }

    save_spreadsheets();

    is_running = false;

    saver_thread.join();
}

bool spreadsheet_server::currently_running() const
{
    return is_running;
}
