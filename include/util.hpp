#ifndef UTIL_HPP
#define UTIL_HPP

#include <vector>
#include <set>
#include <unordered_map>
#include <map>
#include <string>
#include <chrono>
#include <fstream>
#include <iostream>

#define NONE 4294967295

using namespace std;

typedef chrono::high_resolution_clock::time_point Chrono_t;

// related file
vector<string> get_file_paths(string folder_path);
void export_summaries(vector<unordered_map<string,unsigned>> &summaries, string output_file_path);
void export_summaries(vector<tuple<unsigned,unsigned,unsigned>> &summaries, string output_file_path);
void export_summaries(vector<unsigned*> &summaries, string output_file_path);
vector<string> read_block(ifstream &ifs_file, char delim);
vector<vector<unsigned>> read_csv(ifstream &ifs_file, char delim);
vector<vector<string>> read_table(ifstream &ifs_file, char delim);

// string manipulation
vector<string> split(string str, char delim);
template <class T>
string join(vector<T> &v, string delim);

// related vector
template <class T>
vector<T> slice(vector<T> v, int start, int end);
template <class T>
bool contains(vector<T> &v, T value);
// related map
template <class T, class U>
vector<T> sort_values(unordered_map<T, U>& M);

// related time
string get_timestamp();
Chrono_t get_time();
double get_msec_runtime(const Chrono_t *start_time, const Chrono_t *end_time);
double get_sec_runtime(const Chrono_t *start_time, const Chrono_t *end_time);

// print
template <typename T>
void print(vector<T> &v);
template <typename T>
void print(const vector<T> &v);
template <typename T, typename U>
void print(unordered_map<T,U> &m);
template <typename T, typename U>
void print(const unordered_map<T,U> &m);
template <typename T, typename U>
void print(map<T,U> &m);
template <typename T>
void print(set<T> &s);

string to_string(string s);
template <typename T>
string to_string(vector<T> &v);
template <typename T>
string to_string(set<T> &s);
template <typename T>
string to_string(const set<T> &s);
template <typename T>
string to_string(const vector<T> &v);
template <typename T, typename U>
string to_string(map<T,U> &m);
template <typename T, typename U>
string to_string(unordered_map<T,U> &m);
template <typename T, typename U>
string to_string(const unordered_map<T,U> &m);

#endif
