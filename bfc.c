#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_TAPE_SIZE 30000 // Size as cells
#define MAX_TAPE_SIZE 60000 

#define MAX_PATH_LENGTH 1024
#define MAX_STACK_SIZE 1024

#define ARG_IS(a, b) (strcmp(a, b) == 0)

struct args {
    unsigned int tape_size;
    char input_file[MAX_PATH_LENGTH];
};

void
parse_arg(char* arg, struct args* a)
{
    if (ARG_IS(arg, "-t")) {
        arg++;

        if (*arg == '\n') {
            fprintf(stderr, "Tape size expected, got nothing.\n");
            exit(1);
        }

        unsigned int tape_size;

        if (sscanf(arg, "%u", &tape_size) != 1) {
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
in_action(unsigned char* tape, unsigned int ptr) 
{
    tape[ptr] = (unsigned char) getchar();
}

void
out_action(unsigned char* tape, unsigned int ptr) 
{
    putchar(tape[ptr]);
}

void 
interpret(struct args a)
{
    FILE *inputf = fopen(a.input_file, "rb");

    if (!inputf) { 
        fprintf(stderr, "Could not open file \"%s\", perhaps it doesn't exist\n?", a.input_file);
        exit(1); 
    }

    unsigned char tape[a.tape_size];
    unsigned int ptr = 0;

    long stack[MAX_STACK_SIZE];
    unsigned int stack_top = 1;
    

    int c;

    while ( ( c = fgetc(inputf ) ) != EOF) {
        unsigned char inst = (unsigned char) c;

        switch (inst) {
            case '+': tape[ptr]++; break;
            case '-': tape[ptr]--; break; 
            case '.': out_action(tape, ptr); break;
            case ',': in_action(tape, ptr); break;

            case '>': 
                ptr = (ptr + 1) % a.tape_size; 
                break;

            case '<': 
                ptr = (ptr + a.tape_size - 1) % a.tape_size; 
                break;

            case '[':
                if (tape[ptr] == 0) {
                    int depth = 1;
                    long pos;

                    while (depth > 0) {
                        pos = ftell(inputf);
                        int nc = fgetc(inputf);

                        if (nc == EOF) break; // unterminated bracket -> stop

                        if (nc == '[') depth++;
                        else if (nc == ']') depth--;
                    }

                } else {
                    if (stack_top < MAX_STACK_SIZE) {
                        stack[stack_top++] = ftell(inputf);
                    } else {
                        fprintf(stderr, "Stack overflow!\n");
                        exit(1);
                    }
                }
                break;

            case ']':
                if (stack_top == 0) {
                    fprintf(stderr, "Unmatched ']'\n");
                    exit(1);
                }

                if (tape[ptr] != 0) {
                    long target = stack[stack_top - 1];

                    if (fseek(inputf, target, SEEK_SET) != 0) {
                        perror("fseek");
                        exit(1);
                    }
                    
                } else {
                    stack_top--;
                }
                break;

            default:
                /* ignore other chars */
                break;
        }
    }

    fclose(inputf);
}

int main(int argc, char** argv) 
{
    struct args a = parse_args(argc, argv);
    interpret(a);
    return 0;
}