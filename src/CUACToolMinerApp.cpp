//#include "example_unicode_utils.h"
#include <nanodbc/nanodbc.h>

#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

using namespace nanodbc;

const string db_folder = "C:/Users/Phil Ahrenkiel/Documents/Development/SAC/CUACTool-Rev2-0-0 2";
const string main_db_name = "CUACTool-Rev2-0-0.accdb";
//const string main_db_name = "error.accdb";
const string lookup_db_name = "CUACTool_LookupTables.accdb";

const string output_path = "C:/Users/Phil Ahrenkiel/Documents/Development/SAC";
//const string output_filename = "project_data.csv";

std::vector<string> tables_to_export = {"tProjects", "tVersion", "tInputApt", "tInput CECPV", "tVNM"};

bool quiet = false;

string fix_sql_string(const string &sIn)
{
    string sOut = sIn;
    if (sOut.find(" ") != std::string::npos)
        sOut = "[" + sOut + "]";
    return sOut;
}

void read_cats(connection &conn, std::list<string> &cats)
{
    nanodbc::catalog cat(conn);
    cats = cat.list_catalogs();

}

void read_table_names(connection &conn, std::vector<string> &table_names)
{
    nanodbc::catalog cat(conn);
    nanodbc::catalog::tables tabl = cat.find_tables();
    long count = 0;
    while (tabl.next())
    {
        auto s = tabl.table_name();
        table_names.push_back(s);
        count++;
    }
}

void read_table_catalog(connection &conn, const string &sTable_name)
{
    nanodbc::catalog cat(conn);
    auto tbls = cat.find_tables(sTable_name);
   // auto tabl_cat = tbls.table_name();
    while (tbls.next()) {
        std::cout << tbls.table_name() << std::endl;
    }
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

void read_rows_from_cols(connection &conn, const string &sTable_name, const std::vector<string> &sCol_names, std::vector<std::vector<string>> &cols)
{
    for (auto &sCol_name: sCol_names)
    {
        std::vector<string> col;
        read_rows_from_col(conn, sTable_name, sCol_name, col);
        cols.push_back(col);
    }
}

int getProjectList(string sDBPathFilename, vector<string>& vsProjects)
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

int getTableNames(string sDBPathFilename, vector<string>& vsTables)
{
    int res = 0;
    try
    {
        string full_arg = "Driver={Microsoft Access Driver (*.mdb, *.accdb)};Dbq=" + sDBPathFilename + ";";
        auto const connection_string = NANODBC_TEXT(full_arg);
        connection conn(connection_string);

        auto name = conn.dbms_name();
        if (!quiet) std::cout << "Connection to " << name << " database named " << sDBPathFilename << "\n";

        read_table_names(conn, vsTables);

    }
    catch (database_error const& e)
    {
        res = static_cast<int>(e.native());
    };
    return res;
}

int getTableProjectData(const string &sDBPathFilename, const string &sTable_name,const string &sProjDataPathFilename, const long project_key)
{
    try
    {
        string full_arg = "Driver={Microsoft Access Driver (*.mdb, *.accdb)};Dbq=" + sDBPathFilename + ";";
        auto const connection_string = NANODBC_TEXT(full_arg);
        connection conn(connection_string);

        bool has_projID(false);
        std::vector<string> col_names;
        if (read_col_names(conn, sTable_name, col_names)) {
            for (auto &sCol: col_names)
            {
                if (sCol == "ProjectID")
                {
                    has_projID = true;
                }
            }
            if (!has_projID)
                return 0;

            if (!quiet) std::cout << std::endl;
            if (!quiet) std::cout << "Table: " << sTable_name << std::endl;
            bool first = true;
            for (auto &sCol: col_names) {
                if (sCol != "ProjectID")
                {
                    if (!first) {
                        if (!quiet) std::cout << ", ";
                        }
                    if (!quiet) std::cout << sCol;
                    first = false;
                }
            }
            if (!quiet) std::cout << std::endl;
            vector<string> vsKey;
            read_rows_from_col(conn, sTable_name, "ProjectID", vsKey);
            long iKey(0);
            first = true;
            for (auto &sKey: vsKey) 
            {
                if (sKey == "")
                    continue;
                long keyp = std::stoi(sKey);
                if (keyp == project_key)
                {
                    std::vector<string> sData;
                    for (auto &sCol: col_names)
                    {
                        if (sCol != "ProjectID")
                        {
                            std::vector<string> sColData;
                            read_rows_from_col(conn, sTable_name, sCol, sColData);
                            if (!first && !quiet) std::cout << ", ";
                            if (!quiet) std::cout << sColData[iKey];
                            first = false;
                        }

                    }
                    if (!quiet) std::cout << std::endl;
                }

            }
        }
    }
    catch (database_error const& e)
    {
        return static_cast<int>(e.native());
    }
    return 0;
 }

bool getProjectID(const string& sDBPathFilename,const string &sProjectName, long &project_key)
{
    int res = 0;
    try
    {
        string full_arg = "Driver={Microsoft Access Driver (*.mdb, *.accdb)};Dbq=" + sDBPathFilename + ";";
        auto const connection_string = NANODBC_TEXT(full_arg);
        connection conn(connection_string);

        auto name = conn.dbms_name();
        if (!quiet) std::cout << "Connection to " << name << " database named " << sDBPathFilename << "\n";

        vector<string> vsCols;
        read_col_names(conn, "tProjects", vsCols);
  
        vector<string> vsProjects;
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

        vector<string> vsKey;
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

int getProjectData(const string& sDBPathFilename,const string &sProjectName, const string &sProjDataPath)
{
    int res = 0;
    long project_key(-1);
    if (!getProjectID(sDBPathFilename, sProjectName, project_key))
        return -1;

    int err = 0;
    for (auto sTable_name: tables_to_export) {
        string sProjDataPathFilename = sProjectName + "/" + sTable_name + ".csv";
        err = getTableProjectData(sDBPathFilename, sTable_name, sProjDataPathFilename, project_key);
        if (err != 0)
            break;
    }
    return err;
}

int main(int /*argc*/, char* /*argv*/[])
{
    string sDBPathFilename = db_folder + "/" + main_db_name;

    quiet = false;
    /*
    int res = 0;
    try
    {
        string full_arg = "Driver={Microsoft Access Driver (*.mdb, *.accdb)};Dbq=" + sDBPathFilename + ";";
        auto const connection_string = NANODBC_TEXT(full_arg);
        connection conn(connection_string);

        std::vector<string> col_names;
        read_col_names(conn, "tAptTypes", col_names);
    }

    catch (database_error const& e)
    {
        res = static_cast<int>(e.native());
    };
    */
    vector<string> vsProjects;
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

    res = getProjectData(sDBPathFilename, "Lo Rev", output_path);
    if (res < 0)
        std::cout << "Error: " << res <<std::endl;
    else
        std::cout << "Successfully wrote data." << std::endl;



}
