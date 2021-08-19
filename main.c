#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/stat.h>

#define EXECUTABLE "cg"

#define STRCMP(X, Y) strcmp(X, Y) == 0
#define DEBUG(...) fprintf(stdout, __VA_ARGS__)

#define ERROR_EXIT exit(1)
#define ERROR(...) fprintf(stderr, __VA_ARGS__)
#define ERROR_AND_EXIT(...)\
    ERROR(__VA_ARGS__);\
    ERROR_EXIT

#define CG_PANIC_A_EXIT(C)\
    Usage(stderr);\
    wreck_config((C));\
    ERROR_EXIT

#define MKDIR_OR_EXIT(X)\
    if (mkdir(X, S_IRWXU) < 0) {\
        ERROR_AND_EXIT("ERROR: Creating %s\n", X);\
    }\

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
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})\n\
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
        .main = main,
        .cmakelists = cmakelists,
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
        .test = test,
        .cmakelists = cmakelists,
    };
}
/*TEST DIR END*/

/*BENCHMARK DIR*/
const char* benchmark_cmakelists = "cmake_minimum_required (VERSION 3.10)\n\
\n\
add_subdirectory(benchmark)\n\
\n\
add_executable(bench\n\
    bench.cpp\n\
)\n\
\n\
target_link_libraries(bench\n\
    benchmark\n\
)\n\
";

const char* benchmark_bench = "#include <benchmark/benchmark.h>\n\
\n\
BENCHMARK_MAIN();\n\
";

typedef struct {
    const char* bench;
    const char* cmakelists;
} DirBenchmark;

DirBenchmark mk_dir_benchmark(const char* bench, const char* cmakelists) {
    return (DirBenchmark) {
        .bench = bench,
        .cmakelists = cmakelists,
    };
}
/*BENCHMARK DIR END*/

/*CONFIG*/
typedef struct {
    char* name;
    char* path;
    char* directory;
} Config;

void make_config(Config* config) {
    config->name = NULL;
    config->path = NULL;
    config->directory = NULL;
}

void wreck_config(Config* config) {
    free(config->name);
    free(config->path);
}
/*CONFIG END*/

/*GIT*/
/*REPOSITORYS*/
#define REPOSITORY_GOOGLE_TEST "https://github.com/google/googletest"
#define REPOSITORY_GOOGLE_BENCHMARK "https://github.com/google/benchmark.git"

#define GIT_INIT "git init"
#define GIT_SUBMODULE_ADD "git submodule add"
#define CG_GIT_INIT() CG_SYSTEM(GIT_INIT)

#define CG_SYSTEM(x)\
    if (system(x) < 0) {\
        ERROR_AND_EXIT("ERROR: executing %s", x);\
    }

#define CG_MKDIR(X)\
    if (mkdir(X, S_IRWXU) < 0) {\
        ERROR("ERROR: mkdir(): %s: ", X);\
        perror(NULL);\
        exit(1);\
    }

#define CG_MKDIR_W_GIT(X, Y)\
    CG_MKDIR(Y);\
    if (X) {\
        chdir(X);\
        CG_SYSTEM(GIT_INIT);\
        chdir("..");\
    }

void CG_ADD_SUBMODULE(const char* dir, const char* repo) {
    size_t buffer_size = strlen(GIT_SUBMODULE_ADD) + strlen(repo) + 3;
    char buffer[buffer_size];
    snprintf(buffer, buffer_size, "%s %s\0", GIT_SUBMODULE_ADD, repo);

    /*TODO Write a wrapper to jump inside a dir and exit*/
    chdir(dir);
    CG_SYSTEM(buffer);
    chdir("..");
}
/*GIT END*/

/*Args*/
typedef struct {
    bool init;
    bool new;
} Args;


/*Flags*/
#define IS_ADD_SUPPLIED (flags.test || flags.benchmark)

typedef struct {
    bool test;
    bool benchmark;
    bool make_random_dir;
    bool initialize_git_repo;
} Flags;

void mk_flags(Flags* flags) {
    flags->initialize_git_repo = true;
}


