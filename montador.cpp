#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <regex>
#include <stdexcept>

// instruction table
std::map<std::string, int> instruction_table = {{"ADD", 1},
                                                {"SUB", 2},
                                                {"MUL", 3},
                                                {"DIV", 4},
                                                {"JMP", 5},
                                                {"JMPN", 6},
                                                {"JMPP", 7},
                                                {"JMPZ", 8},
                                                {"COPY", 9},
                                                {"LOAD", 10},
                                                {"STORE", 11},
                                                {"INPUT", 12},
                                                {"OUTPUT", 13},
                                                {"STOP", 14}};

// arguments per instruction
std::map<std::string, int> argument_table = {{"ADD", 1},
                                             {"SUB", 1},
                                             {"MUL", 1},
                                             {"DIV", 1},
                                             {"JMP", 1},
                                             {"JMPN", 1},
                                             {"JMPP", 1},
                                             {"JMPZ", 1},
                                             {"COPY", 2},
                                             {"LOAD", 1},
                                             {"STORE", 1},
                                             {"INPUT", 1},
                                             {"OUTPUT", 1},
                                             {"STOP", 0}};

// directive table
std::map<std::string, int> directive_table = {{"CONST", 1},
                                              {"SPACE", 0},
                                              {"BEGIN", 0},
                                              {"END", 0},
                                              {"PUBLIC", 1},
                                              {"EXTERN", 0}};

/** @ parser for the compiler
 *
 * function longe
 * @param[in]  param1_name  description of parameter1
 * @param[in]  param2_name  description of parameter2
 * @return return_name return description
 */
std::map<std::string, std::string> parser(std::string line) {
    std::map<std::string, std::string> syntax_tree {{"label", ""},
                                                    {"inst", ""},
                                                    {"arg1", ""},
                                                    {"arg2", ""}};
    int op_idx = 0;

    //
    // LABEL
    //
    std::string::difference_type n = std::count(line.begin(), line.end(), ':');
    if (n==0) {
        syntax_tree["label"] = "";
    } else if (n==1) {
        op_idx = line.find(':')+1;
        // check if label is lexically correct
        const std::regex label_regex("^([A-Za-z_][A-Za-z0-9_]*) ?:$");
        std::string label = line.substr(0, line.find(':')+1);
        std::smatch match;

        if (std::regex_match(label, match, label_regex)) {
            label = match[1].str();
            syntax_tree["label"] = label;
        } else {
            throw std::runtime_error("Lexical error: bad label definition");
        }

    } else {
        // raise error
        throw std::runtime_error("Syntax error: more than one label defined in the line");
    }

    line = line.substr(op_idx);

    //
    // INSTRUCTION AND ARGUMENTS
    //
    const std::regex inst_args("^ *([A-Za-z_][A-Za-z0-9_]*)(?:(?: +(?:([A-Za-z_][A-Za-z0-9_]*) *,)? *([A-Za-z_][A-Za-z0-9_]*)?)|(?: +([0-9]+)))? *$");
    std::smatch line_match;
    int arg_count = 0;

    if (std::regex_match(line, line_match, inst_args)) {
        std::string token;
        std::string token_type;
        for (int i=1; i<line_match.size(); i++) {
            token = line_match[i].str();
            if (token.size() != 0) {
                switch(i) {
                    case 1:
                        // instruction
                        token_type = "inst";
                        break;
                    case 2:
                        // argument 1 (if there are two)
                        token_type = "arg1";
                        arg_count = 2;
                        break;
                    case 3:
                        // argument 2 (if there are two), argument 1 (if there is one)
                        if (arg_count==2) {
                            token_type = "arg2";
                        } else {
                            token_type = "arg1";
                            arg_count = 1;
                        }
                        break;
                    case 4:
                        // numerical argument
                        token_type = "arg1";
                        arg_count = 1;
                        break;
                }
                syntax_tree[token_type] = token;
            }
        }
    } else {
        throw std::runtime_error("Syntax error");
    }

    std::string inst = syntax_tree["inst"];

    // Check if instruction exists
    if (!instruction_table.count(inst) && !directive_table.count(inst)) {
        throw std::runtime_error("Syntax error: invalid instruction");
    }

    // Check if number of arguments if correct
    int req_args;
    if (instruction_table.count(inst)) {
        req_args = argument_table[inst];
    } else {
        req_args = directive_table[inst];
    }

    if (req_args != arg_count) {
        throw std::runtime_error("Syntax error: invalid number of arguments");
    }
 
    // check if type of argument is correct
    if (req_args != 0) {
        if ((syntax_tree["inst"] == "CONST" && !std::isdigit(syntax_tree["arg1"])) ||
            (syntax_tree["inst"] != "CONST" && (std::isdigit(syntax_tree["arg1"]) || syntax_tree["arg2"]))) {
            throw std::runtime_error("Syntax error: wrong type of argument");
        }
    }

    // check if extern and space directives have label
    if ((syntax_tree["inst"] == "EXTERN" && syntax_tree["label"].empty()) 
        || (syntax_tree["inst"] == "SPACE" && syntax_tree["label"].empty())) {
        throw std::runtime_error("Syntax error: missing label for directive");
    }
    
    return syntax_tree;
}

