#include "spreadsheet.h"
#include <stdexcept>
#include <iterator>
#include <JSON_message.h>
#include <fstream>
#include <iostream>

/* ========== CELL FUNCTIONS ======= */
cell::cell()
{
	this->contents = "";
}

cell::cell(const std::string &cellName)
{
	this->cellName = cellName;
	this->contents = "";
}

cell::cell(const cell &other_cell)
{
	this->contents = other_cell.contents;
	this->dependencies = other_cell.dependencies;
	this->history = other_cell.history;
	this->cellName = other_cell.cellName;
}

cell::cell(const std::string &contents, const std::vector<std::string> &dependencies,
		   const std::string &cellName)

{
	this->dependencies = dependencies;
	this->contents = contents;
	this->cellName = cellName;
}

cell::~cell()
{
	// nothing for now
	// no 'new' calls, so as far as I know,
	// nothing needs to be deleted
}

/* ========== SPREADSHEET FUNCTIONS ====== */

spreadsheet::spreadsheet()
{
	this->hasChanged = false;
}

spreadsheet::spreadsheet(std::string name)
{
	this->name = name;
}

spreadsheet::spreadsheet(const spreadsheet &sheet)
{
	this->name = sheet.name;
	this->edits = sheet.edits;
	this->cells = sheet.cells;
	this->dependents = sheet.dependents;
	this->dependees = sheet.dependees;
	this->hasChanged = sheet.hasChanged;
}

// spreadsheet::spreadsheet(std::string JSON_Data)
// {
//     // Read spreadsheet data from JSON
// }

spreadsheet::~spreadsheet()
{
	// destroy everything
}

/*
 * Returns the dependencies for the requested cell.
 * Returns an empty vector if the requested cell does not exist.
 */
const std::vector<std::string> spreadsheet::getCellDependencies(const std::string &cellName) const
{
	try
	{
		return cells.at(cellName).dependencies;
	}
	catch (const std::out_of_range &e)
	{
		std::vector<std::string> vec;
		return vec;
	}
	std::vector<std::string> vec;
	return vec;
}

/* 
 * Returns the contents for the requested cell.
 * Returns NULL if the requested cell does not exist.
 */
const std::string spreadsheet::getCellContents(const std::string &cellName) const
{
	try
	{
		return cells.at(cellName).contents;
	}
	catch (const std::out_of_range &e)
	{
		return NULL;
	}
}

/*
 * Returns a vector of strings that contains all of the cells names that
 * are modified in the spreadsheet.
 */
const std::vector<std::string> spreadsheet::getAllCellNames() const
{
	std::vector<std::string> vec;
	// Const itnerator because we say we won't modify this, so cbegin() and cend() return
	// const begin and end values
	for (std::unordered_map<std::string, cell>::const_iterator it = cells.cbegin(); it != cells.cend(); it++)
	{
		// it->first returns the key...
		vec.push_back(it->first);
	}

	return vec;
}

/*
 * Returns the spreadsheet name as a string
 */
const std::string spreadsheet::getName() const
{
	return name;
}

/*
 * Set a specific cells contents and dependencies.
 * 
 * If the cell cannot be added, the invalid cell name is returned.
 * Else null
 */
bool spreadsheet::setCellContents(const std::string &cellName, const std::string &contents,
								  std::vector<std::string> const &dependencies)
{
	// Check if the cell exists, if it doesn't, add it, if it does,
	//  push the current cell onto the history stack,
	//  change the contents, dependencies, check if it's a formula and change that

	/* circ dep and valid cell checking */
	if (!cellIsValid(cellName, contents, dependencies))
	{
		return false;
	}

	std::vector<std::string> oldDeps;

	// If the cell already exists
	try
	{
		cell old_cell = cells.at(cellName); // Finds cell, throws ex if !exists

		// CHANGED
		cell_data old_data;
		old_data.cellName = cellName;
		old_data.contents = old_cell.contents;
		old_data.dependencies = old_cell.dependencies;

		edits.push(old_data); // Push old cell data onto spreadsheet history stack

		cells[cellName].history.push(old_data); // Push old cell contents and dependencies onto stack
		// ENDCHANGE

		cells[cellName].contents = contents;
		oldDeps = cells[cellName].dependencies;
		cells[cellName].dependencies = dependencies;
	}
	// If the cell doesn't exist already
	catch (const std::out_of_range &e)
	{
		// CHANGED
		cell_data empty_data;
		empty_data.cellName = cellName;
		empty_data.contents = "";
		empty_data.dependencies = std::vector<std::string>();

		edits.push(empty_data); // Push an empty cell onto the edits, so that we know the cell was

		cell new_cell = cell(contents, dependencies, cellName);
		new_cell.history.push(empty_data);

		cells[cellName] = new_cell;
	}

	/* remove old deps (if any) */
	if (oldDeps.size() > 0)
	{
		cell rm_deps = cells.at(cellName);

		for (unsigned int i = 0; i < oldDeps.size(); i++)
		{
			removeDependency(oldDeps[i], cellName);
		}
	}

	/* add new deps */
	for (unsigned int i = 0; i < dependencies.size(); i++)
	{
		addDependency(dependencies[i], cellName);
	}

	hasChanged = true;
	return true;
}

