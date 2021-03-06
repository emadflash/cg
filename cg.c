/*
 * GPL-v3.0 license
 * copyright (c) emadflash 2021
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
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

#define CG_PANIC(C)\
    Usage(stderr);\
    wreck_config((C));\
    ERROR_EXIT

#define MKDIR_OR_PANIC(C, X)\
    if (mkdir(X, S_IRWXU) < 0) {\
        CG_PANIC(C);\
        ERROR_AND_EXIT("ERROR: Creating %s\n", X);\
    }\

/* -------------------------------------------------------------------------------------------- */
static const char* root_gitignore = "\
build/\n\
";

static const char* root_cmakelists = "\
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

static DirRoot mk_dir_root(const char* cmakelists, const char* gitignore) {
    return (DirRoot) {
        .cmakelists = cmakelists,
        .gitignore = gitignore,
    };
}

/* -------------------------------------------------------------------------------------------- */
static const char* source_main = "#include<stdio.h>\n\
\n\
int main(int argc, char** argv) {\n\
    printf(\"hello world\");\n\
    return 0;\n\
}\n";

static const char* source_cmakelists = "\
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

static DirSource mk_dir_source(const char* main, const char* cmakelists) {
    return (DirSource) {
        .main = main,
        .cmakelists = cmakelists,
    };
}

/* -------------------------------------------------------------------------------------------- */
static const char* test_test_gtest = "#include <gtest/gtest.h>\n\
\n\
TEST(test, sample) {\n\
    EXPECT_EQ(true, true);\n\
}\n\
";

static const char* test_test_libcheck = "#include <check.h>\n\
\n\
START_TEST(sample_test) {\n\
    ck_assert_int_ne(1, -1);\n\
}\n\
END_TEST\n\
\n\
Suite* suite(void) {\n\
    Suite* s;\n\
    TCase* tc_core;\n\
    s = suite_create(\"sample\");\n\
    tc_core = tcase_create(\"test\");\n\
\n\
    tcase_add_test(tc_core, sample_test);\n\
    return s;\n\
}\n\
\n\
int main() {\n\
    int no_failed = 0;\n\
    Suite* s;\n\
    SRunner* runner;\n\
\n\
    s = suite();\n\
    runner = srunner_create(s);\n\
\n\
    srunner_run_all(runner, CK_NORMAL);\n\
    no_failed = srunner_ntests_failed(runner);\n\
    srunner_free(runner);\n\
    return (no_failed == 0) ? 0 : 1;\n\
}\n\
";

static const char* test_cmakelists_gtest = "cmake_minimum_required(VERSION 3.10)\n\
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

static const char* test_cmakelists_libcheck = "cmake_minimum_required(VERSION 3.10)\n\
\n\
project(test)\n\
\n\
add_subdirectory(check)\n\
add_executable(${PROJECT_NAME}\n\
    test.c\n\
)\n\
\n\
target_link_libraries(${PROJECT_NAME}\n\
    check\n\
    pthread\n\
)\n\
";

typedef struct {
    char* test;
    char* cmakelists;
} DirTest;

static DirTest mk_dir_test(const char* test, const char* cmakelists) {
    return (DirTest) {
        .test = test,
        .cmakelists = cmakelists,
    };
}

/* -------------------------------------------------------------------------------------------- */
static const char* benchmark_cmakelists = "cmake_minimum_required (VERSION 3.10)\n\
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

static const char* benchmark_bench = "#include <benchmark/benchmark.h>\n\
\n\
BENCHMARK_MAIN();\n\
";

typedef struct {
    const char* bench;
    const char* cmakelists;
} DirBenchmark;

static DirBenchmark mk_dir_benchmark(const char* bench, const char* cmakelists) {
    return (DirBenchmark) {
        .bench = bench,
        .cmakelists = cmakelists,
    };
}

