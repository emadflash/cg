#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#define STRCMP(X, Y) strcmp(X, Y) == 0
#define DEBUG(...) fprintf(stdout, __VA_ARGS__)
#define ERROR(...) fprintf(stderr, __VA_ARGS__)

/*#define FORWARD_SLASH "/"*/
/*#define APPEND_PATH(X, Y) X ## Y*/
#define ADVANCE_ARG(A, X) A += X

/*#define CURR_PATH secure_getenv("PATH")*/

static const char* executable = "cg";

/* files in source di */
const char* SOURCE[] =  { "main.cpp", "CMakeLists.txt", NULL};

/* files in test dir */
const char* TEST[] =  { "test.cpp", "CMakeLists.txt", NULL};

/* CMakeLists.txt */
static const char* Root_CMakeLists = "\
cmake_minimum_required (VERSION 3.10)\n\
\n\
set(THIS \"Project_Name\")\n\
project(${THIS} VERSION 0.0 DESCRIPTION \"Your project discription\")\n\
\n\
set(CMAKE_CXX_STANDARD 20)\n\
\n\
add_subdirectory(src)\n\
";


static const char* Source_main_c = "#include<stdio.h>\n\
\n\
int main(int argc, char** argv) {\n\
    printf(\"%s\", \"hello world\");\n\
    return 0;\n\
}\n";

static const char* Source_CMakeLists = "\
cmake_minimum_required(VERSION 3.10)\n\
\n\
set(THIS \"Project_Binary\")\n\
add_executable(${THIS}\n\
    main.cpp\n\
)\n\
";

int is_dir_exists(const char* dir) {
    struct stat _stat;
    if (stat(&dir, &stat) == 0 && S_ISDIR(_stat.st_mode)) {
        return 0;
    }
    return -1;
}

static void Usage(FILE* where) {
    fprintf(where, "Usage: ./%s\n", executable);
    fprintf(where, "    Creates a temporary project\n");
    fprintf(where, "Options\n");
    fprintf(where, "    new    Creates new project dir\n");
    fprintf(where, "    init   Iniitializes new project in current dir, Deletes existing files/folders in current dir\n");
    fprintf(where, "    help   Prints this crappy message\n");
}

typedef struct {
    char* name;
    char* path;
    char* dir;
} Config;

void make_config(Config* config) {
    config->name = NULL;
    config->path = NULL;
    config->dir = NULL;
}

void wreck_config(Config* config) {
    free(config->name);
    free(config->path);
}

typedef struct {
    bool init;
} Flags;

void make_flags(Flags* f) {
    f->init = false;
}

static const char* get_curr_path() {
    char* curr_path = getenv("PWD");
    if (curr_path == NULL) {
        fprintf(stdout, "This thing uses getenv which requires PWD env\n");
        exit(1);
    }
    return curr_path;
}

char* get_curr_folder(char* path, size_t path_size) {
    char* start = NULL;
    char* end = path + path_size;

    char* cursor = end;

LOOP:
    if (*cursor == '/') {
        start = cursor + 1;
    } else {
        --cursor;
        goto LOOP;
    }

    size_t folder_size = end - start + 1;
    char* folder = (char*) malloc(folder_size);
    memcpy(folder, start, folder_size);
    folder[folder_size - 1] = '\0';
    return folder;
}


char* DEEP_COPY(char* dest, size_t size) {
    size_t _size = size + 1;
    char* ret = (char*) malloc(_size* sizeof(char));
    memcpy(ret, dest, _size);
    ret[_size - 1] = '\0';
    return ret;
}

/* TODO Make it variadic */
char* append_to_path(char* path, char* file) {
    size_t _path_size = strlen(path) + strlen(file) + 2;
    char* _path = (char*) malloc(_path_size* sizeof(char));
    snprintf(_path, _path_size, "%s/%s\0", path, file);
    return _path;
}

void write_to_file(char* directory_path, char* file_path, char* thing) {
    char* _file = append_to_path(directory_path, file_path);
    FILE* fp = fopen(_file, "w");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: Writing to %s\n", _file);
        exit(1);
    }
    fprintf(fp, "%s", thing);
    free(_file);
    fclose(fp);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        Usage(stderr);
        exit(1);
    }

    Config config;
    make_config(&config);

    Flags flags;
    make_flags(&flags);

    char** args_begin = argv + 1;
    char** args_end = argv + argc;

    while(args_begin != args_end) {
        if (STRCMP(*args_begin, "new")) {
            char** curr = args_begin + 1;
            if (curr == args_end) {
                ERROR("ERROR: missing _NAME\n", executable);
                Usage(stderr);
                exit(1);
            } else {
                config.name = DEEP_COPY(*curr, strlen(*curr));

                char* _curr_path = get_curr_path();
                size_t t_size = strlen(config.name) + strlen(_curr_path) + 2;
                config.path = (char*) malloc(t_size* sizeof(char));
                snprintf(config.path, t_size, "%s/%s\0", _curr_path, config.name);

                args_begin = curr + 1;
            }
        } else if (STRCMP(*args_begin, "init")) {
            flags.init = true;
            args_begin += 1;
        } else {
            ERROR("NO MATCH\n");
            Usage(stderr);
            exit(1);
        }
    }

    if (flags.init) {
        char* _path = get_curr_path();
        config.path = DEEP_COPY(_path, strlen(_path));
        config.name = get_curr_folder(config.path, strlen(config.path));
    }

    config.dir = config.name;

    if (is_dir_exists(config.dir) == 0) {
        perror("ERROR: is_dir_exists()");
        exit(1);
    }

    if (mkdir(config.dir, S_IRWXU) < 0) {
        fprintf(stderr, "ERROR: Already exists: %s\n", config.path);
        perror("ERROR: mkdir()");
        exit(1);
    }

    /* Creating directory and files */
    /* - Root directory */
    char* directory_root = config.path;
    write_to_file(directory_root, "CMakeLists.txt", Root_CMakeLists);

    /* --- Source directory */
    char* directory_source = append_to_path(config.path, "src");
    if (mkdir(directory_source, S_IRWXU) < 0) {
        fprintf(stderr, "ERROR: Creating source directory: %s\n", directory_source);
        perror("ERROR: mkdir()");
        exit(1);
    }

    write_to_file(directory_source, "main.c", Source_main_c);
    write_to_file(directory_source, "CMakeLists.txt", Source_CMakeLists);

    free(directory_source);
    wreck_config(&config);
    return 0;
}
