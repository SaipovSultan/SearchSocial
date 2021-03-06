#pragma once

#include "iterator_range.h"

#include <string_view>
#include <sstream>
#include <vector>
using namespace std;

template <typename Container>
string Join(char c, const Container& cont) {
    ostringstream os;
    for (const auto& item : Head(cont, cont.size() - 1)) {
        os << item << c;
    }
    os << *rbegin(cont);
    return os.str();
}
vector<string_view> SplitBy(string_view s, char sep);
string_view Strip(string_view s);
void StripSv(string_view& s);
vector<string_view> SplitIntoWords(const string& str, char sep = ' ');