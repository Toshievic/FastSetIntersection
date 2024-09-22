#include "../include/util.hpp"
#include "../include/labeled_graph.hpp"

#include <filesystem>
#include <sstream>


// related file ---------------------------------------------------------------

using namespace filesystem;

vector<string> get_file_paths(string folder_path) {
    vector<string> file_paths;

    directory_iterator itr(folder_path), end;
    error_code err;

    for (; itr != end && !err; itr.increment(err)) {
        const directory_entry entry = *itr;

        file_paths.push_back( entry.path().string() );
    }

    return file_paths;
};

void export_summaries(vector<unordered_map<string,string>> &summaries, string output_file_path) {
    // まずcolumnを特定
    unordered_map<string,int> columns;
    vector<string> inv_cols;
    int i = 0;
    for (auto &summary : summaries) {
        for (auto &stat : summary) {
            if (!columns.contains(stat.first)) {
                columns[stat.first] = i;
                inv_cols.push_back(stat.first);
                ++i;
            }
        }
    }

    ofstream output_csv(output_file_path);
    if (!output_csv.is_open()) { cerr << "cannot open file." << endl; }

    // write header
    for (int i=0; i<inv_cols.size()-1; i++) { output_csv << inv_cols[i] << ","; }
    output_csv << inv_cols.back() << endl;

    // write body
    for (auto &summary : summaries) {
        vector<string> tmp(inv_cols.size());
        for (auto &itr : summary) { tmp[columns[itr.first]] = itr.second; }
        for (int i=0; i<tmp.size()-1; i++) { output_csv << tmp[i] << ","; }
        output_csv << tmp.back() << endl;
    }

    output_csv.close();
}

void export_summaries(vector<tuple<unsigned,unsigned,unsigned>> &summaries, string output_file_path) {
    ofstream output_csv(output_file_path);
    if (!output_csv.is_open()) { cerr << "cannot open file." << endl; }
    // write header
    output_csv << "list_len_sum,match_count,runtime" << endl;

    for (int i=0; i<summaries.size(); ++i) {
        output_csv << to_string(get<0>(summaries[i])) << ","
        << to_string(get<1>(summaries[i])) << ","
        << to_string(get<2>(summaries[i])) << endl;
    }

    output_csv.close();
}

vector<string> read_block(ifstream &ifs_file, char delim) {
    string line_buf;
    vector<string> word_buf;

    getline(ifs_file, line_buf);
    istringstream i_stream(line_buf);
    word_buf = split(line_buf, delim);

    return word_buf;
};

vector<vector<unsigned>> read_csv(ifstream &ifs_file, char delim) {
    string line_buf;
    vector<string> word_buf;
    vector<vector<unsigned>> table;
    while (getline(ifs_file, line_buf)) {
        istringstream i_stream(line_buf);
        word_buf = split(line_buf, delim);
        vector<unsigned> tmp_v(word_buf.size());
        for (int i=0; i<word_buf.size(); ++i) {
            tmp_v[i] = stoul(word_buf[i]);
        }
        table.push_back(tmp_v);
    }

    return table;
};

vector<vector<string>> read_table(ifstream &ifs_file, char delim) {
    string line_buf;
    vector<string> word_buf;
    vector<vector<string>> table;
    while (getline(ifs_file, line_buf)) {
        istringstream i_stream(line_buf);
        word_buf = split(line_buf, delim);
        table.push_back(word_buf);
    }

    return table;
};

// string manipulation --------------------------------------------------------

vector<string> split(string str, char delim) {
    int first = 0;
    int last = str.find_first_of(delim);

    vector<string> result;

    if ((last == string::npos) && (str != "")) {
        result.push_back(str);
        return result;
    }

    while (first < str.size()) {
        string sub_str(str, first, last - first);
        result.push_back(sub_str);

        first = last + 1;
        last = str.find_first_of(delim, first);

        if (last == string::npos) {
            last = str.size();
        }
    }
    return result;
}

template <class T>
string join(vector<T> &v, string delim) {
    string str;
    if (v.size() > 0) {
        for (int i=0; i<v.size()-1; i++) { str += to_string(v[i]) + delim; }
        str += to_string(v.back());
    }

    return str;
};
template string join<int>(vector<int> &, string);
template string join<string>(vector<string> &, string);

// related vector -------------------------------------------------------------

template <class T>
vector<T> slice(vector<T> v, int start, int end) {
    if (start < 0) {
        start = 0;
    }
    if (end > v.size()) {
        end = v.size();
    }
    vector<T> new_v;
    for (int i=start; i<end; i++) {
        new_v.push_back(v.at(i));
    }
    return new_v;
}
template vector<long> slice<long>(vector<long>,int,int);
template vector<string> slice<string>(vector<string>,int,int);

template <class T>
bool contains(vector<T> &v, T value) {
    return find(v.begin(), v.end(), value) != v.end();
}
template bool contains<string>(vector<string> &,string);
template bool contains<int>(vector<int> &,int);

// related map ----------------------------------------------------------------