/* -------------------------------------------------------------------------------------------- */
static const char* c_makefile = "CC=gcc\n\
CFLAGS=-Wall -g -pedantic -fsanitize=address -std=c99\n\
EXEC=%s\n\
\n\
EXEC: main.c\n\
		$(CC) $(CFLAGS) main.c -o $(EXEC)";


/* -------------------------------------------------------------------------------------------- */
#define REPOSITORY_GOOGLE_TEST "https://github.com/google/googletest"
#define REPOSITORY_GOOGLE_BENCHMARK "https://github.com/google/benchmark.git"
#define REPOSITORY_LIBCHECK_TEST "https://github.com/libcheck/check"

#define GIT_INIT "git init"
#define GIT_SUBMODULE_ADD "git submodule add"
#define CG_GIT_INIT() CG_SYSTEM(GIT_INIT)

#define CG_SYSTEM(x)\
    if (system(x) < 0) {\
        ERROR_AND_EXIT("ERROR: executing %s", x);\
    }

#define CG_SWITCH_DIRECTORY_A_EXEC(DIR, E)\
    chdir(DIR);\
    CG_SYSTEM((E));\
    chdir("..")

#define CG_MKDIR(X)\
    if (mkdir(X, S_IRWXU) < 0) {\
        ERROR("ERROR: mkdir(): %s: ", X);\
        perror(NULL);\
        exit(1);\
    }

#define CG_MKDIR_W_GIT(X, Y)\
    CG_MKDIR(X);\
    if (Y) {\
        CG_SWITCH_DIRECTORY_A_EXEC(X, GIT_INIT);\
    }

static void CG_ADD_SUBMODULE(const char* dir, const char* repo) {
    size_t buffer_size = strlen(GIT_SUBMODULE_ADD) + strlen(repo) + 3;
    char buffer[buffer_size];
    snprintf(buffer, buffer_size, "%s %s\0", GIT_SUBMODULE_ADD, repo);

    CG_SWITCH_DIRECTORY_A_EXEC(dir, buffer);
}

/* -------------------------------------------------------------------------------------------- */
typedef struct {
    bool init;
    bool new;
} Args;

#define IS_ADD_SUPPLIED (flags.test || flags.benchmark)

typedef struct {
    bool test,
         benchmark,
         make_random_dir,
         initialize_git_repo,
         sprinkle_w_numerics,
         add_libcheck,
         make_c_files;
} Flags;

static void mk_flags(Flags* flags) {
    flags->initialize_git_repo = true;
}

/* -------------------------------------------------------------------------------------------- */
static int exists(const char* dir) {
    struct stat _stat;
    if (stat(&dir, &stat) == 0 && S_ISDIR(_stat.st_mode)) {
        return 0;
    }
    return -1;
}

__attribute__ ((const)) static const char* get_curr_path(void) {
    char* curr_path = getenv("PWD");
    if (curr_path == NULL) {
        fprintf(stdout, "This thing uses getenv which requires PWD env\n");
        exit(1);
    }
    return curr_path;
}

