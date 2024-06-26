/*
* This file is part of the Pandaria 5.4.8 Project. See THANKS file for Copyright information
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DBCSTORE_H
#define DBCSTORE_H

#include "DBCFileLoader.h"
#include "DBStorageIterator.h"
#include "Log.h"
#include "Field.h"
#include "DatabaseWorkerPool.h"
#include "Implementation/WorldDatabase.h"
#include "DatabaseEnv.h"

struct SqlDbc
{
    std::string const* formatString;
    std::string const* indexName;
    std::string sqlTableName;
    int32 indexPos;
    int32 sqlIndexPos;
    SqlDbc(std::string const* _filename, std::string const* _format, std::string const* _idname, char const* fmt)
        : formatString(_format), indexName (_idname), sqlIndexPos(0)
    {
        // Convert dbc file name to sql table name
        sqlTableName = *_filename;
        for (uint32 i = 0; i< sqlTableName.size(); ++i)
        {
            if (isalpha(sqlTableName[i]))
                sqlTableName[i] = char(tolower(sqlTableName[i]));
            else if (sqlTableName[i] == '.')
                sqlTableName[i] = '_';
        }

        // Get sql index position
        DBCFileLoader::GetFormatRecordSize(fmt, &indexPos);
        if (indexPos >= 0)
        {
            uint32 uindexPos = uint32(indexPos);
            for (uint32 x = 0; x < formatString->size(); ++x)
            {
                // Count only fields present in sql
                if ((*formatString)[x] == FT_SQL_PRESENT)
                {
                    if (x == uindexPos)
                        break;
                    ++sqlIndexPos;
                }
            }
        }
    }
};

template<class T>
class DBCStorage
{
    friend void LoadDBCStores(const std::string&, uint32& availableDbcLocales);
    typedef std::vector<char*> StringPoolList;
    typedef DBStorageIterator<T> iterator;
    public:
        explicit DBCStorage(char const* f)
            : fmt(f), nCount(0), fieldCount(0), dataTable(nullptr)
        {
            indexTable.asT = nullptr;
        }

        ~DBCStorage() 
        { 
            delete[] reinterpret_cast<char*>(indexTable.asT); 
        }

        T const* LookupEntry(uint32 id) const
        {
            return (id >= nCount) ? nullptr : indexTable.asT[id];
        }
        T const* AssertEntry(uint32 id) const { return ASSERT_NOTNULL(LookupEntry(id)); }

        uint32  GetNumRows() const { return nCount; }
        char const* GetFormat() const { return fmt; }
        uint32 GetFieldCount() const { return fieldCount; }

        bool Load(char const* fn, SqlDbc* sql, LocaleConstant locale)
        {
            DBCFileLoader dbc;
            // Check if load was sucessful, only then continue
            if (!dbc.Load(fn, fmt))
                return false;

            uint32 sqlRecordCount = 0;
            uint32 sqlHighestIndex = 0;
            Field* fields = nullptr;
            QueryResult result = QueryResult(nullptr);
            // Load data from sql
            if (sql)
            {
                std::string query = "SELECT * FROM " + sql->sqlTableName;
                if (sql->indexPos >= 0)
                    query +=" ORDER BY " + *sql->indexName + " DESC";
                query += ';';


                result = WorldDatabase.Query(query.c_str());
                if (result)
                {
                    sqlRecordCount = uint32(result->GetRowCount());
                    if (sql->indexPos >= 0)
                    {
                        fields = result->Fetch();
                        sqlHighestIndex = fields[sql->sqlIndexPos].GetUInt32();
                    }

                    // Check if sql index pos is valid
                    if (int32(result->GetFieldCount() - 1) < sql->sqlIndexPos)
                    {
                        TC_LOG_ERROR("server.loading", "Invalid index pos for dbc:'%s'", sql->sqlTableName.c_str());
                        return false;
                    }
                }
            }

            char* sqlDataTable = nullptr;
            fieldCount = dbc.GetCols();

            dataTable = reinterpret_cast<T*>(dbc.AutoProduceData(fmt, nCount, indexTable.asChar,
                sqlRecordCount, sqlHighestIndex, sqlDataTable));

            stringPoolList.push_back(dbc.AutoProduceStrings(fmt, reinterpret_cast<char*>(dataTable), locale));

            // Insert sql data into arrays
            if (result)
            {
                if (indexTable.asT)
                {
                    uint32 offset = 0;
                    uint32 rowIndex = dbc.GetNumRows();
                    do
                    {
                        if (!fields)
                            fields = result->Fetch();

                        if (sql->indexPos >= 0)
                        {
                            uint32 id = fields[sql->sqlIndexPos].GetUInt32();
                            if (indexTable.asT[id])
                            {
                                TC_LOG_ERROR("server.loading", "Index %d already exists in dbc:'%s'", id, sql->sqlTableName.c_str());
                                return false;
                            }

                            indexTable.asT[id] = reinterpret_cast<T*>(&sqlDataTable[offset]);
                        }
                        else
                            indexTable.asT[rowIndex]= reinterpret_cast<T*>(&sqlDataTable[offset]);

                        uint32 columnNumber = 0;
                        uint32 sqlColumnNumber = 0;

                        for (; columnNumber < sql->formatString->size(); ++columnNumber)
                        {
                            if ((*sql->formatString)[columnNumber] == FT_SQL_ABSENT)
                            {
                                switch (fmt[columnNumber])
                                {
                                    case FT_FLOAT:
                                        offset += 4;
                                        break;
                                    case FT_IND:
                                    case FT_INT:
                                        offset += 4;
                                        break;
                                    case FT_BYTE:
                                        offset += 1;
                                        break;
                                    case FT_STRING:
                                        offset += sizeof(DbcStr);
                                        break;
                                }
                            }
                            else if ((*sql->formatString)[columnNumber] == FT_SQL_PRESENT)
                            {
                                bool validSqlColumn = true;
                                switch (fmt[columnNumber])
                                {
                                    case FT_FLOAT:
                                        *reinterpret_cast<float*>(&sqlDataTable[offset]) = fields[sqlColumnNumber].GetFloat();
                                        offset += 4;
                                        break;
                                    case FT_IND:
                                    case FT_INT:
                                        *reinterpret_cast<uint32*>(&sqlDataTable[offset]) = fields[sqlColumnNumber].GetUInt32();
                                        offset += 4;
                                        break;
                                    case FT_BYTE:
                                        *reinterpret_cast<uint8*>(&sqlDataTable[offset]) = fields[sqlColumnNumber].GetUInt8();
                                        offset += 1;
                                        break;
                                    case FT_STRING:
                                        TC_LOG_ERROR("server.loading", "Unsupported data type in table '%s' at char %d", sql->sqlTableName.c_str(), columnNumber);
                                        return false;
                                    case FT_SORT:
                                        break;
                                    default:
                                        validSqlColumn = false;
                                        break;
                                }
                                if (validSqlColumn && (columnNumber != (sql->formatString->size()-1)))
                                    sqlColumnNumber++;
                            }
                            else
                            {
                                TC_LOG_ERROR("server.loading", "Incorrect sql format string '%s' at char %d", sql->sqlTableName.c_str(), columnNumber);
                                return false;
                            }
                        }

                        if (sqlColumnNumber != (result->GetFieldCount() - 1))
                        {
                            TC_LOG_ERROR("server.loading", "SQL and DBC format strings are not matching for table: '%s'", sql->sqlTableName.c_str());
                            return false;
                        }

                        fields = nullptr;
                        ++rowIndex;
                    } while (result->NextRow());
                }
            }

            // error in dbc file at loading if NULL
            return indexTable.asT != nullptr;
        }

        bool LoadStringsFrom(char const* fn, LocaleConstant loc)
        {
            // DBC must be already loaded using Load
            if (!indexTable.asT)
                return false;

            DBCFileLoader dbc;
            // Check if load was successful, only then continue
            if (!dbc.Load(fn, fmt))
                return false;

            stringPoolList.push_back(dbc.AutoProduceStrings(fmt, reinterpret_cast<char*>(dataTable), loc));

            return true;
        }

        iterator begin() { return iterator(indexTable.asT, nCount); }
        iterator end() { return iterator(indexTable.asT, nCount, nCount); }

    private:
        char const* fmt;
        uint32 nCount;
        uint32 fieldCount;

        union
        {
            T** asT;
            char** asChar;
        }
        indexTable;

        T* dataTable;
        StringPoolList stringPoolList;
};

#endif