template <class T, class U>
vector<T> sort_values(unordered_map<T, U>& M) {
    // Declare vector of pairs 
    vector<pair<T, U> > A; 
 
    // Copy key-value pair from UnorderedMap 
    // to vector of pairs 
    for (auto& it : M) { 
        A.push_back(it); 
    } 
 
    // Sort using comparator function 
    sort(A.begin(), A.end(), [] (pair<T,U> &a, pair<T,U> &b) { return a.second < b.second; }); 
 
    // create sorted
    vector<T> sorted_keys;
    for (auto& it : A) {
        sorted_keys.push_back(it.first);
    } 
    return sorted_keys;
};
template vector<string> sort_values<string, double>(unordered_map<string,double> &);
template vector<string> sort_values<string, int>(unordered_map<string,int> &);

// get time -------------------------------------------------------------------

string get_timestamp() {
    auto now = chrono::system_clock::now();
    auto now_c = chrono::system_clock::to_time_t(now);
    stringstream ss;
    ss << put_time(localtime(&now_c), "%Y%m%d_%H%M%S");
    return ss.str();
}

Chrono_t get_time() {
    return chrono::high_resolution_clock::now();
}

double get_msec_runtime(const Chrono_t *start_time, const Chrono_t *end_time) {
    return static_cast<double>(
        chrono::duration_cast<chrono::microseconds>(*end_time - *start_time).count()/ 1000.0
    );
}

double get_sec_runtime(const Chrono_t *start_time, const Chrono_t *end_time) {
    return static_cast<double>(
        chrono::duration_cast<chrono::microseconds>(*end_time - *start_time).count()/ 1000000.0
    );
}

// print ----------------------------------------------------------------------

template <typename T>
void print(vector<T> &v) {
    cout << "[";
    if (v.size() == 0) {
        cout << "]" << endl;
    }
    else {
        for (int i=0; i<v.size()-1; i++) {
            cout << to_string(v[i]) << ", ";
        }
        cout << to_string(v.back()) << "]" << endl;
    }
}
template void print<int>(vector<int> &);
template void print<bool>(vector<bool> &);
template void print<string>(vector<string> &);
template void print<unsigned>(vector<unsigned> &);
template void print<float>(vector<float> &);
template void print<vector<string>>(vector<vector<string>> &);
template void print<vector<int>>(vector<vector<int>> &);
template void print<set<string>>(vector<set<string>> &);
template void print<unordered_map<string,set<string>>> (vector<unordered_map<string, set<string>>> &);

template <typename T>
void print(const vector<T> &v) {
    cout << "[";
    if (v.size() == 0) {
        cout << "]" << endl;
    }
    else {
        for (int i=0; i<v.size()-1; i++) {
            cout << to_string(v[i]) << ", ";
        }
        cout << to_string(v.back()) << "]" << endl;
    }
}
template void print<set<string>>(const vector<set<string>> &);
template void print<unsigned>(const vector<unsigned> &);
template void print<bool>(const vector<bool> &);
template void print<vector<string>>(const vector<vector<string>> &);
template void print<vector<unsigned>>(const vector<vector<unsigned>> &);
template void print<unordered_map<string,unsigned>>(const vector<unordered_map<string,unsigned>> &);
template void print<unordered_map<string, set<string>>>(const vector<unordered_map<string, set<string>>> &);

template <typename T, typename U>
void print(unordered_map<T,U> &m) {
    if (m.size() == 0) {
        cout << "{}" << endl;
        return;
    }
    cout << "{" << endl;
    for (auto itr=m.begin(); itr!=m.end(); ++itr) {
        cout << to_string(itr->first) << ":\t\t" << to_string(itr->second) << endl;
    }
    cout << "}" << endl;
}
template void print<string,string>(unordered_map<string,string> &);
template void print<string,int>(unordered_map<string,int> &);
template void print<int,unsigned>(unordered_map<int,unsigned> &);
template void print<int,int>(unordered_map<int,int> &);
template void print<int,double>(unordered_map<int,double> &);
template void print<string,unsigned>(unordered_map<string,unsigned> &);
template void print<string,double>(unordered_map<string,double> &);
template void print<string,vector<string>>(unordered_map<string,vector<string>> &);
template void print<string,set<string>>(unordered_map<string,set<string>> &);
template void print<string,vector<vector<unsigned>>>(unordered_map<string,vector<vector<unsigned>>> &);
template void print<string,map<unsigned,int>>(unordered_map<string,map<unsigned,int>> &);
template void print<string,unordered_map<string,vector<unsigned>>>(unordered_map<string,unordered_map<string,vector<unsigned>>> &);
template void print<string,unordered_map<string,float>>(unordered_map<string,unordered_map<string,float>> &);
template void print<string,map<unsigned,vector<unsigned>>>(unordered_map<string,map<unsigned,vector<unsigned>>> &);

