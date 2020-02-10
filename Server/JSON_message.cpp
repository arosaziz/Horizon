#include "lib/rapidjson/document.h"
#include "lib/rapidjson/writer.h"
#include "lib/rapidjson/stringbuffer.h"
#include "lib/rapidjson/prettywriter.h"
#include "include/JSON_message.h"
#include <iostream>
#include <fstream>
#include <vector>

namespace JSON_message
{
/* This JSON_Message class used to build JSON_message strings on the server side
 * 
 * The command that is returned needs to be freed
 *
 */
command *get_type(char const *const data)
{
    command *cmd = NULL;
    rapidjson::Document doc;

    //Parse the document using Parse(const char *);
    if (doc.Parse(data).HasParseError())
    {
        std::cout << "Error parsing JSON str\n";
        return cmd;
    }
    //Make sure that doc is an object (it was parsed correctly)
    if (doc.IsObject())
    {
        //Look for a data member (a.k.a field name)
        if (doc.HasMember("type"))
        {
            if (doc["type"].IsString() && doc["type"] == "admin")
            {
                cmd = new admin_command();
            }

            if (doc["type"].IsString() && doc["type"] == "close")
            {
                cmd = new close_command();
            }

            if (doc["type"].IsString() && doc["type"] == "user")
            {
                if (doc["order"].IsString() &&
                    doc["username"].IsString() &&
                    doc["password"].IsString())
                {
                    cmd = new user_command(doc["order"].GetString(),
                                           doc["username"].GetString(),
                                           doc["password"].GetString());
                }
            }

            if (doc["type"].IsString() && doc["type"] == "sheet")
            {
                if (doc["order"].IsString() &&
                    doc["name"].IsString())
                {
                    cmd = new sheet_command(doc["order"].GetString(),
                                            doc["name"].GetString());
                }
            }

            if (doc["type"].IsString() && doc["type"] == "open")
            {
                if (doc["name"].IsString() &&
                    doc["username"].IsString() &&
                    doc["password"].IsString())
                {
                    cmd = new open_command(doc["name"].GetString(),
                                           doc["username"].GetString(),
                                           doc["password"].GetString());
                }
            }

            if (doc["type"].IsString() && doc["type"] == "edit")
            {

                if (doc["cell"].IsString() &&
                    (doc["value"].IsString() || doc["value"].IsDouble()) &&
                    doc["dependencies"].IsArray())
                {
                    std::vector<std::string> v;

                    //create a vector from the array in the JSON string to pass through our edit object
                    for (rapidjson::SizeType i = 0; i < doc["dependencies"].Size(); i++)
                    {
                        v.push_back(doc["dependencies"][i].GetString());
                    }
                    if (doc["value"].IsString())
                    {
                        cmd = new edit_command(doc["cell"].GetString(), doc["value"].GetString(), v);
                    }
                    else
                    {
                        cmd = new edit_command(doc["cell"].GetString(), std::to_string(doc["value"].GetDouble()), v);
                    }
                }
            }

            if (doc["type"].IsString() && doc["type"] == "undo")
            {
                cmd = new undo_command();
            }

            if (doc["type"].IsString() && doc["type"] == "revert")
            {
                if (doc["cell"].IsString())
                {
                    cmd = new revert_command(doc["cell"].GetString());
                }
            }
        }
    }
    return cmd;
}

/*
* Takes in a spreadsheet and returns a string in JSON format
* using the rapidjson libraries. This is the full send message
* for the official communications protocol.
**/
std::string full_send_message(const spreadsheet &s)
{
    //Rapid JSON will require a string buffer to serialize our JSON string
    rapidjson::StringBuffer sb;

    //Outputs normal format
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

    //start the JSON string
    writer.StartObject();

    //populate the type field
    writer.Key("type");
    writer.String("full send");

    //populate the spreadsheet field
    writer.Key("spreadsheet");

    //begin our array of cells
    writer.StartObject();

    std::vector<std::string> cells = s.getAllCellNames();

    //iterate through each cell and add it to our json string
    for (unsigned int i = 0; i < cells.size(); i++)
    {
        std::string cell_contents = s.getCellContents(cells[i]);
        writer.Key(cells[i].c_str());

        if (cell_contents.empty())
        {
            writer.String("");
            continue;
        }

        //Check to see if the contents are a number
        size_t parse_len;
        try
        {
            double check = stod(cell_contents, &parse_len);

            //Make sure the whole string parsed into a double
            if (parse_len != cell_contents.size())
            {
                writer.String(cell_contents.c_str());
            }
            else
            {
                writer.Double(check);
            }
        }
        catch (std::exception &e)
        {
            writer.String(cell_contents.c_str());
        }
    }

    //end the list of cells once we iterate through every cell
    writer.EndObject();

    //end the JSON string
    writer.EndObject();

    //return our JSON string double newline deliminated
    return (std::string)(sb.GetString()) + "\n\n";
}

/*
* Takes in an error type (one or two) and returns a string 
  in JSON format using the rapidjson libraries. This is 
* the error message for the official communications protocol.
**/
std::string error_message(ERROR_TYPE e, std::string bad_cell)
{
    //Rapid JSON will require a string buffer to serialize our JSON string
    rapidjson::StringBuffer sb;

    //Outputs normal format
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

    if (e == INVALID_USER_PASS)
    {
        //start the JSON string
        writer.StartObject();

        //populate the type field
        writer.Key("type");
        writer.String("error");

        //populate the code field
        writer.Key("code");
        writer.Int(INVALID_USER_PASS);

        //populate source field
        writer.Key("source");
        writer.String("");

        writer.EndObject();

        // Return our JSON string with two newlines on the end
        return (std::string)(sb.GetString()) + "\n\n";
    }
    else if (e == CIRC_DEP)
    {
        //start the JSON string
        writer.StartObject();

        //populate the type field
        writer.Key("type");
        writer.String("error");

        //populate the code field
        writer.Key("code");
        writer.Int(CIRC_DEP);

        //populate source field
        writer.Key("source");
        writer.String(bad_cell.c_str());

        writer.EndObject();

        // Return our string with two newlines on the end
        return (std::string)(sb.GetString()) + "\n\n";
    }
    else
    {
        return NULL;
    }
}

/*
* Takes in a vector of spreadsheet string names and returns a 
* string in JSON format using the rapidjson libraries. This is 
* the spreadsheet list message for the official 
* communications protocol.
**/
std::string spreadsheet_list_message(std::vector<std::string> list)
{
    //Rapid JSON will require a string buffer to serialize our JSON string
    rapidjson::StringBuffer sb;

    //Outputs normal format
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

    //start the JSON string
    writer.StartObject();

    //populate the type field
    writer.Key("type");
    writer.String("list");

    //populate the spreadsheets field
    writer.Key("spreadsheets");

    //begin our list/array of spreadsheets
    writer.StartArray();

    //iterate through spreadsheet name and add it to our json string
    for (unsigned int i = 0; i < list.size(); i++)
    {
        writer.String(list[i].c_str());
    }

    //end the array once we iterate through every cell
    writer.EndArray();

    //end the JSON string
    writer.EndObject();

    //return our JSON string double newline deliminated
    return (std::string)(sb.GetString()) + "\n\n";
}

std::string full_send_message(const spreadsheet &s, const std::string &cell_name)
{
    //Rapid JSON will require a string buffer to serialize our JSON string
    rapidjson::StringBuffer sb;

    //Outputs normal format
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

    //start the JSON string
    writer.StartObject();

    //populate the type field
    writer.Key("type");
    writer.String("full send");

    //populate the spreadsheet field
    writer.Key("spreadsheet");

    //begin our array of cells
    writer.StartObject();

    std::string cell_contents = s.getCellContents(cell_name);

    writer.Key(cell_name.c_str());

    //Check to see if the contents are a number
    size_t parse_len;
    try
    {
        writer.Double(stod(cell_contents, &parse_len));
    }
    catch (std::exception &e)
    {
        writer.String(cell_contents.c_str());
    }

    //Make sure the whole string parsed into a double
    if (parse_len != cell_contents.size())
    {
        writer.String(cell_contents.c_str());
    }

    //end the list of cells once we iterate through every cell
    writer.EndObject();

    //end the JSON string
    writer.EndObject();

    //return our JSON string double newline deliminated
    //std::cout << sb.GetString() << std::endl;
    return (std::string)(sb.GetString()) + "\n\n";
}

/**
 * Save Spreadsheet by serializing it into a JSON and saving all its cells and dependents 
 **/
std::string save_spreadsheet(spreadsheet &s)
{
    //Rapid JSON will require a string buffer to serialize our JSON string
    rapidjson::StringBuffer sb;

    //Outputs normal format
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);

