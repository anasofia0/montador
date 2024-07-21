#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <sstream>

// Função utilitária para juntar elementos de um vetor de inteiros em uma string com separador
std::string intVectorJoin(const std::vector<int> &elements, const std::string &separator) {
    std::stringstream result;
    for (size_t i = 0; i < elements.size(); ++i) {
        if (i != 0)
            result << separator;
        result << elements[i];
    }
    return result.str();
}

// Função utilitária para juntar elementos de um vetor de strings em uma string com separador
std::string strVectorJoin(const std::vector<std::string> &elements, const std::string &separator) {
    std::string result;
    for (size_t i = 0; i < elements.size(); ++i) {
        if (i != 0)
            result += separator;
        result += elements[i];
    }
    return result;
}

// Estrutura que representa uma célula de símbolo
typedef struct symbol_cell_t {
    int address;
    bool defined;
    bool external;
    std::vector<int> dependencies;
} symbol_cell_t;

typedef std::map<std::string, int> def_table_t;
typedef std::map<std::string, std::vector<int>> use_table_t;

// Função para criar e inicializar uma célula de símbolo
symbol_cell_t create_s_cell(int addr, bool def, bool external) {
    symbol_cell_t s_cell;
    std::vector<int> dependencies;
    s_cell.address = addr;
    s_cell.defined = def;
    s_cell.external = external;
    s_cell.dependencies = dependencies;
    return s_cell;
}

typedef enum filetype {EXC, OBJ} filetype;

// Classe base para arquivos de saída
class out_file_t {
public:
    filetype type;
    std::vector<int> code;
    out_file_t() {}
    virtual void write_file(std::string filename) = 0;
};

// Classe para arquivos de objeto
class obj_file_t: public out_file_t {
public:
    use_table_t use_table;
    def_table_t def_table;
    std::vector<int> relative;
    obj_file_t(): out_file_t() {}
    void write_file(std::string filename) override;
};

// Classe para arquivos executáveis
class exc_file_t: public out_file_t {
public:
    exc_file_t(): out_file_t(){};
    void write_file(std::string filename) override;
};

// Implementação dos métodos de escrita dos arquivos de objeto
void obj_file_t::write_file(std::string filename) {
    std::stringstream result;
    
    result << "USO\n";
    for (const auto &use : use_table)
        result << use.first << " " << intVectorJoin(use.second, " ") << '\n';
    result << "DEF\n";
    for (const auto &def : def_table)
        result << def.first << " " << def.second << '\n';

    std::string real_str(code.size(), '0');
    for (const int &addr : relative) {
        real_str[addr] = '1';
    }
    result << "REAL\n" << real_str << '\n';

    for (const int &byte : code) {
        result << byte << ' ';
    }

    FILE *file = fopen(filename.c_str(), "w+");
    fputs(result.str().c_str(), file);
    fclose(file);
}


// Implementação dos métodos de escrita dos arquivos executáveis
void exc_file_t::write_file(std::string filename) {
    FILE *file = fopen(filename.c_str(), "w+");
    for (const int &byte : code)
        fprintf(file, "%d ", byte);
    fclose(file);
}

// Função para ler arquivos de objeto
obj_file_t read_obj(std::string str_filename) {
    obj_file_t obj_file;

    const char *filename = str_filename.c_str();
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("File <%s> doesn't exist.\n", filename);
        throw;
    }
    
    fseek(file, 0L, SEEK_END);
    size_t str_len = ftell(file);
    rewind(file);

    char orig[str_len + 1], *aux, *line, *token;

    size_t bytes_read = fread(orig, sizeof(char), str_len, file);
    if (bytes_read != str_len) {
        printf("Error reading the file: <%s>.\n", filename);
        fclose(file);
        throw;
    }

    fclose(file);
    orig[str_len] = '\0';

    aux = orig;
    int mode = -1;
    while ((line = strtok_r(aux, "\n", &aux))) {
        std::string line_str(line); 
        if(line_str == "USO") {
            mode = 0;
        } else if(line_str == "DEF") {
            mode = 1;
        } else if(line_str == "REAL") {
            mode = 2;
        } else if (mode == 0) {
            use_table_t *uses = &obj_file.use_table;
            char *key = strtok_r(line, " ", &line);
            auto it = uses->find(key);
            if (it == uses->end()) {
                (*uses)[key] = std::vector<int>();
            }
            while ((token = strtok_r(line, " ", &line)))
                (*uses)[key].push_back(atoi(token));
        } else if (mode == 1) {
            char *key = strtok_r(line, " ", &line);
            char *value = strtok_r(line, " ", &line);
            obj_file.def_table[key] = atoi(value);
        } else if (mode == 2) {
            std::string real_str(line);
            for (size_t i = 0; i < real_str.size(); ++i) {
                if (real_str[i] == '1') {
                    obj_file.relative.push_back(i);
                }
            }
            mode = 3; // CO
        } else if (mode == 3) {
            while ((token = strtok_r(line, " ", &line))) {
                obj_file.code.push_back(atoi(token));
            }
        }
    }
    
    return obj_file;
}


std::string generate_output_filename(const std::string &input_filename) {
    size_t pos = input_filename.rfind(".obj");
    if (pos != std::string::npos) {
        return input_filename.substr(0, pos) + ".e";
    } else {
        return input_filename + ".e";
    }
}

// Função principal que executa o processo de ligação de arquivos de objeto
int main(int argc, char **argv) {
    if (argc <= 1) {
        printf("No files provided for linking.\n");
        return 0;
    }

    def_table_t global_def_table;
    use_table_t global_use_table;
    std::map<int, int> relative_corrections;
    exc_file_t combined_code;

    int offset = 0;
    for (int i = 1; i < argc; i++) {
        std::string filename(argv[i]);
        std::string obj_filename = filename;
        obj_file_t obj_file = read_obj(obj_filename);

        for (const auto &def_entry : obj_file.def_table) {
            global_def_table[def_entry.first] = def_entry.second + offset;
        }

        for (const auto &use_entry : obj_file.use_table) {
            auto &global_uses = global_use_table[use_entry.first];
            for (const int &addr : use_entry.second) {
                global_uses.push_back(addr + offset);
            }
        }

        combined_code.code.insert(combined_code.code.end(), obj_file.code.begin(), obj_file.code.end());

        for (const int &rel_addr : obj_file.relative) {
            relative_corrections[rel_addr + offset] = offset;
        }

        offset += obj_file.code.size();
    }

    for (const auto &use_entry : global_use_table) {
        int symbol_addr = global_def_table[use_entry.first];
        for (const int &use_addr : use_entry.second) {
            relative_corrections[use_addr] = symbol_addr;
        }
    }

    for (const auto &rel_entry : relative_corrections) {
        combined_code.code[rel_entry.first] += rel_entry.second;
    }

    //std::string exc_filename = std::string(argv[1]) + ".e";
    std::string exc_filename = generate_output_filename(argv[1]);
    combined_code.write_file(exc_filename);

    printf("Linked files into %s\n", exc_filename.c_str());

    return 0;
}
