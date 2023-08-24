
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include "CUACToolMiner.h"

using namespace CUACToolMiner;

const std::string db_folder("C:/CUAC");
const std::string main_db_name("CUACTool-Rev2-0-0.accdb");
const std::string lookup_db_name("CUACTool_LookupTables.accdb");

const std::string output_path("C:/Users/Phil Ahrenkiel/Documents/Development/SAC");
std::vector<std::string> tables_to_export({"tProjects", "tVersion", "tInputApt", "tInputCECPV", "tVNM"});

const std::string project_name("Lo Project");

int main(int /*argc*/, char* /*argv*/[])
{
    std::string sDBPathFilename = db_folder + "/" + main_db_name;

    quiet = false;

    // List all projects
    {
        std::vector<std::string> vsProjects;
        auto res = getProjectList(sDBPathFilename, vsProjects);
        if (res < 0)
            std::cout << "Error: " << res <<std::endl;
        else
        {
            std::cout << "Database contains " << res << " project(s)." << std::endl;
            for (auto& project : vsProjects)
                std::cout << project << std::endl;
        }
        std::cout << std::endl;
    }
    // Get data for specific project
    { 
        Project project;
        auto res = getProjectData(sDBPathFilename, project_name, tables_to_export, project);

        // Create project folder
        auto sProjectPath = output_path + "/" + project_name;
        std::error_code ec;
        int err = 0;
         res = std::filesystem::create_directory(sProjectPath, ec);
        if (ec)
        { // An error occured
            std::cout << "Error creating/finding directory: " << ec.message() << std::endl;
            err = ec.value();
        }
        else
        { // Write project csv files
     
            for (auto &table_info: project)
            {
                std::cout << std::endl;
                std::cout << "Table: " << std::get<0>(table_info) << std::endl;
                switch (std::get<1>(table_info))
                {
                    case PROJECT_NOT_FOUND:
                    std::cout << "Project not found." << std::endl;
                    continue;

                    case TABLE_NOT_FOUND:
                    std::cout << "Table not found." << std::endl;
                    continue;

                    case NO_PROJECT_DATA:
                    std::cout << "No project data." << std::endl;
                    continue;

                    case NO_ERROR: 
                    {
                        std::ofstream outfile;
                        std::string sProjDataPathFilename = sProjectPath + "/" + std::get<0>(table_info) + ".csv";
                        outfile.open(sProjDataPathFilename);
                        for (auto &row: std::get<2>(table_info))
                        {
                            bool first(true);
                            for (auto &col: row)
                            {
                                if (!first)
                                {
                                    outfile << ",";                   
                                    if (!quiet) std::cout << ", ";
                                }
                                outfile << col;
                                if (!quiet) std::cout << col;
                                first = false;
                            }
                            outfile << std::endl;
                            std::cout << std::endl;
                        }
                        outfile.close();
                    }
                }
            }
        }

        std::cout << std::endl;
        if (res < 0)
            std::cout << "Error: " << res << std::endl;
        else
            std::cout << "No errors." << std::endl;
    }
}
