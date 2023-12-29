#pragma once

#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>

using namespace std;

/*
* 以表格的形式构建字符串，实现列之间的对齐
*/
class StringTable {
private:
    vector<vector<string>> table;
    vector<size_t> column_widths;

public:
    void Add(initializer_list<string> strings) {
        table.push_back(vector<string>(strings));
        UpdateColumnWidths(table.back());
    }

    string ToString() {
        stringstream stream;

        for (const vector<string>& row : table) {
            size_t colSize = row.size();
            for (size_t colIndex = 0; colIndex < colSize; colIndex++) {
                const string& str = row[colIndex];
                stream << str;
                if (colIndex != colSize - 1) {
                    size_t spaceCount = column_widths[colIndex] - str.size() + 1;
                    stream << std::setw(spaceCount) << std::left << "";
                }
            }
            stream << "\n";
        }
        return stream.str();
    }

    void Clear()
    {
        table.clear();
        column_widths.clear();
    }

private:
    void UpdateColumnWidths(const vector<string>& row) {
        if (column_widths.size() < row.size()) {
            column_widths.resize(row.size(), 0);
        }

        for (size_t i = 0; i < row.size(); ++i) {
            column_widths[i] = max(column_widths[i], row[i].length());
        }
    }
};