/*Check if file exists*/
int exists(const char* dir) {
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

char* CG_DEEP_COPY(char* dest, size_t size) {
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

bool is_vaild_string_of_ints(char* str, size_t len) {
    int i = 0;
    for(; i < len; ++i) {
        if (str[i] >= 48 && str[i] <= 57) {
        } else {
            return false;
        }
    }
    return true;
}

#define WRITE(X, Y, Z) CG_WRITE("w", X, Y, Z)
#define WRITE_APPEND(X, Y, Z) CG_WRITE("a", X, Y, Z)
void CG_WRITE(const char* mode, const char* directory_path, const char* file_path, char* thing) {
    char* _file = append_to_path(directory_path, file_path);
    FILE* fp = fopen(_file, mode);
    if (fp == NULL) {
        ERROR("ERROR: Writing to %s\n", _file);
        exit(1);
    }
    fprintf(fp, "%s", thing);
    free(_file);
    fclose(fp);
}

/*make random dir*/
static const char* alphas = "abcdefghigklmopqrstuvxyz";

int get_random_idx(const size_t range) {
    return rand() % range;
}

char get_random_char(const char* chars, size_t len) {
    size_t n = get_random_idx(len);
    assert((n >= 0 && n < len) && "Random number generated is overflowing the chars buffer");
    return chars[n];
}

char* get_random_dir_name(const size_t len) {
    int i = 0;
    char* dir_name = (char*) malloc((len + 1)* sizeof(char));
    for(; i < len; ++i) {
        dir_name[i] = get_random_char(alphas, strlen(alphas));
    }
    dir_name[len] = '\0';
    return dir_name;
}
/*end*/

void cmd_initialize_new(Config* config, char* curr) {
    config->name = CG_DEEP_COPY(curr, strlen(curr));

    char* _curr_path = get_curr_path();
    size_t t_size = strlen(config->name) + strlen(_curr_path) + 2;
    config->path = (char*) malloc(t_size* sizeof(char));
    snprintf(config->path, t_size, "%s/%s\0", _curr_path, config->name);
}

static const char* help_message = "\
Usage: %s\n\
    Creates a temporary project\n\
Args:\n\
    new    Creates new project dir\n\
    init   Iniitializes new project in current dir\n\
\n\
Options:\n\
    -add   Generate test, benchmark dirs\n\
           example: -add +test +bench\n\
    -rnd   create a random dir, default directory length is 3\n\
           can be changed using +[length]\n\
    -dg    Donot initialize git repo. Will raise error in case of external modules required\n\
    -help  Prints this crappy message\n\
";

static void Usage(FILE* where) {
    fprintf(where, help_message, EXECUTABLE);
}

int main(int argc, char** argv) {
    srand(time(NULL));

    if (argc < 2) {
        Usage(stderr);
        exit(1);
    }

    Config config;
    make_config(&config);

    Args args;

    Flags flags;
    mk_flags(&flags);

    char** args_begin = argv + 1;
    char** args_end = argv + argc;

    while(args_begin != args_end) {
        if (STRCMP(*args_begin, "new")) {
            char** curr = args_begin + 1;
            if (curr == args_end) {
                ERROR("ERROR: missing NAME\n");
                Usage(stderr);
                exit(1);
            } else {
                if (flags.make_random_dir) {
                    ERROR("ERROR: Invaild use of -rnd with new\n");
                    CG_PANIC_A_EXIT(&config);
                }
                cmd_initialize_new(&config, *curr);
                args.new = true;
                args_begin = curr + 1;
            }
        } else if (STRCMP(*args_begin, "init")) {
            args.init = true;
            args_begin += 1;
        } else if (STRCMP(*args_begin, "-add")) {
            char** curr = args_begin + 1;
            if (curr == args_end) {
                ERROR("ERROR: missing +test +benchmark\n");
                Usage(stderr);
                exit(1);
            } else {
                /*-add +test +benchmark unicode*/
                size_t max_args_len = 2;
                char** list_args_begin = curr;
                char** list_args_end = (curr + 1 != args_end) ? (list_args_begin + max_args_len) : (args_end);
                /*Check first arg*/
                if (*list_args_begin[0] != '+') {
                    ERROR("ERROR: Invaild %s\n", *list_args_begin);
                    Usage(stderr);
                    exit(1);
                }

                while(list_args_begin != list_args_end) {
                    if (*list_args_begin[0] != '+') {
                        goto CONTINUE_PARSE;
                    }

                    if (STRCMP(*list_args_begin, "+test")) {
                        flags.test = true;
                    } else if (STRCMP(*list_args_begin, "+bench")) {
                        flags.benchmark = true;
                    } else {
                        ERROR("ERROR: Invaild %s\n", *list_args_begin);
                        Usage(stderr);
                        exit(1);
                    }
                    list_args_begin++;
                }
CONTINUE_PARSE:
                args_begin = list_args_begin;
            }
        } else if (STRCMP(*args_begin, "-rnd")) {
            /*Raise an error if new is supplied as well*/
            if (args.new) {
                ERROR("ERROR: Invaild use of new with -rnd\n");
                CG_PANIC_A_EXIT(&config);
            }
            int dir_name_length = 3;

            char** list_args_begin = (args_begin + 1); 
            if (list_args_begin != args_end && *list_args_begin[0] == '+') {
                char* curr = *list_args_begin;
                size_t dir_name_size = strlen(curr) - 1;
                char* _dir_name_length = (char*) malloc(dir_name_size + 1 * sizeof(char));
                memcpy(_dir_name_length, curr + 1, dir_name_size);
                _dir_name_length[dir_name_size] = '\0';

                if (!is_vaild_string_of_ints(_dir_name_length, strlen(_dir_name_length))) {
                    ERROR("Invaild: %s, requires a int literal\n", curr);
                    CG_PANIC_A_EXIT(&config);
                }
                dir_name_length = atoi(_dir_name_length);
                free(_dir_name_length);
                args_begin++;
            }

            flags.make_random_dir = true;
            args.new = true;

            char* dir_name = get_random_dir_name(dir_name_length);
            cmd_initialize_new(&config, dir_name); /* try using it with -rnd new suicide */
            free(dir_name);
            args_begin++;
        } else if (STRCMP(*args_begin, "-dg")) {
            flags.initialize_git_repo = false;
            args_begin++;
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
        config.path = CG_DEEP_COPY(_path, strlen(_path));
        config.name = get_curr_folder(config.path, strlen(config.path));
    }

    config.directory = config.name;

    if (!flags.initialize_git_repo && IS_ADD_SUPPLIED) {
        ERROR("Invaild: use of -dg with test and bench flags\n");
        exit(1);
    }

    if (!args.init) {
        CG_MKDIR_W_GIT(flags.initialize_git_repo, config.directory);
    } else {
        if (flags.initialize_git_repo) {
            CG_SYSTEM(GIT_INIT);
        }
    }

    /* ---------------------------- */
    /* Creating directory and files */
    /* ---------------------------- */
    /* Root directory */
    DirRoot Dir_Root = mk_dir_root(root_cmakelists, root_gitignore);
    char* directory_root = config.path;

    WRITE(directory_root, "CMakeLists.txt", Dir_Root.cmakelists);
    WRITE(directory_root, ".gitignore", Dir_Root.gitignore);

    /* Source directory */
    DirSource Dir_Source = mk_dir_source(source_main, source_cmakelists);
    char* directory_source = append_to_path(config.path, "src");
    MKDIR_OR_EXIT(directory_source);

    WRITE(directory_source, "main.cpp", Dir_Source.main);
    WRITE(directory_source, "CMakeLists.txt", Dir_Source.cmakelists);
    free(directory_source);

    /* Test directory */
    if (flags.test) {
        DirTest Dir_Test = mk_dir_test(test_test, test_cmakelists);
        const char* directory_test = append_to_path(config.path, "test");
        MKDIR_OR_EXIT(directory_test);

        WRITE_APPEND(directory_root, "CMakeLists.txt", "add_subdirectory(test)\n");
        WRITE(directory_test, "test.cpp", Dir_Test.test);
        WRITE(directory_test, "CMakeLists.txt", Dir_Test.cmakelists);

        /*Add googletest*/
        CG_ADD_SUBMODULE(directory_test, REPOSITORY_GOOGLE_TEST);

        free(directory_test);
    }

    /*Benchmark directory*/
    if (flags.benchmark) {
        if (!flags.test) {
            /*Add Googletest as a dependency*/
            const char* directory_vendor = append_to_path(config.path, "vendor");
            MKDIR_OR_EXIT(directory_vendor);

            WRITE_APPEND(directory_root, "CMakeLists.txt", "add_subdirectory(vendor/googletest)\n");

            /*Add googletest*/
            CG_ADD_SUBMODULE(directory_vendor, REPOSITORY_GOOGLE_TEST);

            free(directory_vendor);
            /*END*/
        }

        DirBenchmark Dir_Benchmark = mk_dir_benchmark(benchmark_bench, benchmark_cmakelists);
        const char* directory_benchmark = append_to_path(config.path, "benchmark");
        MKDIR_OR_EXIT(directory_benchmark);

        WRITE_APPEND(directory_root, "CMakeLists.txt", "add_subdirectory(benchmark)\n");
        WRITE(directory_benchmark, "bench.cpp", Dir_Benchmark.bench);
        WRITE(directory_benchmark, "CMakeLists.txt", Dir_Benchmark.cmakelists);

        /*Add googlebenchmark*/
        CG_ADD_SUBMODULE(directory_benchmark, REPOSITORY_GOOGLE_BENCHMARK);

        free(directory_benchmark);
    }

    wreck_config(&config);
    return 0;
}
