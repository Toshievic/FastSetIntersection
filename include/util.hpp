#ifndef UTIL_NEW_HPP
#define UTIL_NEW_HPP

#include <vector>
#include <string>
#include <chrono>

typedef std::chrono::high_resolution_clock::time_point Chrono_t;


std::vector<std::string> split(std::string &str, char delim);
Chrono_t get_time();
double get_runtime(const Chrono_t *start, const Chrono_t *end);

#endif