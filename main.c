#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#define EXECUTABLE "cg"

#define STRCMP(X, Y) strcmp(X, Y) == 0
#define DEBUG(...) fprintf(stdout, __VA_ARGS__)

#define ERROR_EXIT exit(1)
#define ERROR(...) fprintf(stderr, __VA_ARGS__)
#define ERROR_AND_EXIT(...)\
    ERROR(__VA_ARGS__);\
    ERROR_EXIT

/*ROOT DIR*/
const char* root_gitignore = "\
build/\n\
";

const char* root_cmakelists = "\
cmake_minimum_required (VERSION 3.10)\n\
\n\
set(THIS \"Project_Name\")\n\
project(${THIS} VERSION 0.0 DESCRIPTION \"Your project discription\")\n\
\n\
set(CMAKE_CXX_STANDARD 20)\n\
\n\
add_subdirectory(src)\n\
";

typedef struct {
    const char* cmakelists;
    const char* gitignore;
} DirRoot;

DirRoot mk_dir_root(const char* cmakelists, const char* gitignore) {
    return (DirRoot) {
        .cmakelists = cmakelists,
        .gitignore = gitignore,
    };
}
/*ROOT DIR END*/

/*SOURCE DIR*/
const char* source_main = "#include<stdio.h>\n\
\n\
int main(int argc, char** argv) {\n\
    printf(\"%s\", \"hello world\");\n\
    return 0;\n\
}\n";

const char* source_cmakelists = "\
cmake_minimum_required(VERSION 3.10)\n\
\n\
set(THIS \"Project_Binary\")\n\
add_executable(${THIS}\n\
    main.cpp\n\
)\n\
";

typedef struct {
    char* main;
    char* cmakelists;
} DirSource;

DirSource mk_dir_source(const char* main, const char* cmakelists) {
    return (DirSource) {
        .main = source_main,
        .cmakelists = source_cmakelists,
    };
}
/*SOURCE DIR END*/


/*TEST DIR*/
const char* test_test = "#include <gtest/gtest.h>\n\
\n\
TEST(test, sample) {\n\
    EXPECT_EQ(true, true);\n\
}\n\
";

const char* test_cmakelists = "cmake_minimum_required(VERSION 3.10)\n\
\n\
project(test)\n\
\n\
add_subdirectory(googletest)\n\
add_executable(${PROJECT_NAME}\n\
    test.cpp\n\
)\n\
\n\
target_link_libraries(${PROJECT_NAME}\n\
    gtest\n\
    gtest_main\n\
)\n\
";

typedef struct {
    char* test;
    char* cmakelists;
} DirTest;

DirTest mk_dir_test(const char* test, const char* cmakelists) {
    return (DirTest) {
        .test = test_test,
        .cmakelists = test_cmakelists,
    };
}
/*TEST DIR END*/

/*CONFIG*/
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
/*CONFIG END*/

/*GIT*/
#define REPOSITORY_GOOGLETEST "https://github.com/google/googletest"
#define GIT_INIT "git init"
#define GIT_SUBMODULE_ADD "git submodule add"
#define CG_GIT_INIT() CG_SYSTEM(GIT_INIT)

void CG_SYSTEM(const char* x) {
    if (system(x) < 0) {
        ERROR_AND_EXIT("ERROR: executing %s", x);
    }
}

static void CG_MKDIR_OR_EXIT(const char* X) {
    if (mkdir(X, S_IRWXU) < 0) {
        ERROR("ERROR: Already exists: %s\n", X);
        perror("ERROR: mkdir()");
        exit(1);
    }
    chdir(X);
    CG_SYSTEM(GIT_INIT);
    chdir("..");
}

void CG_ADD_SUBMODULE(const char* dir, const char* repo) {
    size_t buffer_size = strlen(GIT_SUBMODULE_ADD) + strlen(repo) + 3;
    char* buffer[buffer_size];
    snprintf(buffer, buffer_size, "%s %s\0", GIT_SUBMODULE_ADD, repo);

    /*TOOD Write a wrapper to jump inside a dir and exit*/
    chdir(dir);
    CG_SYSTEM(buffer);
    chdir("..");
}
/*GIT END*/

typedef struct {
    bool init;
    bool new;
} Args;

typedef struct {
    bool test;
    bool benchmark;
} Flags;

/*Maybe unused for sure*/
int is_dir_exists(const char* dir) {
    struct stat _stat;
    if (stat(&dir, &stat) == 0 && S_ISDIR(_stat.st_mode)) {
        return 0;
    }
    return -1;
}

