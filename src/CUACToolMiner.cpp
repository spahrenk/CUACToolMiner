
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

#include <nanodbc/nanodbc.h>

#include "CUACToolMiner.h"

using namespace nanodbc;

namespace CUACToolMiner {

bool quiet(false);

string fix_sql_string(const string &sIn)
{
    string sOut = sIn;
    if (sOut.find(" ") != std::string::npos)
        sOut = "[" + sOut + "]";
    return sOut;
}

bool read_col_names(connection &conn, string sTable_name, std::vector<string> &col_names)
{
    try{
        sTable_name = fix_sql_string(sTable_name);
        string command = "SELECT * FROM " + sTable_name;
        result data = execute(conn, NANODBC_TEXT(command));

        long nCols = data.columns();
        for (int i = 0; i < nCols; ++i)
        {
            std::string s(data.column_name(i));
            col_names.push_back(s);
        }
        return true;
    }
    catch (database_error const& e)
    {
        return false;
    }
}

template <typename T>
void read_rows_from_col(connection &conn, string sTable_name, string sCol_name, std::vector<T> &rows)
{
    sCol_name = fix_sql_string(sCol_name);
    sTable_name = fix_sql_string(sTable_name);

    string command = "SELECT " + sCol_name + " FROM " + sTable_name;
    result row = execute(conn, NANODBC_TEXT(command));
    for (int i = 1; row.next(); ++i)
    {
        auto s = row.get<T>(0,"");
        rows.push_back(s);
    }
}

bool read_projectID(connection &conn, const string &sProjectName, long &project_key)
{
    int res = 0;
    try
    {
        std::vector<string> vsCols;
        read_col_names(conn, "tProjects", vsCols);
  
        std::vector<string> vsProjects;
        read_rows_from_col(conn, "tProjects", "Project Name", vsProjects);
        long iKey = 0;
        bool found = false;
        for (auto &project: vsProjects) 
        {
            if (project == sProjectName)
            {
                found = true;
                break;
            }
            ++iKey;
        }
        if (!found) {return false;}

        std::vector<string> vsKey;
        read_rows_from_col(conn, "tProjects", "ProjectID", vsKey);
        project_key = std::stoi(vsKey[iKey]);
 
        if (!quiet) std::cout << "Key for " << sProjectName << " is " << project_key << std::endl;
    }
    catch (database_error const& e)
    {
       return false;
    };
    return true;
}

int read_table_project_data(
    connection &conn,
    const string &sTable_name,
    const long project_key,
    Table &table
)
{
    try
    {

        bool has_projID(false);
       
        std::vector<string> col_names;
        std::vector<string> row;
        if (read_col_names(conn, sTable_name, col_names)) {
            for (auto &sCol: col_names)
            {
                if (sCol == "ProjectID")
                {
                    has_projID = true;
                }
                else
                   row.push_back(sCol);
            }
            if (has_projID)
            {
                table.push_back(row); // column labels

                std::vector<std::string> vsKey;
                read_rows_from_col(conn, sTable_name, "ProjectID", vsKey);

                // Display rows referring to project_key
                long iKey(0);
                bool first(true);
                for (auto &sKey: vsKey) 
                {
                    if (sKey != "")
                    {
                        long keyp = std::stoi(sKey);
                        if (keyp == project_key)
                        {
                            row.clear();
                            for (auto &sColName: col_names)
                            {
                                if (sColName != "ProjectID")
                                {
                                    std::vector<string> col;
                                    read_rows_from_col(conn, sTable_name, sColName, col);
                                    auto sEntry = col[iKey]; 
                                    row.push_back(sEntry);
                                    first = false;
                                }

                            }
                            table.push_back(row);
                        }
                    }
                    ++iKey;
                }
            }
            else
            {
                return NO_PROJECT_DATA;
            }
        }
        else
        {
            return TABLE_NOT_FOUND;
        }   
    }
    catch (database_error const& e)
    {
        return static_cast<int>(e.native());
    }
    return 0;
}


int getProjectList(string sDBPathFilename, std::vector<string>& vsProjects)
{
    int res = 0;
    try
    {
        string full_arg = "Driver={Microsoft Access Driver (*.mdb, *.accdb)};Dbq=" + sDBPathFilename + ";";
        auto const connection_string = NANODBC_TEXT(full_arg);
        connection conn(connection_string);

        auto name = conn.dbms_name();
        if (!quiet) std::cout << "Connection to " << name << " database named " << sDBPathFilename << "\n";

        read_rows_from_col(conn, "tProjects", "Project Name", vsProjects);
        res = static_cast<int>(vsProjects.size());
    }
    catch (database_error const& e)
    {
        res = static_cast<int>(e.native());
    }
    return res;
}

int getProjectData(
	const std::string& sDBPathFilename,
	const std::string& sProjectName, 
	const std::vector<std::string> &sTableNames, 
	Project& project
)
{
    int res = NO_ERROR;
    try {
        string full_arg = "Driver={Microsoft Access Driver (*.mdb, *.accdb)};Dbq=" + sDBPathFilename + ";";
        auto const connection_string = NANODBC_TEXT(full_arg);
        connection conn(connection_string);
 
        auto name = conn.dbms_name();
        if (!quiet) std::cout << "Connection to " << name << " database named " << sDBPathFilename << "\n";

        long project_key(-1);
        if (!read_projectID(conn, sProjectName, project_key))
        {
            res = PROJECT_NOT_FOUND;
        }
        else
        {
            for (auto &sTable_name: sTableNames)
            {
                Table table;
                res = read_table_project_data(conn, sTable_name, project_key, table);
                if (res < 0)
                    break;
                project.push_back({sTable_name, res, table});
            }
        }
    }
    catch (database_error const& e)
    {
        res = static_cast<int>(e.native());
    }


    return res;
}
}