/*
 * Saves the spreadsheet
 */
void spreadsheet::saveSpreadsheet()
{
	std::ofstream myfile;

	// Spreadsheet files wil have .sprd extensions and stored in the
	// spreadsheets directory
	std::string path = "spreadsheets/" + this->getName() + ".sprd";
	myfile.open(path.c_str());

	std::string saved;
	spreadsheet s = *this;
	saved = JSON_message::save_spreadsheet(s);
	myfile << saved;

	myfile.close();
	this->hasChanged = false;
}

/*
 * Remove a cell from the cells map
 *  (probably not needed unless an undo or revert deletes the cell)
 */
void spreadsheet::removeCell(const std::string &cellName)
{
	cell old_cell;
	std::vector<std::string> deps;

	try
	{
		old_cell = cells.at(cellName); // Finds cell, throws ex if !exists
		deps = cells[cellName].dependencies;

		/* remove old deps (if any) */
		if (deps.size() > 0)
		{
			cell rm_deps = cells.at(cellName);

			for (unsigned int i = 0; i < deps.size(); i++)
			{
				removeDependency(deps[i], cellName);
			}
		}
	}

	catch (const std::out_of_range &e)
	{
	}

	cells.erase(cellName);
}

/*
 * Check for circular dependencies and other errors that might occur
 * such as if a formula is trying to reference a string or something similar
 *
 * Requirements for valid cell:
 * 	- if the contents are a string, it must not be a dependent of any cell
 * 	- if the contents are a formula, it must not cause a circular dependancy
 * 		either directly or indirectly
 */
const bool spreadsheet::cellIsValid(const std::string &cellName, const std::string &contents, const std::vector<std::string> &deps) const
{
	int ret = checkContents(cellName, contents);

	if (ret < 0)
	{
		return false;
	}

	for (unsigned int i = 0; i < deps.size(); i++)
	{
		std::unordered_set<std::string> visited;
		ret = checkCircDeps(deps[i], cellName, &visited);

		if (ret < 0)
		{
			return false;
		}
	}

	return true;
}

/* This will make sure the new cell contents are a formula or
* a number if the cell exists as a dependent. If the contents
* are incorrect, it will return INVALID_DEPENDENCY
*/
const int spreadsheet::checkContents(const std::string &cellName, const std::string &contents) const
{
	// Check too see if given cell is part of another cell's formula
	// (meaning it is a dependent of another cell)
	if (dependents.find(cellName) != dependents.end())
	{
		//Check to see if the contents are a formula
		if (contents[0] != '=')
		{
			//Check to see if the contents are a number
			size_t parse_len;
			try
			{
				stod(contents, &parse_len);
			}
			catch (std::exception &e)
			{
				return INVALID_DEPENDENCY;
			}

			//Make sure the whole string parsed into a double
			if (parse_len != contents.size())
			{
				return INVALID_DEPENDENCY;
			}
		}
	}

	return 0;
}

/* This function is recursive and it will visit every 
 * cell associated with the given cell and return 
 * CIRCULAR_DEPENDENCY if such a dependency is ever found
 */
const int spreadsheet::checkCircDeps(const std::string &start, const std::string &cellName, std::unordered_set<std::string> *visited) const
{
	//std::unordered_set<std::string> directDependents = *(dependents.find(cellName));
	std::unordered_set<std::string> directDependents;

	if (dependents.find(cellName) != dependents.end())
	{
		directDependents = dependents.at(cellName);
	}
	else
	{
		return 0;
	}

	std::string curCell = cellName;
	visited->insert(curCell);

	for (auto it = directDependents.begin(); it != directDependents.end(); ++it)
	{
		if (*it == start)
		{
			return CIRCULAR_DEPENDENCY;
		}
		else if (visited->find(*it) != visited->end())
		{
			checkCircDeps(start, *it, visited);
		}
	}

	return 0;
}

void spreadsheet::addDependency(std::string s, std::string t)
{
	std::unordered_set<std::string> dependent_list;
	std::unordered_set<std::string> dependee_list;

	if (dependents.find(s) != dependents.end())
	{
		dependent_list = dependents.at(s);

		if (dependent_list.find(t) == dependent_list.end())
		{
			dependent_list.insert(t);
		}
		dependents[s] = dependent_list;
	}
	else
	{
		dependent_list.insert(t);
		dependents[s] = dependent_list;
	}

	if (dependees.find(t) != dependees.end())
	{
		dependee_list = dependees.at(t);

		if (dependee_list.find(s) == dependee_list.end())
		{
			dependee_list.insert(s);
		}
		dependees[t] = dependee_list;
	}
	else
	{
		dependee_list.insert(s);
		dependees[t] = dependee_list;
	}
}