    //start the JSON string
    writer.StartObject();

    //populate the name field
    writer.Key("name");
    writer.String(s.getName().c_str());

    writer.Key("spreadsheet");

    //begin our object of cells
    writer.StartObject();

    std::vector<std::string> cells = s.getAllCellNames();

    //iterate through each cell and add it to our json string
    for (unsigned int i = 0; i < cells.size(); i++)
    {
        std::string cell_contents = s.getCellContents(cells[i]);

        //write cell_name
        writer.Key(cells[i].c_str());

        writer.StartObject();

        writer.Key("contents");

        //Check to see if the contents are a number
        size_t parse_len;
        try
        {
            double check = stod(cell_contents, &parse_len);

            //Make sure the whole string parsed into a double
            if (parse_len != cell_contents.size())
            {
                writer.String(cell_contents.c_str());
            }
            else
            {
                writer.Double(check);
            }
        }
        catch (std::exception &e)
        {
            writer.String(cell_contents.c_str());
        }

        writer.Key("dependencies");

        writer.StartArray();
        std::vector<std::string> deps = s.getCellDependencies(cells[i]);
        //get cell dependents
        for (unsigned int k = 0; k < deps.size(); k++)
        {
            writer.String(deps[k].c_str());
        }
        writer.EndArray();

        //end cell
        writer.EndObject();
    }

