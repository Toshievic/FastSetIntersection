#include "../include/util_new.hpp"


std::vector<std::string> split(std::string &str, char delim) {
    int first = 0;
    int last = str.find_first_of(delim);

    std::vector<std::string> result;

    if ((last == std::string::npos) && (str != "")) {
        result.push_back(str);
        return result;
    }

    while (first < str.size()) {
        std::string sub_str(str, first, last - first);
        result.push_back(sub_str);

        first = last + 1;
        last = str.find_first_of(delim, first);

        if (last == std::string::npos) {
            last = str.size();
        }
    }
    return result;
}


Chrono_t get_time() {
    return std::chrono::high_resolution_clock::now();
}


double get_runtime(const Chrono_t *start, const Chrono_t *end) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(*end-*start).count() / 1000000.0;
}
