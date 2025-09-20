#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_TAPE_SIZE 30000 // Size as cells
#define MAX_TAPE_SIZE 60000

#define MAX_PATH_LENGTH 1024
#define MAX_STACK_SIZE 1024

#define ARG_IS(a, b) (strcmp(a, b) == 0)

struct args {
    size_t tape_size;
    char input_file[MAX_PATH_LENGTH];
};

void
parse_arg(char* arg, struct args* a)
{
    if (ARG_IS(arg, "-t")) {
        arg++;

        if (*arg == '\n' || *arg == '\0') {
            fprintf(stderr, "Tape size expected, got nothing.\n");
            exit(1);
        }

        size_t tape_size;

        if (sscanf(arg, "%zu", &tape_size) != 1) {
            fprintf(stderr, "Could not understand buffer size. Perhaps it's written wrong?\n");
            exit(1);
        }

        if (tape_size > MAX_TAPE_SIZE) {
            fprintf(stderr, "Buffer size can not be bigger than %u\n", MAX_TAPE_SIZE);
            exit(1);
        }

        a->tape_size = tape_size;
    }
}

void
parse_path(char* path, struct args* a) 
{
    if (path[0] == '~' && path[1] == '/') 
    {
        // Expand ~/
        const char* HOME = getenv("HOME");

        if (HOME == NULL) {
            fprintf(stderr, "Error: HOME environment variable not set. Why?\n");
            exit(1);
        }

        snprintf(a->input_file, MAX_PATH_LENGTH, "%s%s", HOME, path + 1);

    } else {
        snprintf(a->input_file, MAX_PATH_LENGTH, "%s", path);
    }
}

struct args 
parse_args(int argc, char** argv) 
{
    struct args a = {
        DEFAULT_TAPE_SIZE,
        ""
    };

    for (int i = 1; i < argc; i++) // skip the program name
    {
        char* arg = argv[i];

        switch (arg[0]) {
            case '-':
                parse_arg(arg, &a);
                break;
            default: 
                // If it does not start with -, lets assume it is a path
                parse_path(arg, &a);
                break;
        }
    }

    return a;
}

void
in_action(unsigned char* tape, size_t ptr) 
{
    tape[ptr] = (unsigned char) getchar();
}

void
out_action(unsigned char* tape, size_t ptr) 
{
    putchar(tape[ptr]);
}

void 
interpret(struct args a)
{
    FILE* inputf = fopen(a.input_file, "rb");

    if (!inputf) {
        fprintf(stderr, "Could not open file \"%s\", perhaps it doesn't exist\n?", a.input_file);
        exit(1);
    }

    if (fseek(inputf, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(inputf);
        exit(1);
    }

    long prog_len = ftell(inputf);

    if (prog_len < 0) {
        perror("ftell");
        fclose(inputf);
        exit(1);
    }

    rewind(inputf);

    char* program = malloc(( size_t) prog_len + 1) ;

    if (!program) {
        fprintf(stderr, "Out of memory reading program\n");
        fclose(inputf);
        exit(1);
    }

    size_t read = fread(program, 1, (size_t) prog_len, inputf);
    program[read] = '\0';
    fclose(inputf);

    // Firstly precompute bracket matches
    int* match = malloc((read) * sizeof(int));

    if (!match) {
        fprintf(stderr, "Out of memory\n");
        free(program);
        exit(1);
    }

    long stack[MAX_STACK_SIZE];
    int stack_top = 0;

    for (size_t i = 0; i < read; i++) {
        char c = program[i];

        if (c == '[') {
            if (stack_top < MAX_STACK_SIZE) {
                stack[stack_top++] = (long) i;
            } else {
                fprintf(stderr, "Bracket stack overflow during precompute!\n");
                free(program);
                free(match);
                exit(1);
            }

        } else if (c == ']') {
            if (stack_top == 0) {
                fprintf(stderr, "Unmatched ']' in program\n");
                free(program);
                free(match);
                exit(1);
            }

            long open_pos = stack[--stack_top];
            match[open_pos] = (int) i;
            match[i] = (int) open_pos;
        }
    }

    if (stack_top != 0) {
        fprintf(stderr, "Unmatched '[' in program\n");
        free(program);
        free(match);
        exit(1);
    }

    unsigned char* tape = calloc(a.tape_size, 1);
    
    if (!tape) {
        fprintf(stderr, "Out of memory for tape\n");
        free(program);
        free(match);
        exit(1);
    }

    size_t ptr = 0;

    for (size_t ip = 0; ip < read; ip++) {
        unsigned char inst = (unsigned char) program[ip];

        switch (inst) {
            case '+': tape[ptr]++; break;
            case '-': tape[ptr]--; break;
            case '.': out_action(tape, ptr); break;
            case ',': in_action(tape, ptr); break;

            case '>':
                ptr++;
                if (ptr >= a.tape_size) ptr = 0;
                break;

            case '<':
                if (ptr == 0) ptr = a.tape_size - 1;
                else ptr--;
                break;

            case '[':
                if (tape[ptr] == 0) {
                    ip = (size_t) match[ip];
                }
                break;

            case ']':
                if (tape[ptr] != 0) {
                    // jump back to matching [
                    ip = (size_t) match[ip];
                }
                break;

            default:
                // ignore other chars 
                break;
        }
    }

    free(program);
    free(match);
    free(tape);
}

int main(int argc, char** argv)
{
    struct args a = parse_args(argc, argv);
    interpret(a);
    return 0;
}