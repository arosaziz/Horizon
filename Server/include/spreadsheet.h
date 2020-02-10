#ifndef SPREADSHEET_H
#define SPREADSHEET_H

#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <unordered_set>

#define CIRCULAR_DEPENDENCY -1
#define INVALID_DEPENDENCY -2

class spreadsheet;
class cell;

enum UNDO_STATUS
{
	UNDO_SUCCESS = 0,
	UNDO_FAIL = 1,
	UNDO_EMPTY = 2
};

struct cell_data
{
	std::string cellName;
	std::string contents;
	std::vector<std::string> dependencies;
};

class cell
{
  private:
	friend class spreadsheet;

	std::stack<cell_data> history;
	std::vector<std::string> dependencies;
	std::string contents;
	std::string cellName;

  public:
	// All of these constructors/desctructor need to be public
	// because when we pass cells to data structures (such as a vector), it
	// makes a copy and needs to call the constructor
	cell();
	cell(const std::string &cellName);
	cell(const cell &other_cell);
	cell(const std::string &contents, const std::vector<std::string> &dependencies, const std::string &cellName);
	~cell();
	//const std::string getDependencies() const;
	//const std::string getContents() const;
};

class spreadsheet
{
  private:
	std::string name;
	//Worry about this data member later
	std::stack<cell_data> edits;
	std::unordered_map<std::string, cell> cells;
	std::unordered_map<std::string, std::unordered_set<std::string>> dependents;
	std::unordered_map<std::string, std::unordered_set<std::string>> dependees;
	bool hasChanged;

	void removeCell(const std::string &cellName);
	//const std::string cellIsValid(const std::string &cellName) const;
	const bool cellIsValid(const std::string &cellName, const std::string &contents, const std::vector<std::string> &deps) const;
	const std::vector<std::string> getDirectDependents(std::string cellName);
	//const int getCellsToRecalculate(const std::vector<std::string> & cellNames, const std::vector<std::string> *cellsToRecalculate) const;
	//const int checkCellChain(const std::string & start, const std::string & cellName, std::unordered_set<std::string> *visited, std::unordered_set<std::string> *changed) const;
	const int checkCircDeps(const std::string &start, const std::string &cellName, std::unordered_set<std::string> *visited) const;
	const int checkContents(const std::string &cellName, const std::string &contents) const;
	void addDependency(std::string s, std::string t);
	void removeDependency(std::string s, std::string t);
	void print_graph();

  public:
	spreadsheet();
	spreadsheet(std::string name);
	spreadsheet(const spreadsheet &sheet);
	// Can't overload with the same parameter type...
	// gonna need an object or something
	// spreadsheet(std::string JSON_Data);
	~spreadsheet();

	const std::vector<std::string> getCellDependencies(const std::string &cellName) const;
	const std::string getCellContents(const std::string &cellName) const;
	const std::vector<std::string> getAllCellNames() const;
	const std::string getName() const;
	bool setCellContents(const std::string &cellName, const std::string &contents, std::vector<std::string> const &dependencies);
	void saveSpreadsheet();
	bool revertCell(const std::string &cellName);
	UNDO_STATUS undo(std::string &cellName);
	bool getSaveStatus();
	void setName(std::string name);
	std::stack<cell_data> get_cell_history(std::string &cellName);
	std::stack<cell_data> get_edits();

	// std::stack<std::string> get_cell_history(std::string cellName);
};

#endif