const char* get_curr_path(void) {
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
char* append_to_path(const char* path, char* file) {
    size_t _path_size = strlen(path) + strlen(file) + 2;
    char* _path = (char*) malloc(_path_size* sizeof(char));
    snprintf(_path, _path_size, "%s/%s\0", path, file);
    return _path;
}

#define WRITE(X, Y, Z) CG_WRITE("w", X, Y, Z)
#define WRITE_APPEND(X, Y, Z) CG_WRITE("a", X, Y, Z)
void CG_WRITE(const char* mode, const char* directory_path, const char* file_path, char* thing) {
    char* _file = append_to_path(directory_path, file_path);
    FILE* fp = fopen(_file, mode);
    if (fp == NULL) {
        fprintf(stderr, "ERROR: Writing to %s\n", _file);
        exit(1);
    }
    fprintf(fp, "%s", thing);
    free(_file);
    fclose(fp);
}

static const char* help_message = "\
Usage: ./%s\n\
        Creates a temporary project\n\
    Args:\n\
        new    Creates new project dir\n\
        init   Iniitializes new project in current dir\n\
    Options:\n\
        -add   Generate test, benchmark dirs);\n\
        -help   Prints this crappy message);\n\
";

static void Usage(FILE* where) {
    fprintf(where, help_message, EXECUTABLE);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        Usage(stderr);
        exit(1);
    }

    Config config;
    make_config(&config);

    Flags flags;
    Args args;

    char** args_begin = argv + 1;
    char** args_end = argv + argc;

    while(args_begin != args_end) {
        if (STRCMP(*args_begin, "new")) {
            char** curr = args_begin + 1;
            if (curr == args_end) {
                ERROR("ERROR: missing _NAME\n");
                Usage(stderr);
                exit(1);
            } else {
                config.name = DEEP_COPY(*curr, strlen(*curr));

                char* _curr_path = get_curr_path();
                size_t t_size = strlen(config.name) + strlen(_curr_path) + 2;
                config.path = (char*) malloc(t_size* sizeof(char));
                snprintf(config.path, t_size, "%s/%s\0", _curr_path, config.name);

                args.new = true;
                args_begin = curr + 1;
            }
        } else if (STRCMP(*args_begin, "init")) {
            args.init = true;
            args_begin += 1;
        } else if (STRCMP(*args_begin, "-add")) {
            char** curr = args_begin + 1;
            if (curr == args_end) {
                ERROR("ERROR: missing [test, benchmark]\n");
                Usage(stderr);
                exit(1);
            } else {
                /*currently reading only one at a time*/
                if (STRCMP(*curr, "test")) {
                    flags.test = true;
                } else if (STRCMP(*curr, "test")) {
                    flags.benchmark = true;
                } else {
                    ERROR("ERROR: Invaild %s", *curr);
                    exit(1);
                }
                args_begin = curr + 1;
            }
        } else if (STRCMP(*args_begin, "-help")) {
            Usage(stdout);
            exit(0);
        } else {
            ERROR("NO MATCH: %s\n", *args_begin);
            Usage(stderr);
            exit(1);
        }
    }

    if (!args.new && !args.init) {
        ERROR("ERROR: Missing positional args %s, %s\n", "init", "new");
        exit(1);
    }

    if (args.init) {
        char* _path = get_curr_path();
        config.path = DEEP_COPY(_path, strlen(_path));
        config.name = get_curr_folder(config.path, strlen(config.path));
    }

    config.dir = config.name;

    (!args.init)
        ? CG_MKDIR_OR_EXIT(config.dir) : CG_SYSTEM(GIT_INIT);

    /* ---------------------------- */
    /* Creating directory and files */
    /* ---------------------------- */
    /* Root directory */
    DirRoot Dir_Root = mk_dir_root(root_cmakelists, root_gitignore);
    char* directory_root = config.path;

    WRITE(directory_root, "CMakeLists.txt", Dir_Root.cmakelists);
    WRITE(directory_root, ".gitignore", Dir_Root.gitignore);
    if (flags.test) {
        WRITE_APPEND(directory_root, "CMakeLists.txt", "add_subdirectory(test)");
    }

    /* Source directory */
    DirSource Dir_Source = mk_dir_source(source_main, source_cmakelists);
    char* directory_source = append_to_path(config.path, "src");
    if (mkdir(directory_source, S_IRWXU) < 0) {
        fprintf(stderr, "ERROR: Creating source directory: %s\n", directory_source);
        exit(1);
    }

    WRITE(directory_source, "main.cpp", Dir_Source.main);
    WRITE(directory_source, "CMakeLists.txt", Dir_Source.cmakelists);
    free(directory_source);

    /* Test directory */
    if (flags.test) {
        DirTest Dir_Test = mk_dir_test(test_test, test_cmakelists);
        const char* directory_test = append_to_path(config.path, "test");
        if (mkdir(directory_test, S_IRWXU) < 0) {
            fprintf(stderr, "ERROR: Creating test directory: %s\n", directory_test);
            exit(1);
        }

        WRITE(directory_test, "test.cpp", Dir_Test.test);
        WRITE(directory_test, "CMakeLists.txt", Dir_Test.cmakelists);

        /*Add googletest*/
        CG_ADD_SUBMODULE(directory_test, "https://github.com/google/googletest");

        free(directory_test);
    }

    wreck_config(&config);
    return 0;
}