void spreadsheet::removeDependency(std::string s, std::string t)
{
	std::unordered_set<std::string> dependent_list;
	std::unordered_set<std::string> dependee_list;

	if (dependents.find(s) != dependents.end())
	{
		dependent_list = dependents.at(s);

		if (dependent_list.find(t) != dependent_list.end())
		{
			dependent_list.erase(t);
		}

		if (dependent_list.size() > 0)
		{
			dependents[s] = dependent_list;
		}
		else
		{
			dependents.erase(s);
		}
	}

	if (dependees.find(t) != dependees.end())
	{
		dependee_list = dependees.at(t);

		if (dependee_list.find(s) != dependee_list.end())
		{
			dependee_list.erase(s);
		}

		if (dependee_list.size() > 0)
		{
			dependees[t] = dependee_list;
		}
		else
		{
			dependees.erase(t);
		}
	}
}

/*
 * Revert the provided cell to the previous contents.
 * If cell was previously empty, the cell contents is delted and returns true.
 * If the cell is current empty, do nothing. True is returned
 * 
 * If reverting the cell causes a circular dependency, nothing is changed and 
 * false is returned.
 */
bool spreadsheet::revertCell(const std::string &cellName)
{
	if (cells.find(cellName) == cells.end()) // The cell currently has no contents
		return true;

	// If the cell was previously empty
	if (cells[cellName].history.empty())
	{
		cells[cellName].contents = "";
		cells[cellName].dependencies = std::vector<std::string>();
		return true;
	}
	// If the cell has a history
	else
	{
		// CHANGED
		cell_data old_data = cells[cellName].history.top();
		if (setCellContents(cellName, old_data.contents, old_data.dependencies))
		{
			cells[cellName].history.pop(); // Pop off the old value
			cells[cellName].history.pop(); // Pop off the cell that setCellContents added
			return true;
		}
		return false;
	}
}

/**
 * Undo a cell
 * Returns UNDO_SUCCESS for a successful undo
 * returns UNDO_FAIL for a failed undo
 * returns UNDO_EMPTY if undo was called on an empty spreadsheet
 **/
UNDO_STATUS spreadsheet::undo(std::string &cellName)
{
	//gets the previous spreadsheet cell
	if (edits.empty())
		return UNDO_EMPTY;

	// ALMOST ALL OF THIS CHANGED slightly
	cell_data old_data = edits.top();

	//check if old cell does not return circular dependencies
	if (cellIsValid(old_data.cellName, old_data.contents, old_data.dependencies))
	{
		cell current_cell = cells[old_data.cellName];
		for (auto const &dep : current_cell.dependencies)
		{
			removeDependency(dep, current_cell.cellName);
		}

		//change the top of the cell to the old cell
		// CHANGED
		current_cell.contents = old_data.contents;
		current_cell.dependencies = old_data.dependencies;
		if (!current_cell.history.empty())
			current_cell.history.pop(); // Fairly certain this is the only way
		cells[old_data.cellName] = current_cell;

		// Pop off the edit we just applied
		edits.pop();

		//add the new dependencies
		for (auto const &dep : current_cell.dependencies)
		{
			addDependency(dep, current_cell.cellName);
		}
		hasChanged = true;
		return UNDO_SUCCESS;
	}

	//could not undo
	cellName = old_data.cellName;
	return UNDO_FAIL;
}

/**
 * Returns true if the spreadsheet has changed
 **/
bool spreadsheet::getSaveStatus()
{
	return this->hasChanged;
}

void spreadsheet::setName(std::string name)
{
	this->name = name;
}

void spreadsheet::print_graph()
{
	std::cout << "Dependents" << std::endl;
	for (auto &elem : dependents)
	{
		std::cout << elem.first << ":\t";
		for (auto &elem2 : elem.second)
			std::cout << elem2;
		std::cout << std::endl;
	}
	std::cout << "\nDependees" << std::endl;
	for (auto &elem : dependees)
	{
		std::cout << elem.first << ":\t";
		for (auto &elem2 : elem.second)
			std::cout << elem2;
		std::cout << std::endl;
	}
}

std::stack<cell_data> spreadsheet::get_cell_history(std::string &cellName)
{
	std::stack<cell_data> empty;

	if (cells.find(cellName) != cells.end())
	{
		return cells.at(cellName).history;
	}

	return empty;
}

std::stack<cell_data> spreadsheet::get_edits()
{
	return edits;
}

// std::vector<std::string> spreadsheet::get_cell_history(std::string cellName)
// {
// 	return std::vector<std::string>();
// }