    //end spreadsheet
    writer.EndObject();

    //BEGIN EDIT HISTORY
    writer.Key("edit_history");
    writer.StartArray();

    std::stack<cell_data> edit_history(s.get_edits());
    //begin edit histories
    for (unsigned int i = 0; i < edit_history.size(); i++)
    {
        writer.StartObject();
        struct cell_data cell = edit_history.top();
        edit_history.pop();
        writer.Key("cellname");
        writer.String(cell.cellName.c_str());
        writer.Key("contents");
        writer.String(cell.contents.c_str());
        writer.Key("dependencies");
        writer.StartArray();

        std::vector<std::string> cell_dependencies = cell.dependencies;
        //iterate through each dependency
        for (unsigned int l = 0; l < cell_dependencies.size(); l++)
        {
            writer.String(cell_dependencies[l].c_str());
        }

        //end dependencies
        writer.EndArray();
        writer.EndObject();
    }
    writer.EndArray();

    //BEGIN CELL HISTORY
    writer.Key("cell_history");
    writer.StartObject();
    std::stack<cell_data> cell_history;

    //iterate through each cell and add it to our json string
    for (unsigned int i = 0; i < cells.size(); i++)
    {
        cell_history = s.get_cell_history(cells[i]);
        writer.Key(cells[i].c_str());
        writer.StartArray();
        for (unsigned int j = 0; j < cell_history.size(); j++)
        {
            struct cell_data cell = cell_history.top();
            cell_history.pop();
            writer.StartObject();
            writer.Key("contents");
            writer.String(cell.contents.c_str());
            writer.Key("dependencies");
            writer.StartArray();
            std::vector<std::string> cell_dependencies = cell.dependencies;
            //iterate through each dependency
            for (unsigned int k = 0; k < cell_dependencies.size(); k++)
            {
                writer.String(cell_dependencies[k].c_str());
            }

            writer.EndArray();

            writer.EndObject();
        }
        writer.EndArray();
    }
    writer.EndObject();

    //end save spreadsheet
    writer.EndObject();

    //return our JSON string double newline deliminated
    return (std::string)(sb.GetString()) + "\n\n";
}