template <typename T, typename U>
void print(const unordered_map<T,U> &m) {
    if (m.size() == 0) {
        cout << "{}" << endl;
        return;
    }
    cout << "{" << endl;
    for (auto itr=m.begin(); itr!=m.end(); ++itr) {
        cout << to_string(itr->first) << ":\t\t" << to_string(itr->second) << endl;
    }
    cout << "}" << endl;
}
template void print<string,int>(const unordered_map<string,int> &);
template void print<string,unsigned>(const unordered_map<string,unsigned> &);

template <typename T, typename U>
void print(map<T,U> &m) {
    if (m.size() == 0) {
        cout << "{}" << endl;
        return;
    }
    cout << "{" << endl;
    for (auto itr=m.begin(); itr!=m.end(); ++itr) {
        cout << to_string(itr->first) << ":\t\t" << to_string(itr->second) << endl;
    }
    cout << "}" << endl;
}
template void print<int, vector<string>>(map<int, vector<string>> &);
template void print<unsigned, vector<vector<string>>>(map<unsigned, vector<vector<string>>> &);
template void print<int,int>(map<int,int> &);
template void print<int,unsigned>(map<int,unsigned> &);
template void print<int,vector<int>>(map<int,vector<int>> &);
template void print<int, unordered_map<string,unsigned>>(map<int, unordered_map<string,unsigned>> &);

template <typename T>
void print(set<T> &s) {
    cout << "{";
    if (s.size() == 0) {
        cout << "}" << endl;
    }
    else {
        for (auto itr=s.begin(); itr!=s.end(); ++itr) {
            cout << to_string(*itr) << ", ";
        }
        cout << "}" << endl;
    }
};
template void print<string>(set<string> &);
template void print<int>(set<int> &);
template void print<vector<unsigned>>(set<vector<unsigned>> &);

// ----------------------------------------------------------------------------

string to_string(string s) { return s; }

template <typename T>
string to_string(vector<T> &v) {
    string str;
    str += "[";
    if (v.size() == 0) {
        str += "]";
    }
    else {
        for (int i=0; i<v.size()-1; i++) {
            str += to_string(v[i]) + ", ";
        }
        str += to_string(v.back()) + "]";
    }
    return str;
}
template string to_string<string>(vector<string> &);
template string to_string<unsigned>(vector<unsigned> &);
template string to_string<vector<unsigned>>(vector<vector<unsigned>> &);

template <typename T>
string to_string(set<T> &s) {
    string str;
    str += "{";
    if (s.size() == 0) {
        str += "}";
    }
    else {
        for (auto itr=s.begin(); itr!=s.end(); ++itr) {
            str += to_string(*itr) + ", ";
        }
        str += "}";
    }
    return str;
}
template string to_string<string>(set<string> &s);

template <typename T>
string to_string(const set<T> &s) {
    string str;
    str += "{";
    if (s.size() == 0) {
        str += "}";
    }
    else {
        for (auto itr=s.begin(); itr!=s.end(); ++itr) {
            str += to_string(*itr) + ", ";
        }
        str += "}";
    }
    return str;
}
template string to_string<string>(const set<string> &s);

template <typename T>
string to_string(const vector<T> &v) {
    string str;
    str += "[";
    if (v.size() == 0) {
        str += "]";
    }
    else {
        for (int i=0; i<v.size()-1; i++) {
            str += to_string(v[i]) + ", ";
        }
        str += to_string(v.back()) + "]";
    }
    return str;
}
template string to_string<unsigned>(const vector<unsigned> &);
template string to_string<float>(const vector<float> &);
template string to_string<string>(const vector<string> &);

template <typename T, typename U>
string to_string(map<T,U> &m) {
    string str;
    if (m.size() == 0) {
        str += "{}";
    }
    str += "{";
    for (auto itr=m.begin(); itr!=m.end(); ++itr) {
        str += "\t{" + to_string(itr->first) + ", " + to_string(itr->second) + "}\n"; 
    }
    str += "}";

    return str;
}
template string to_string<unsigned,int>(map<unsigned,int> &);
template string to_string<unsigned,vector<unsigned>>(map<unsigned,vector<unsigned>> &);

template <typename T, typename U>
string to_string(unordered_map<T,U> &m) {
    string str;
    if (m.size() == 0) {
        str += "{}";
    }
    str += "{";
    for (auto itr=m.begin(); itr!=m.end(); ++itr) {
        str += "\t{" + to_string(itr->first) + ", " + to_string(itr->second) + "}\n"; 
    }
    str += "}";

    return str;
};
template string to_string<string,vector<unsigned>>(unordered_map<string,vector<unsigned>> &);
template string to_string<string,float>(unordered_map<string,float> &);

template <typename T, typename U>
string to_string(const unordered_map<T,U> &m) {
    string str;
    if (m.size() == 0) {
        str += "{}";
    }
    str += "{";
    for (auto itr=m.begin(); itr!=m.end(); ++itr) {
        str += "\t{" + to_string(itr->first) + ", " + to_string(itr->second) + "}\n"; 
    }
    str += "}";

    return str;
}
template string to_string<string,unsigned>(const unordered_map<string,unsigned> &);
