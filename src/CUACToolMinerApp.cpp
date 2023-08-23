//#include "example_unicode_utils.h"
#include <nanodbc/nanodbc.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

using namespace std;

using namespace nanodbc;

const string db_folder("C:/Users/Phil Ahrenkiel/Documents/Development/SAC/CUACTool-Rev2-0-0 2");
const string main_db_name("CUACTool-Rev2-0-0.accdb");
const string lookup_db_name("CUACTool_LookupTables.accdb");

const string output_path("C:/Users/Phil Ahrenkiel/Documents/Development/SAC");
std::vector<string> tables_to_export({"tProjects", "tVersion", "tInputApt", "tInputCECPV", "tVNM"});

const string project_name("Lo Project");

bool quiet(false);
namespace fs = std::filesystem;

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

        if (!quiet) std::cout << std::endl << std::endl;
        if (!quiet) std::cout << "Table: " << sTable_name << std::endl;

        bool has_projID(false);
        std::vector<std::vector<string>> sTable;
        
        std::vector<string> col_names;
        std::vector<string> sRow;
        if (read_col_names(conn, sTable_name, col_names)) {
            for (auto &sCol: col_names)
            {
                if (sCol == "ProjectID")
                {
                    has_projID = true;
                }
                else
                   sRow.push_back(sCol);
            }
            if (has_projID)
            {
                sTable.push_back(sRow); // column labels

                vector<string> vsKey;
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
                            sRow.clear();
                            for (auto &sCol: col_names)
                            {
                                if (sCol != "ProjectID")
                                {
                                    std::vector<string> sColData;
                                    read_rows_from_col(conn, sTable_name, sCol, sColData);
                                    auto sEntry = sColData[iKey]; 
                                    sRow.push_back(sEntry);
                                    first = false;
                                }

                            }
                            sTable.push_back(sRow);
                        }
                    }
                    ++iKey;
                }
            }
            else
            {
                if (!quiet) std::cout << "No related entries." << std::endl;
            }
        }
        else
        {
            if (!quiet) std::cout << "Table not found." << std::endl;
            return 1;
        }

        // Write data
        if (has_projID)
        {
            ofstream outfile;
            outfile.open(sProjDataPathFilename);
            for (auto &sRow: sTable)
            {
                bool first(true);
                for (auto &sCol: sRow)
                {
                    if (!first)
                    {   outfile << ",";                   
                        if (!quiet) std::cout << ", ";
                    }
                    outfile << sCol;
                    if (!quiet) std::cout << sCol;
                    first = false;
                }
                outfile << std::endl;
                std::cout << std::endl;
            }
            outfile.close();
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
    long project_key(-1);
    if (!getProjectID(sDBPathFilename, sProjectName, project_key))
        return -1;

    auto sProjectPath = output_path + "/" + sProjectName;
    std::error_code ec;
    int err = 0;
    auto res = fs::create_directory(sProjectPath, ec);
    if (ec)
    {
        std::cout << "Error creating/finding directory: " << ec.message() << std::endl;
        err = ec.value();
    }
    else
    {
        for (auto sTable_name: tables_to_export)
        {
            string sProjDataPathFilename = sProjectPath + "/" + sTable_name + ".csv";
            err = getTableProjectData(sDBPathFilename, sTable_name, sProjDataPathFilename, project_key);
            if (err != 0)
                break;
        }
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

    res = getProjectData(sDBPathFilename, project_name, output_path);

    std::cout << std::endl;
    if (res < 0)
        std::cout << "Error: " << res <<std::endl;
    else
        std::cout << "No errors." << std::endl;



}