spreadsheet open_spreadsheet(const std::string &filename)
{
    spreadsheet sheet;
    std::string file_string;
    std::string temp_string;

    std::ifstream spread_file;
    std::string path = "spreadsheets/" + filename + ".sprd";
    spread_file.open(path.c_str(), std::fstream::in);

    // Read in the file
    if (spread_file.is_open())
    {
        while (getline(spread_file, temp_string))
        {
            file_string += temp_string;
        }
        spread_file.close();
    }
    else
    {
        sheet.setName(filename);
        return sheet; // spreadsheet doesn't exist, return an empty sheet
    }

    rapidjson::Document doc;

    //Parse the document using Parse(const char *);
    if (doc.Parse(file_string.c_str()).HasParseError())
    {
        std::cout << "Error parsing JSON spreadsheet file\n";
    }
    //Make sure that doc is an object (it was parsed correctly)
    if (doc.IsObject())
    {
        //Look for a data member (a.k.a field name)
        if (doc.HasMember("name"))
        {
            if (doc["name"].IsString())
            {
                sheet.setName(doc["name"].GetString());
            }
        }
        if (doc.HasMember("spreadsheet"))
        {
            if (doc["spreadsheet"].IsObject())
            {
                // Iterate through all of the cells and store them
                for (auto it = doc["spreadsheet"].GetObject().MemberBegin(); it != doc["spreadsheet"].GetObject().MemberEnd(); ++it)
                {
                    const auto &cell_obj = doc["spreadsheet"].GetObject()[it->name.GetString()]; //make object value

                    // Get the cell dependenceies and put into a vector
                    std::vector<std::string> dependencies;
                    for (auto &element : cell_obj["dependencies"].GetArray())
                    {
                        dependencies.push_back(element.GetString());
                    }

                    // Get cell contents and store as a string, regardless of if it is a double or string
                    if (cell_obj["contents"].IsString())
                    {
                        sheet.setCellContents(it->name.GetString(), cell_obj["contents"].GetString(), dependencies);
                    }
                    else if (cell_obj["contents"].IsDouble())
                    {
                        sheet.setCellContents(it->name.GetString(), std::to_string(cell_obj["contents"].GetDouble()), dependencies);
                    }
                    else
                        std::cout << "Bad cell contents in saved JSON" << std::endl;
                }
            }
        }
    }

    return sheet;
}

std::unordered_map<std::string, std::string> deserialize_users()
{
    std::unordered_map<std::string, std::string> users;
    std::string file_string;
    std::string temp_string;

    std::ifstream user_file;
    std::string path = "spreadsheets/users";
    user_file.open(path.c_str(), std::fstream::in);

    // Read in the file
    if (user_file.is_open())
    {
        while (getline(user_file, temp_string))
        {
            file_string += temp_string;
        }
        user_file.close();
    }
    else
    {
        return users; // users doesn't exist, return an empty map
    }

    rapidjson::Document doc;

    //Parse the document using Parse(const char *);
    if (doc.Parse(file_string.c_str()).HasParseError())
    {
        std::cout << "Error parsing JSON spreadsheet file\n";
    }
    //Make sure that doc is an object (it was parsed correctly)
    if (doc.IsObject())
    {
        //Look for a data member (a.k.a field name)
        if (doc.HasMember("users"))
        {
            if (doc["users"].IsObject())
            {
                // Iterate through all of the users and store them
                for (auto it = doc["users"].GetObject().MemberBegin(); it != doc["users"].GetObject().MemberEnd(); ++it)
                {
                    users[it->name.GetString()] = doc["users"].GetObject()[it->name.GetString()].GetString();
                }
            }
        }
    }
    return users;
}

std::string state_message(std::unordered_map<std::string, std::string> users)
{
    //Rapid JSON will require a string buffer to serialize our JSON string
    rapidjson::StringBuffer sb;

    //Outputs normal format
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

    //start the JSON string
    writer.StartObject();

    //populate the type field
    writer.Key("type");
    writer.String("state");

    writer.Key("users");

    //begin our object of cells
    writer.StartObject();

    for (auto it = users.begin(); it != users.end(); ++it)
    {
        writer.Key(it->first.c_str());
        writer.String(it->second.c_str());
    }

    writer.EndObject();

    writer.EndObject();

    return (std::string)(sb.GetString()) + "\n\n";
}

std::string send_message(std::string message)
{
    //Rapid JSON will require a string buffer to serialize our JSON string
    rapidjson::StringBuffer sb;

    //Outputs normal format
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

    //start the JSON string
    writer.StartObject();

    //populate the type field
    writer.Key("type");
    writer.String("message");

    writer.Key("msg");
    writer.String(message.c_str());

    writer.EndObject();

    return (std::string)(sb.GetString()) + "\n\n";
}

} // namespace JSON_message
