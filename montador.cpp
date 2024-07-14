#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <regex>
#include <stdexcept>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 2560
#define MAX_LABEL_LENGTH 5000
#define MAX_CONSTANTS 1000

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

typedef struct {
    char label[MAX_LABEL_LENGTH];
    char value[MAX_LINE_LENGTH];
} Constant;

Constant constants[MAX_CONSTANTS];
int constant_count = 0;

void to_uppercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper(str[i]);
    }
}

void remove_comments(char *line) {
    char *comment_start = strchr(line, ';');
    if (comment_start != NULL) {
        *comment_start = '\0';
    }
}

void trim_whitespace(char *line) {
    // removendo espaços em branco iniciais
    char *start = line;
    while (isspace((unsigned char)*start)) start++;
    memmove(line, start, strlen(start) + 1);

    // removendo espaços em branco finais
    char *end = line + strlen(line) - 1;
    while (end > line && isspace((unsigned char)*end)) end--;
    end[1] = '\0';

    // removendo espaços do meio
    char *src = line;
    char *dst = line;
    int in_space = 0;

    while (*src != '\0') {
        if (isspace((unsigned char)*src)) {
            if (!in_space) {
                *dst++ = ' ';
                in_space = 1;
            }
        } else {
            *dst++ = *src;
            in_space = 0;
        }
        src++;
    }
    *dst = '\0';
}


void process_equ_directive(char *line) {
    char *label = strtok(line, " ");
    char *equ = strtok(NULL, " ");
    char *value = strtok(NULL, " ");
    if (equ && strcmp(equ, "EQU") == 0 && value) {
        // armazena o rótulo sem ':'
        char label_no_colon[MAX_LABEL_LENGTH];
        strncpy(label_no_colon, label, strlen(label) - 1);
        label_no_colon[strlen(label) - 1] = '\0';
        strcpy(constants[constant_count].label, label_no_colon);
        strcpy(constants[constant_count].value, value);
        constant_count++;
    }
}


void replace_constants(char *line) {
    for (int i = 0; i < constant_count; i++) {
        char *pos;
        while ((pos = strstr(line, constants[i].label)) != NULL) {
            char temp[MAX_LINE_LENGTH];
            strncpy(temp, line, pos - line);
            temp[pos - line] = '\0';
            strcat(temp, constants[i].value);
            strcat(temp, pos + strlen(constants[i].label));
            strcpy(line, temp);
        }
    }
}

int evaluate_if_directive(char *line) {
    char *if_directive = strtok(line, " ");
    char *condition = strtok(NULL, " ");
    if (if_directive && strcmp(if_directive, "IF") == 0 && condition) {
        for (int i = 0; i < constant_count; i++) {
            if (strcmp(condition, constants[i].label) == 0) {
                return atoi(constants[i].value) != 0;
            }
        }
    }
    return 0;
}

// verifica linhas que só tem rótulo e junta com a linha seguinte não vazia
void realign_labels(char lines[][MAX_LINE_LENGTH], int *line_count) {
    for (int i = 0; i < *line_count - 1; i++) {
        char *label_end = strchr(lines[i], ':');
        if (label_end && *(label_end + 1) == '\0') {
            int j = i + 1;
            while (j < *line_count && strlen(lines[j]) == 0) {
                j++;
            }
            if (j < *line_count) {
                strcat(lines[i], " ");
                strcat(lines[i], lines[j]);
                for (int k = j; k < *line_count - 1; k++) {
                    strcpy(lines[k], lines[k + 1]);
                }
                (*line_count)--;
                i--;  //corrige a linha
            }
        }
    }
}

void preprocess_file(const char *input_filename, const char *output_filename) {
    FILE *input_file = fopen(input_filename, "r");
    FILE *output_file = fopen(output_filename, "w");
    if (!input_file || !output_file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char line[MAX_LINE_LENGTH];
    char lines[MAX_CONSTANTS][MAX_LINE_LENGTH];
    int line_count = 0;
    int skip_next_line = 0;
    int stop_line_index = -1;

    // Primeira etapa
    while (fgets(line, sizeof(line), input_file)) {
        to_uppercase(line);
        remove_comments(line);
        trim_whitespace(line);
        strcpy(lines[line_count++], line);
    }
    fclose(input_file);

    realign_labels(lines, &line_count);

    // processa as diretivas, switch case era mais chato
    for (int i = 0; i < line_count; i++) {
        if (stop_line_index == -1 && strstr(lines[i], "STOP")) {
            stop_line_index = i;
        }

        if (strstr(lines[i], "EQU")) {
            process_equ_directive(lines[i]);
            lines[i][0] = '\0'; //  linha que será excluída
            continue;
        }

        if (strstr(lines[i], "IF")) {
            skip_next_line = !evaluate_if_directive(lines[i]);
            lines[i][0] = '\0'; 
            continue;
        }

        if (skip_next_line) {
            lines[i][0] = '\0'; 
            skip_next_line = 0;
            continue;
        }
    }

    //substituir todos os rótulos
    for (int i = 0; i < line_count; i++) {
            replace_constants(lines[i]);
        }

    // escreve a saida
    for (int i = 0; i < line_count; i++) {
        if (strlen(lines[i]) > 0) {
            fprintf(output_file, "%s\n", lines[i]);
        }
    }

    fclose(output_file);
}

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
    const std::regex re_number ("^[0-9]+$");
    std::string arg1 = syntax_tree["arg1"];
    if (req_args != 0) {
        if ((inst == "CONST" && !std::regex_search(arg1, re_number)) ||
            (inst != "CONST" && std::regex_search(arg1, re_number))) {
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
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " -p <inputfile.asm> | -o <inputfile.pre>\n";
        return EXIT_FAILURE;
    }

    std::string mode = argv[1];
    std::string input_filename = argv[2];

    if (mode == "-p") {
        // Modo de pré-processamento
        std::string output_filename = input_filename.substr(0, input_filename.find_last_of('.')) + ".pre";
        preprocess_file(input_filename.c_str(), output_filename.c_str());
    } else if (mode == "-o") {
        // Modo de montagem
        std::vector<std::string> code;
        std::string line;
        std::ifstream source(input_filename);

        if (source.is_open()) {
            while (std::getline(source, line)) {
                code.push_back(line);
            }
            source.close();

            std::string object_file = single_pass_assembler(code);
            std::string output_filename = input_filename.substr(0, input_filename.find_last_of('.')) + ".obj";
            std::ofstream output(output_filename);
            if (output.is_open()) {
                output << object_file;
                output.close();
            } else {
                std::cerr << "Failed to open output file\n";
                return EXIT_FAILURE;
            }
        } else {
            std::cerr << "Failed to open input file\n";
            return EXIT_FAILURE;
        }
    } else {
        std::cerr << "Invalid mode. Use -p for pre-processing or -o for assembling.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


