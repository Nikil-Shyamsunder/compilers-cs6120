#include "common.hpp"
#include <algorithm>

bool has_dest(const json& instr) {
    return instr.contains("dest") && instr["dest"].is_string();
}

std::vector<std::string> get_args(const json& instr) {
    std::vector<std::string> out;
    if (instr.contains("args") && instr["args"].is_array()) {
        out.reserve(instr["args"].size());
        for (const auto& a : instr["args"]) out.push_back(a.get<std::string>());
    }
    return out;
}

std::vector<std::vector<json>> gen_basic_blocks(const json& function) {
    std::vector<std::vector<json>> basic_blocks;
    std::vector<json> curr_block;

    for (const auto& instr : function.at("instrs")) {
        bool has_label = instr.contains("label");
        bool has_op = instr.contains("op");
        bool is_term = has_op && (instr.at("op") == "br" || instr.at("op") == "jmp" || instr.at("op") == "ret");

        if (has_label) {
            if (!curr_block.empty()) basic_blocks.push_back(curr_block);
            curr_block.clear();
            curr_block.push_back(instr);
        } else if (is_term) {
            curr_block.push_back(instr);
            basic_blocks.push_back(curr_block);
            curr_block.clear();
        } else {
            curr_block.push_back(instr);
        }
    }
    if (!curr_block.empty()) basic_blocks.push_back(curr_block);
    return basic_blocks;
}

cfg_info build_cfg(const std::string& func_name, const std::vector<std::vector<json>>& basic_blocks) {
    cfg_info cfg;
    
    // Initialize all labels in both maps
    for (size_t i = 0; i < basic_blocks.size(); ++i) {
        std::string label = get_label(basic_blocks, func_name, i);
        cfg.successors[label] = std::vector<std::string>();
        cfg.predecessors[label] = std::vector<std::string>();
    }
    
    for (size_t i = 0; i < basic_blocks.size(); ++i) {
        const auto& basic_block = basic_blocks[i];
        std::string label = get_label(basic_blocks, func_name, i);
        
        std::vector<std::string> next;
        
        if (basic_block.empty() || !basic_block.back().contains("op")) {
            // successors are just the next block if it exists
            if (i + 1 < basic_blocks.size()) {
                next.push_back(get_label(basic_blocks, func_name, i + 1));
            }
        } else {
            std::string op = basic_block.back().at("op").get<std::string>();
            
            if (op == "br" || op == "jmp") { // branch or jump
                if (basic_block.back().contains("labels") && basic_block.back()["labels"].is_array()) {
                    for (const auto& target_label : basic_block.back()["labels"]) {
                        next.push_back(target_label.get<std::string>());
                    }
                }
            } else if (op == "ret") { // next should stay empty for return instructions
            } else if (i + 1 < basic_blocks.size()) { // just go to the next block
                next.push_back(get_label(basic_blocks, func_name, i + 1));
            }
        }
        
        cfg.successors[label] = next;
        
        // update predecessors of all successors adding this block
        for (const auto& successor : next) {
            cfg.predecessors[successor].push_back(label);
        }
    }
    
    return cfg;
}


std::string get_label(const std::vector<std::vector<json>>& basic_blocks, const std::string& func, size_t idx) {
    const auto& block = basic_blocks.at(idx);
    if (!block.empty() && block.front().contains("label")) {
        return block.front().at("label").get<std::string>();
    }
    return func + "-block" + std::to_string(idx);
}

void replace_func_instrs(json& func, const std::vector<std::vector<json>>& blocks) {
    func["instrs"] = json::array();
    for (const auto& b : blocks) {
        for (const auto& instr : b) func["instrs"].push_back(instr);
    }
}

value value::from_json(const json& j) {
    value v;
    if (j.contains("op")) v.op = j.at("op").get<std::string>();
    if (j.contains("args")) {
        for (const auto& arg : j.at("args")) v.vals.push_back(arg.get<std::string>());
    } else if (j.contains("value")) {
        // Treat constants as (op="const", vals=[literal])
        v.op = "const";
        if (j["value"].is_number_integer()) {
            v.vals.push_back(std::to_string(j.at("value").get<int>()));
        } else if (j["value"].is_number_float()) {
            v.vals.push_back(std::to_string(j.at("value").get<double>()));
        } else if (j["value"].is_boolean()) {
            v.vals.push_back(j.at("value").get<bool>() ? "true" : "false");
        } else {
            v.vals.push_back(j.at("value").dump());
        }
    }
    return v;
}
