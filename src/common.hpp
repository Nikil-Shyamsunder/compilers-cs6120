#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

bool has_dest(const json& instr);
std::vector<std::string> get_args(const json& instr);

std::vector<std::vector<json>> gen_basic_blocks(const json& function);
std::string get_label(const std::vector<std::vector<json>>& basic_blocks, const std::string& func, size_t idx);

void replace_func_instrs(json& func, const std::vector<std::vector<json>>& blocks);

// Value struct for LVN
struct value {
    std::string op;
    std::vector<std::string> vals;

    bool operator==(const value& other) const {
        return op == other.op && vals == other.vals;
    }
    bool operator<(const value& other) const {
        if (op != other.op) return op < other.op;
        return vals < other.vals;
    }

    static value from_json(const json& j);
};
