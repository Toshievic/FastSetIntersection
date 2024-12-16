#include "../include/labeled_graph_new.hpp"
#include "../include/util_new.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;


void binary_read(std::string &data_filepath, unsigned *&array, int buffer_size) {
    std::ifstream fin(data_filepath, std::ios::in|std::ifstream::ate|std::ios::binary);
    if (!fin) {
        std::cerr << "Cannot open file: " << data_filepath << std::endl;
        exit(1);
    }
    size_t num_elems = static_cast<size_t>(fin.tellg()) / sizeof(unsigned);
    array = new unsigned[num_elems];
    
    fin.seekg(0);
    unsigned current_ptr = 0;
    bool at_end = false;
    while (!at_end) {
        unsigned num_block = buffer_size;
        if (current_ptr + buffer_size > num_elems) {
            num_block = num_elems - current_ptr;
            at_end = true;
        }
        fin.read(reinterpret_cast<char*>(array+current_ptr), num_block*sizeof(unsigned));
        current_ptr += buffer_size;
    }
    fin.close();
}


LabeledGraph::LabeledGraph(std::string &data_dirpath) {
    // data_dirpathが存在するか確認
    if (!fs::exists(data_dirpath) || !fs::is_directory(data_dirpath)) {
        std::cerr << "PathError: input_dirpath '" << data_dirpath << "' is not exist." << std::endl;
        exit(1);
    }

    // graph_infoを読み込む
    std::string info_filepath = data_dirpath + "/data_info.txt";
    std::ifstream fin_info(info_filepath, std::ios::in);
    if (!fin_info) {
        std::cerr << "Cannot open file: " << info_filepath << std::endl;
        exit(1);
    }
    std::string line_buf;
    while (getline(fin_info, line_buf)) {
        // data_info.txtの各行は key,value の形となっている
        std::vector<std::string> key_value = split(line_buf, ',');
        graph_info[key_value[0]] = stoi(key_value[1]);
    }

    // scan_crs, 隣接リストの読み込み
    std::string scan_key_filepath = data_dirpath + "/scan_key.bin";
    std::string scan_crs_filepath = data_dirpath + "/scan_crs.bin";
    std::string al_key_filepath = data_dirpath + "/al_key.bin";
    std::string al_crs_filepath = data_dirpath + "/al_crs.bin";
    std::string al_hub_key_filepath = data_dirpath + "/al_hub_key.bin";
    std::string al_hub_crs_filepath = data_dirpath + "/al_hub_crs.bin";
    binary_read(scan_key_filepath, scan_keys, 1024);
    binary_read(scan_crs_filepath, scan_crs, 1024);
    binary_read(al_key_filepath, al_keys, 1024);
    binary_read(al_crs_filepath, al_crs, 1024);
    binary_read(al_hub_key_filepath, al_hub_keys, 1024);
    binary_read(al_hub_crs_filepath, al_hub_crs, 1024);
}


LabeledAggGraph::LabeledAggGraph(std::string &data_dirpath) {
    // data_dirpathが存在するか確認
    if (!fs::exists(data_dirpath) || !fs::is_directory(data_dirpath)) {
        std::cerr << "PathError: input_dirpath '" << data_dirpath << "' is not exist." << std::endl;
        exit(1);
    }

    // graph_infoを読み込む
    std::string info_filepath = data_dirpath + "/data_info.txt";
    std::ifstream fin_info(info_filepath, std::ios::in);
    if (!fin_info) {
        std::cerr << "Cannot open file: " << info_filepath << std::endl;
        exit(1);
    }
    std::string line_buf;
    while (getline(fin_info, line_buf)) {
        // data_info.txtの各行は key,value の形となっている
        std::vector<std::string> key_value = split(line_buf, ',');
        graph_info[key_value[0]] = stoi(key_value[1]);
    }
    
    std::string scan_key_filepath = data_dirpath + "/scan_key.bin";
    std::string scan_crs_filepath = data_dirpath + "/scan_crs.bin";
    std::string agg2hop_key_filepath = data_dirpath + "/agg_2hop_key.bin";
    std::string agg2hop_crs_filepath = data_dirpath + "/agg_2hop_crs.bin";
    std::string agg_al_key_filepath = data_dirpath + "/agg_al_key.bin";
    std::string agg_al_crs_filepath = data_dirpath + "/agg_al_crs.bin";
    std::string al_key_filepath = data_dirpath + "/al_key.bin";
    std::string al_crs_filepath = data_dirpath + "/al_crs.bin";
    std::string al_hub_key_filepath = data_dirpath + "/al_hub_key.bin";
    std::string al_hub_crs_filepath = data_dirpath + "/al_hub_crs.bin";

    binary_read(scan_key_filepath, scan_keys, 1024);
    binary_read(scan_crs_filepath, scan_crs, 1024);
    binary_read(agg2hop_key_filepath, agg_2hop_keys, 1024);
    binary_read(agg2hop_crs_filepath, agg_2hop_crs, 1024);
    binary_read(agg_al_key_filepath, agg_al_keys, 1024);
    binary_read(agg_al_crs_filepath, agg_al_crs, 1024);
    binary_read(al_key_filepath, al_keys, 1024);
    binary_read(al_crs_filepath, al_crs, 1024);
    binary_read(al_hub_key_filepath, al_hub_keys, 1024);
    binary_read(al_hub_crs_filepath, al_hub_crs, 1024);
}