std::string format_object_file(std::vector<int> object_file,
                               std::map<std::string, std::vector<int>> use_table,
                               std::map<std::string, int> def_table,
                               std::vector<int> real_bitmask,
                               float module) {
    std::string object_text = "";

    if (module) {
        object_text += "USO\n";
        std::string aux_string = "";
        if (!use_table.empty()) {
            for (auto const& use : use_table) {
                for (int val : use.second) {
                    aux_string += use.first + " " + std::to_string(val) + "\n";
                }
            }
            object_text += aux_string;
        } else {
            object_text += "\n";
        }

        object_text += "DEF\n";
        if (!def_table.empty()) {
            for (auto const& def : def_table) {
                object_text += def.first + " " + std::to_string(def.second) + "\n";
            }
        } else {
            object_text += "\n";
        }

        aux_string = "";
        object_text += "REAL\n";
        if (!def_table.empty()) {
            for (int i : real_bitmask) {
                aux_string += std::to_string(i);
            }
            object_text += aux_string;
        }
        
        object_text += "\n\n";
    }

    for (int i : object_file) {
        object_text += std::to_string(i) + " ";
    }

    return object_text;
}

std::string single_pass_assembler(std::vector<std::string> code) {
    
    std::map<std::string, int> symbol_table;
    std::map<std::string, int> def_table;
    std::map<std::string, std::vector<int>> use_table;
    std::map<std::string, std::vector<int>> pending_list;
    std::vector<int> object_file;
    std::vector<int> real_bitmask;

    int line_counter = 0;
    int location_count = 0;
    float module = false;
    float end_module = false;

    std::map<std::string, std::string> parsed_line;
    // single_pass
    for (auto line : code) {
        line_counter++;
        float check_inst = true; // not necessary with extern
        float check_args = true; // not necessary with const and public
        float increase_loc_count = true; // flag used only in instruction check

        // get syntax tree from line
        try {
            parsed_line = parser(line);
        } catch (const std::exception& e) {
            std::string error_message = "Error in line "+std::to_string(line_counter)+": "+e.what()+"\n";
            throw std::runtime_error(error_message);
        }

        // check if there is a label
        std::string label = parsed_line["label"];
        if (!label.empty()) {
            // check if label already exists in symbol table
            if (symbol_table.count(label)) {
                std::string error_message = "Error in line "+std::to_string(line_counter)+": Semantic error: Label "+label+"already declared\n";
                throw std::runtime_error(error_message);
            }

            symbol_table[label] = location_count;

            // check if label is pending in def_table
            if (def_table.count(label)) {
                def_table[label] = location_count;
            }

            // check if extern
            // in case extern, define -1 as address
            if (parsed_line["inst"] == "EXTERN") {
                check_inst = false;
                symbol_table[label] = -1;
            }
            
            // check if label is in pending list
            if (pending_list.count(label)) {
                if (symbol_table[label] == -1) {
                    for (int id : pending_list[label]) {
                        use_table[label].push_back(id);
                        object_file[id] = 0;
                    }
                } else {
                    for (int id : pending_list[label]) {
                        object_file[id] = location_count;
                    }
                }
            }
        }
        
        if (!check_inst) continue;

        // check which instruction/directive
        std::string inst = parsed_line["inst"];
        if (!inst.empty()) {
            if (instruction_table.count(inst)) {
                object_file.push_back(instruction_table[inst]);
                real_bitmask.push_back(0);
            } else {
                if (inst == "CONST") {
                    check_args = false;
                    object_file.push_back(std::stoi(parsed_line["arg1"]));
                    real_bitmask.push_back(0);
                } else if (inst == "SPACE") {
                    object_file.push_back(0);
                    real_bitmask.push_back(0);
                } else if (inst == "PUBLIC") {
                    increase_loc_count = false;
                    check_args = false;
                    // check if label is already defined
                    std::string public_label = parsed_line["arg1"];
                    if (symbol_table.count(public_label)) {
                        def_table[public_label] = symbol_table[public_label];
                    } else {
                        def_table[public_label] = -1;
                    }
                } else if (inst == "BEGIN") {
                    module = true;
                } else if (inst == "END") {
                    end_module = true;
                }
            }
            if (increase_loc_count) location_count++;
        }

        if (!check_args) continue;

        // check argument labels
        std::vector<std::string> arg_list = {"arg1", "arg2"};
        for (std::string arg : arg_list) {
            std::string arg_val = parsed_line[arg];
            if (!arg_val.empty()) {
                // check if label is in symbol_table
                if (symbol_table.count(arg_val)) {
                    object_file.push_back(symbol_table[arg_val]);
                    real_bitmask.push_back(1);
                } else {
                    // add to pending list
                    object_file.push_back(-1);
                    pending_list[arg_val].push_back(location_count);
                }
                location_count++;
            }
        }
    }

    // check if there an "end" if "begin"
    if (module != end_module) {
        throw std::runtime_error("Semantic error: incorrect use of BEGIN and END directives");
    }

    // check if there is still pending label declarations
    for (auto const& x : pending_list) {
        if (!symbol_table.count(x.first)) {
            throw std::runtime_error("Semantic erros: missing declaration for "+x.first+" label.");
        }
    }

    std::string formatted_obj = format_object_file(object_file, use_table, def_table, real_bitmask, module);

    return formatted_obj;
}

int main(int argc, char* argv[]) {

    std::string line;
    std::vector<std::string> code;
    std::ifstream source (argv[1]);
    
    if (source.is_open()) {
        while (getline(source, line)) {
            code.push_back(line);
        }
        source.close();

        std::string object_file = single_pass_assembler(code);
        std::cout << object_file << "\n";
    } else {
        std::cout << "Fail to open file\n";
    }

    return 0;
}