__attribute__ ((pure, malloc)) static char* get_curr_folder(char* path, size_t path_size) {
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

__attribute__((malloc)) static char* v_append_path(const char* path, const char* files, ...) {
    va_list ap;
    va_start(ap, files);

    char* file = files;

    uint8_t path_size = strlen(path) + strlen(file) + 2;
    char* _path = (char*) malloc(path_size);

    snprintf(_path, path_size, "%s/%s", path, file);
    file = va_arg(ap, char*);

    if (file != NULL) {
        do {
            char old_path[path_size];
            memcpy(old_path, _path, path_size);

            path_size += strlen(file) + 2;
            _path = realloc(_path, path_size);

            snprintf(_path, path_size, "%s/%s", old_path, file);
        } while (file = va_arg(ap, char*), file != NULL);
    }

    va_end(ap);
    return _path;
}

__attribute__((malloc, always_inline)) static char* append_path(const char* path, char* file) {
    return v_append_path(path, file, NULL);
}

static bool is_vaild_string_of_ints(char* str, size_t len) {
    int i = 0;
    for(; i < len; ++i) {
        if (str[i] >= 48 && str[i] <= 57) {
        } else {
            return false;
        }
    }
    return true;
}

/* -------------------------------------------------------------------------------------------- */
typedef enum {
    cg_file_write,
    cg_file_append,
} cg_file_type;

static const char* __attribute__((always_inline)) cg_file_type_to_string(cg_file_type type) {
#define _FILE_TYPE_TO_STRING(X, Y)\
    case X:\
        return Y
    switch (type) {
        _FILE_TYPE_TO_STRING(cg_file_write, "w");
        _FILE_TYPE_TO_STRING(cg_file_append, "a");
    }
    return "???";
}

#define WRITE(X, Y, Z) CG_WRITE(cg_file_write, X, Y, Z)
#define WRITE_APPEND(X, Y, Z) CG_WRITE(cg_file_append, X, Y, Z)
static void CG_WRITE(cg_file_type type, const char* directory_path, const char* file_path, char* content, ...) {
    char* _file = append_path(directory_path, file_path);

    const char* mode = cg_file_type_to_string(type);
    FILE* fp = fopen(_file, mode);
    if (fp == NULL) {
        ERROR("ERROR: Writing to %s\n", _file);
        exit(1);
    }

    va_list args;

    va_start(args, content);
    vfprintf(fp, content, args);
    va_end(args);

    free(_file);
    fclose(fp);
}

/* -------------------------------------------------------------------------------------------- */
static const char* alphas = "abcdefghigklmopqrstuvxyz";
static const char* numerics = "0123456789";

int get_random_idx(const size_t range) {
    return rand() % range;
}

static char get_random_char(const char* chars, size_t len) {
    size_t n = get_random_idx(len);
    assert((n >= 0 && n < len) && "Random number generated is overflowing the chars buffer");
    return chars[n];
}

__attribute__((malloc, pure)) char* get_random_dir_name(const size_t len) {
    char* dir_name = (char*) malloc((len + 1)* sizeof(char));

    int i = 0;
    for(; i < len; ++i) {
        dir_name[i] = get_random_char(alphas, strlen(alphas));
    }
    dir_name[len] = '\0';
    return dir_name;
}

static void sprinkle_path_w_numerics(char* path, size_t len) {
    int offset = 1;
    int limit = len / 3;

    int i = 0;
    for(; i < limit; ++i) {
        int idx = rand() % len;
        (idx == 0) ? idx += offset : idx;
        assert((idx != 0 && idx <= len - 1) && "Index overflows the buffer");
        path[idx] = get_random_char(numerics, strlen(numerics));
    }
}

/* -------------------------------------------------------------------------------------------- */
typedef struct {
    char* name;
    char* path;
    char* directory;

    const char* test_repository;
} Config;

static void make_config(Config* config) {
    config->name = NULL;
    config->path = NULL;
    config->directory = NULL;
}

static void wreck_config(Config* config) {
    free(config->name);
    free(config->path);
}

static void mk_config_name_new(Config* config, char* curr) {
    config->name = strdup(curr);
    if (!config->name) perror("strdup");
}

static void mk_config_name_init(Config* config) {
    config->name = get_curr_folder(config->path, strlen(config->path));
}

static void mk_path(Config* config, Args args) {
    char* current_path = get_curr_path();

    if (args.init) {
        config->path = current_path;
    }

    if (args.new) {
        config->path = append_path(current_path, config->name);
    }
}

/* -------------------------------------------------------------------------------------------- */
static const char* help_message = "\
Usage: %s [ARGS...] [OPTIONS...]\n\
\n\
Description: Creates a temporary project\n\
\n\
Args:\n\
    new    Creates new project dir\n\
    init   Iniitializes new project in current dir\n\
\n\
Options:\n\
    -a, --add [test|bench]   generate test, benchmark dirs [test, bench]\n\
\n\
    -r, --random-dir [LEN]   create a random directory, default length is 3\n\
\n\
    -d, --no-git             do not initialize git repo. Will raise error in case of external modules required\n\
\n\
    -n, --numerics           Add numerics to directory name at random positions something like salting\n\
\n\
    -ac, --add-libcheck      Use libcheck for testing\n\
\n\
    -cc, --c-files           Generates c files\n\
\n\
    -h, --help               shows help message\n\
";

static void Usage(FILE* where) {
    fprintf(where, help_message, EXECUTABLE);
}

int main(int argc, char** argv) {
    srand(time(NULL));

    if (argc < 2) {
        Usage(stdout);
        exit(0);
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
                    ERROR("ERROR: Invaild use of --random-dir with new\n");
                    CG_PANIC(&config);
                }
                mk_config_name_new(&config, *curr);
                args.new = true;
                args_begin = curr + 1;
            }
        } else if (STRCMP(*args_begin, "init")) {
            mk_config_name_init(&config);
            args.init = true;
            args_begin += 1;
        } else if (STRCMP(*args_begin, "-a") || STRCMP(*args_begin, "--add")) {
            char** curr = args_begin + 1;
            if (curr == args_end) {
                ERROR("ERROR: missing +test +benchmark\n");
                Usage(stderr);
                exit(1);
            } else {
                size_t max_args_len = 2;
                char** list_args_begin = curr;
                char** list_args_end = (curr + 1 != args_end) ? (list_args_begin + max_args_len) : (args_end);
                
                if (*list_args_begin[0] != '+') { /* Check first arg */
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
        } else if (STRCMP(*args_begin, "-r") || STRCMP(*args_begin, "--random-dir")) {
            if (args.new) {            /*Raise an error if new is supplied as well*/
                ERROR("ERROR: Invaild use of new with --random-dir\n");
                CG_PANIC(&config);
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
                    ERROR("ERROR: Invaild %s, requires a int literal\n", curr);
                    CG_PANIC(&config);
                }
                dir_name_length = atoi(_dir_name_length);
                free(_dir_name_length);
                args_begin++;
            }

            flags.make_random_dir = true;
            args.new = true;

            char* dir_name = get_random_dir_name(dir_name_length);
            mk_config_name_new(&config, dir_name); /* try using it with -rd new suicide */
            free(dir_name);
            args_begin++;
        } else if (STRCMP(*args_begin, "-d") || STRCMP(*args_begin, "--no-git")) {
            flags.initialize_git_repo = false;
            args_begin++;
        } else if (STRCMP(*args_begin, "-n") || STRCMP(*args_begin, "--numerics")) {
            flags.sprinkle_w_numerics = true;
            args_begin++;
        } else if (STRCMP(*args_begin, "-lc") || STRCMP(*args_begin, "-add-libcheck")) {
            flags.add_libcheck = true;
            args_begin++;
        } else if (STRCMP(*args_begin, "-cc") || STRCMP(*args_begin, "--c-files")) {
            flags.make_c_files = true;
            args_begin++;
        } else if (STRCMP(*args_begin, "-h") || STRCMP(*args_begin, "--help")) {
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

    if (flags.sprinkle_w_numerics) {
        sprinkle_path_w_numerics(config.name, strlen(config.name));
    }

    mk_path(&config, args);

    config.directory = config.name;

    if (flags.add_libcheck) {
        config.test_repository = REPOSITORY_LIBCHECK_TEST;
    } else {
        config.test_repository = REPOSITORY_GOOGLE_TEST;
    }

    if (!flags.initialize_git_repo && IS_ADD_SUPPLIED) {
        ERROR("ERROR: Invaild use of -dg with test and bench flags\n");
        CG_PANIC(&config);
    }

    if (!args.init) {
        CG_MKDIR_W_GIT(config.directory, flags.initialize_git_repo);
    } else {
        if (flags.initialize_git_repo) {
            CG_SYSTEM(GIT_INIT);
        }
    }

    /* -------------------------------------------------------------------------------------------- */
    /* Create directories and files                                                                 */
    /* -------------------------------------------------------------------------------------------- */

    if (flags.make_c_files) {
        CG_WRITE(cg_file_write, config.path, "main.c", "/*%s*/\n", config.name);
        CG_WRITE(cg_file_append, config.path, "main.c", source_main);
        CG_WRITE(cg_file_write, config.path, "Makefile", c_makefile, config.name);
        goto DONE;
    }

    DirRoot Dir_Root = mk_dir_root(root_cmakelists, root_gitignore);
    char* directory_root = config.path;

    WRITE(directory_root, "CMakeLists.txt", Dir_Root.cmakelists);
    
    if (flags.initialize_git_repo) {
        WRITE(directory_root, ".gitignore", Dir_Root.gitignore);
    }

    DirSource Dir_Source = mk_dir_source(source_main, source_cmakelists);
    char* directory_source = append_path(config.path, "src");
    MKDIR_OR_PANIC(&config, directory_source);

    WRITE(directory_source, "main.cpp", Dir_Source.main);
    WRITE(directory_source, "CMakeLists.txt", Dir_Source.cmakelists);
    free(directory_source);

    if (flags.test) {
        const char* directory_test = append_path(config.path, "test");
        MKDIR_OR_PANIC(&config, directory_test);

        WRITE_APPEND(directory_root, "CMakeLists.txt", "add_subdirectory(test)\n");

        if (flags.add_libcheck) {
            DirTest Dir_Test = mk_dir_test(test_test_libcheck, test_cmakelists_libcheck);
            WRITE(directory_test, "test.c", Dir_Test.test);
            WRITE(directory_test, "CMakeLists.txt", Dir_Test.cmakelists);
        } else {
            DirTest Dir_Test = mk_dir_test(test_test_gtest, test_cmakelists_gtest);
            WRITE(directory_test, "test.cpp", Dir_Test.test);
            WRITE(directory_test, "CMakeLists.txt", Dir_Test.cmakelists);
        }

        CG_ADD_SUBMODULE(directory_test, config.test_repository);

        free(directory_test);
    }

    if (flags.benchmark) {
        if (!flags.test) {
            const char* directory_vendor = append_path(config.path, "vendor");
            MKDIR_OR_PANIC(&config, directory_vendor);

            if (flags.add_libcheck) {
                WRITE_APPEND(directory_root, "CMakeLists.txt", "add_subdirectory(vendor/check)\n");
            } else {
                WRITE_APPEND(directory_root, "CMakeLists.txt", "add_subdirectory(vendor/googletest)\n");
            }

            CG_ADD_SUBMODULE(directory_vendor, config.test_repository);

            free(directory_vendor);
        }

        DirBenchmark Dir_Benchmark = mk_dir_benchmark(benchmark_bench, benchmark_cmakelists);
        const char* directory_benchmark = append_path(config.path, "benchmark");
        MKDIR_OR_PANIC(&config, directory_benchmark);

        WRITE_APPEND(directory_root, "CMakeLists.txt", "add_subdirectory(benchmark)\n");
        WRITE(directory_benchmark, "bench.cpp", Dir_Benchmark.bench);
        WRITE(directory_benchmark, "CMakeLists.txt", Dir_Benchmark.cmakelists);

        CG_ADD_SUBMODULE(directory_benchmark, REPOSITORY_GOOGLE_BENCHMARK);

        free(directory_benchmark);
    }

    goto DONE;

DONE:
    fprintf(stdout, "cg: Created %s\n", config.directory);
    wreck_config(&config);
    return 0;
}
