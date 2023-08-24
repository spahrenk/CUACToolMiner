#pragma once

#include <iostream>
#include <vector>

namespace CUACToolMiner {

enum Error
{
	NO_ERROR = 0,
	PROJECT_NOT_FOUND = 1,
	TABLE_NOT_FOUND = 2,
	NO_PROJECT_DATA = 3
};

typedef std::vector<std::vector<std::string>> Table;
typedef std::vector<std::tuple<std::string, int, Table>> Project;

extern bool quiet;

int getProjectList(std::string sDBPathFilename, std::vector<std::string>& vsProjects);
int getProjectData(
	const std::string& sDBPathFilename,
	const std::string& sProjectName, 
	const std::vector<std::string> &sTableNames,
	Project &project
);

} // namespace CUACToolMiner