#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 2560
#define MAX_LABEL_LENGTH 5000
#define MAX_CONSTANTS 1000

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
    char *start = line;
    while (isspace((unsigned char)*start)) start++;
    memmove(line, start, strlen(start) + 1);

    char *end = line + strlen(line) - 1;
    while (end > line && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
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

void process_const_directive(char *line) {
    char *label = strtok(line, " ");
    char *directive = strtok(NULL, " ");
    char *value = strtok(NULL, " ");
    if (directive && strcmp(directive, "CONST") == 0 && value) {
        // armazena o rótulo sem ':'
        char label_no_colon[MAX_LABEL_LENGTH];
        strncpy(label_no_colon, label, strlen(label) - 1);
        label_no_colon[strlen(label) - 1] = '\0';
        strcpy(constants[constant_count].label, label_no_colon);
        strcpy(constants[constant_count].value, value);
        constant_count++;
        // troca o CONST pelo seu valor
        sprintf(line, "%s %s", label, value);
    }
}

void process_space_directive(char *line) {
    char *label = strtok(line, " ");
    char *directive = strtok(NULL, " ");
    if (directive && strcmp(directive, "SPACE") == 0) {
        // armazena o rótulo sem ':'
        char label_no_colon[MAX_LABEL_LENGTH];
        strncpy(label_no_colon, label, strlen(label) - 1);
        label_no_colon[strlen(label) - 1] = '\0';
        strcpy(constants[constant_count].label, label_no_colon);
        strcpy(constants[constant_count].value, "00");
        constant_count++;
        
        sprintf(line, "%s 00", label);
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

// Main preprocesso
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

        if (strstr(lines[i], "CONST")) {
            process_const_directive(lines[i]);
            continue;
        }

        if (strstr(lines[i], "SPACE")) {
            process_space_directive(lines[i]);
            continue;
        }
    }

    //substituir todos os rótulos
    for (int i = 0; i < stop_line_index; i++) {
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

int main(int argc, char *argv[]) {
    if (argc != 3 || strcmp(argv[1], "-p") != 0) {
        fprintf(stderr, "Usage: %s -p <myfile.asm>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *input_filename = argv[2];
    char output_filename[FILENAME_MAX];

    // saida
    snprintf(output_filename, sizeof(output_filename), "%.*s.pre", (int)(strrchr(input_filename, '.') - input_filename), input_filename);

    preprocess_file(input_filename, output_filename);
    return EXIT_SUCCESS;
}
