/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

/*********
 * FAQ
 *
 * Q: Why is this code so shitty?
 * A: Because I wrote my own option parser. Don't really want to waste time on
 *    it. It works.
 *
 * Q: Why the hell did you write your own option parser?
 * A: Because the usual compiler option syntax is a mess. No decent parser will
 *    accept it. So I wrote an indecent parser.
 *
 * Q: Why is it so unconfigurable?
 * A: Because I'll never use this piece of crap anywhere else, and neither
 *    should you.
 *
 * Q: Will it segfault?
 * A: I believe not. Despite the awful coding practices here, I still think I
 *    was memory-safe.
 *
 * Q: WTF with the non-free()d malloc()s??
 * A: Not all memory allocated must be freed. This is the command-line options
 *    structure - only one will ever be used, and it lasts almost as long as
 *    the whole program. Why free it?
 *
 * Q: You should fix this.
 * A: That's not a question, dumbass.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "read_args.h"
#include "info.h"
#include "error.h"
#include "free_on_exit.h"

static void init_args (struct args *args);
static void usage (char const *argv0);
static void version (void);
static const char *get_machine (void);
static char *consume (int argc, char **argv, int *i, int len, int meta);

int
read_args(struct args *args, int argc, char **argv)
{
    int i;
    /* Count positions in collection arrays */
    int libs_count = 0, lib_dirs_count = 0, pkg_dirs_count = 0,
        llc_opts_count = 0, as_opts_count = 0, ld_opts_count = 0,
        sources_count = 0;

    init_args(args);

    for (i = 1; i < argc; ++i) {
        if (!strcmp (argv[i], "-h") ||
            !strcmp (argv[i], "-help") ||
            !strcmp (argv[i], "--help")) {
            usage (*argv);
            args->exit_code = 0;
            return 1;
        }

        else if (!strcmp (argv[i], "-version")) {
            version ();
            args->exit_code = 0;
            return 1;
        }

        else if (!strncmp (argv[i], "-o", 2)) {
            args->output = consume (argc, argv, &i, 2, 0);
        }

        else if (!strcmp (argv[i], "-v") || !strcmp (argv[i], "-verbose")) {
            args->verbose = 1;
        }

        else if (!strncmp (argv[i], "-path=", 6)) {
            /* Acceptable syntax:
             *  -path=help
             *  -path=key:value
             */
            /* Message is down here to fit it in... */
#define MSG "The -path argument can be used to give paths to files at compile" \
                " time.\nThe format is \"-path=key:value\", where the valid " \
                "keys are:\n  llc, llvm-as, as, ld, crt1-64, crti-64, " \
                "crtn-64, ldso-64,\n  crt1-32, crti-32, crtn-32, ldso-32, " \
                "runtime-64, runtime-32\n"
            if (!strcmp (argv[i], "-path=help")) {
                printf (MSG);
                args->exit_code = 0;
                return 1;
            }
#undef MSG

            // Separate key and value
            if (argv[i][6] == 0)
                error_message("-path option expects argument");
            char *key = argv[i] + 6;
            char *value = strchr (key, ':');
            if (key == value || !value) {
                // -path=:blah or -path=blah
                error_message("-path option must be given as -path=key:value");
            }
            // 'key' and 'value' don't quite point to key and value yet.
            // 'key' must be NUL-terminated, and 'value' must be advanced one.
            *value = 0; // NUL-terminate 'key'
            ++value;
            if (*value == 0)
                error_message("-path option must be given as -path=key:value");

            if (!strcmp (key, "llc"))
                args->llc = value;
            else if (!strcmp (key, "llvm-as"))
                args->llvm_as = value;
            else if (!strcmp (key, "as"))
                args->as = value;
            else if (!strcmp (key, "ld"))
                args->ld = value;
            else if (!strcmp (key, "crt1-64"))
                args->crt1_64 = value;
            else if (!strcmp (key, "crti-64"))
                args->crti_64 = value;
            else if (!strcmp (key, "crtn-64"))
                args->crtn_64 = value;
            else if (!strcmp (key, "ldso-64"))
                args->ldso_64 = value;
            else if (!strcmp (key, "crt1-32"))
                args->crt1_32 = value;
            else if (!strcmp (key, "crti-32"))
                args->crti_32 = value;
            else if (!strcmp (key, "crtn-32"))
                args->crtn_32 = value;
            else if (!strcmp (key, "ldso-32"))
                args->ldso_32 = value;
            else if (!strcmp (key, "runtime-64"))
                args->runtime_64 = value;
            else if (!strcmp (key, "runtime-32"))
                args->runtime_32 = value;
            else
                error_message ("invalid argument to -path");

        }

        else if (!strcmp (argv[i], "-path")) {
            // Check for and complain about:
            //  -path help
            //  -path key:value
            //  etc.
            error_message ("-path option must be given as -path=key:value"); 
        }

        else if (!strcmp (argv[i], "-list-paths")) {
            args->paths_only = 1;
        }

        else if (!strncmp (argv[i], "-llc", 4)) {
            if (llc_opts_count >= (LIST_ARG_MAX - 1)) {
                error_message ("too many occurrences of -llc");
            }
            args->llc_opts[llc_opts_count] = consume
                (argc, argv, &i, 4, 1);
            ++llc_opts_count;
        }

        else if (!strncmp (argv[i], "-as", 3)) {
            if (as_opts_count >= (LIST_ARG_MAX - 1)) {
                error_message ("too many occurrences of -as");
            }
            args->as_opts[as_opts_count] = consume
                (argc, argv, &i, 3, 1);
            ++as_opts_count;
        }

        else if (!strncmp (argv[i], "-ld", 3)) {
            if (ld_opts_count >= (LIST_ARG_MAX - 1)) {
                error_message ("too many occurrences of -ld");
            }
            args->ld_opts[ld_opts_count] = consume
                (argc, argv, &i, 3, 1);
            ++ld_opts_count;
        }

        else if (!strcmp (argv[i], "-g")) {
            args->debug = 1;
        }

        else if (!strncmp (argv[i], "-O", 2)) {
            if (argv[i][2] >= '0' && argv[i][2] <= '3')
                args->optlevel = argv[i][2] - '0';
            else
                error_message ("-O must be given as -O0, -O1, -O2, -O3");
        }

        /* This MUST stay before -m */
        else if (!strncmp (argv[i], "-malloc", 7)) {
            args->malloc = consume (argc, argv, &i, 7, 0);
        }

        /* This MUST stay after -malloc */
        else if (!strncmp (argv[i], "-m", 2)) {
            if (!strcmp (argv[i], "-m32"))
                args->machine = "32";
            else if (!strcmp (argv[i], "-m64"))
                args->machine = "64";
            else
                error_message ("-m must be given as -m32, -m64");
        }

        else if (!strcmp (argv[i], "-fPIC")) {
            args->fpic = 1;
        }

        else if (!strncmp (argv[i], "-l", 2)) {
            if (libs_count >= (LIST_ARG_MAX - 1)) {
                error_message ("too many occurrences of -l");
            }
            args->libs[libs_count] = consume
                (argc, argv, &i, 2, 0);
            ++libs_count;
        }

        else if (!strncmp (argv[i], "-L", 2)) {
            if (lib_dirs_count >= (LIST_ARG_MAX - 1)) {
                error_message ("too many occurrences of -L");
            }
            args->lib_dirs[lib_dirs_count] = consume
                (argc, argv, &i, 2, 0);
            ++lib_dirs_count;
        }

        else if (!strncmp (argv[i], "-P", 2)) {
            if (pkg_dirs_count >= (LIST_ARG_MAX - 1)) {
                error_message ("too many occurrences of -P");
            }
            args->pkg_dirs[pkg_dirs_count] = consume
                (argc, argv, &i, 2, 0);
            ++pkg_dirs_count;
        }

        else if (!strcmp (argv[i], "-emit-llvm")) {
            args->emit_llvm = 1;
        }

        else if (!strcmp (argv[i], "-S")) {
            args->assembly = 1;
        }

        else if (!strcmp (argv[i], "-c")) {
            args->objfile = 1;
        }

        else if (!strcmp (argv[i], "-nogc")) {
            args->nogc = 1;
        }

        else if (!strcmp (argv[i], "-nomemabort")) {
            args->nomemabort = 1;
        }

        else if (!strcmp (argv[i], "-noboundck")) {
            args->noboundck = 1;
        }

        else if (!strcmp (argv[i], "-sm")) {
            args->sm = 1;
        }

        else if (!strcmp (argv[i], "-free")) {
            args->free = consume (argc, argv, &i, 5, 0);
        }

        else if (!strcmp (argv[i], "-debug-mode")) {
            args->debug_mode = 1;
        }

        else if (!strcmp (argv[i], "-error-trace")) {
            args->error_trace = 1;
        }

        else if (!strcmp (argv[i], "-tokens")) {
            args->tokens_only = 1;
        }

        else if (!strcmp (argv[i], "-ast")) {
            args->ast_only = 1;
        }

        else if (!strcmp (argv[i], "-pre-ast")) {
            args->pre_ast_only = 1;
        }

        else if (!strcmp (argv[i], "-force-platform")) {
            args->force_platform = 1;
        }

        else if (!strcmp (argv[i], "-Wno-octalish")) {
            args->w_octalish = 0;
        }
        else if (!strcmp (argv[i], "-Woctalish")) {
            args->w_octalish = 1;
        }

        else if (*argv[i] != '-') {
            if (sources_count >= (LIST_ARG_MAX - 1)) {
                error_message ("too many source files");
            }
            args->sources[sources_count] = argv[i];
            ++sources_count;
        }

        else {
            error_message ("invalid option %s", argv[i]);
        }
    }

    return 0;
}

static void usage (char const *argv0)
{
    printf (
        "Usage: %s [options] SOURCES...\n"
        "\n"
        "  Options:\n"
        "    -h, -help         display help and exit\n"
        "    -version          display version information and exit\n"
        "    -o<file>          place the output into <FILE>\n"
        "    -v, -verbose      show all commands before executing\n"
        "\n"
        "    -path=<PATH>      set paths - try -path=help\n"
        "    -list-paths       list paths and exit\n"
        "\n"
        "    -llc<opt>         give <opt> to the LLVM static compiler\n"
        "    -as<opt>          give <opt> to the assembler\n"
        "    -ld<opt>          give <opt> to the linker\n"
        "    -g                include debugging information\n"
        "    -O<n>             set optimisation level (0, 1, 2, 3)\n"
        "    -m<bits>          set machine (32, 64)\n"
        "    -fPIC             generate position-independent code (implicit\n"
        "                      with non-executable packages)\n"
        "    -l<lib>           link with <lib>\n"
        "    -L<dir>           add <dir> to the library search path\n"
        "    -P<dir>           add <dir> to the package search path\n"
        "    -emit-llvm        use LLVM formats rather than machine code\n"
        "    -S                stop after generating assembly\n"
        "    -c                stop after assembling\n"
        "    -nogc             disable garbage collection\n"
        "    -nomemabort       do not automatically complain and abort on\n"
        "                      out-of-memory\n"
        "    -noboundck        do not use bounds-checking\n"
        "    -sm               enable systems programming mode\n"
        "    -malloc <func>    use <func> as the allocator\n"
        "    -free <func>      use <func> as the deallocator\n"
        "------------------------------------------------------------------\n"
        "    -W(no-)octalish   (do not) warn about the use of numbers like\n"
        "                      0755 (which is decimal 755 in Alpha).\n"
        "                      Default: do warn.\n"
        "------------------------------------------------------------------\n"
        "    -debug-mode       run in debug mode\n"
        "    -error-trace      print a stack trace for compiler errors\n"
        "    -tokens           dump the token list after lexing the first"
        "                      input, then quit\n"
        "    -ast              dump the AST after parsing, and quit\n"
        "    -pre-ast          dump the AST before type checking, and quit\n"
        "    -force-platform   force compiling on an unsupported platform\n",
        argv0);
}

static void version (void)
{
    printf ("%s: Alpha compiler version %s\n", APPNAME, VERSION);
    printf ("Copyright (c) %d, %s.\n", COPR_YEAR, COPR_BY);
}

/* Initialise the struct args */
static void init_args (struct args *args)
{
    /* Most of the fields initialise to 0 */
    memset (args, 0, sizeof (*args));

    /* Some do not */
    args->w_octalish = 1;

    args->sources = malloc (LIST_ARG_MAX * sizeof (*args->sources));
    if (args->sources == NULL) error_errno ();
    free_on_exit (args->sources);
    memset (args->sources, 0, LIST_ARG_MAX * sizeof (*args->sources));

    args->machine = get_machine ();

    args->libs = malloc (LIST_ARG_MAX * sizeof (*args->libs));
    if (args->libs == NULL) error_errno ();
    free_on_exit (args->libs);
    memset (args->libs, 0, LIST_ARG_MAX * sizeof (*args->libs));

    args->lib_dirs = malloc (LIST_ARG_MAX * sizeof (*args->lib_dirs));
    if (args->lib_dirs == NULL) error_errno ();
    free_on_exit (args->lib_dirs);
    memset (args->lib_dirs, 0, LIST_ARG_MAX * sizeof (*args->lib_dirs));

    args->pkg_dirs = malloc (LIST_ARG_MAX * sizeof (*args->pkg_dirs));
    if (args->pkg_dirs == NULL) error_errno ();
    free_on_exit (args->pkg_dirs);
    memset (args->pkg_dirs, 0, LIST_ARG_MAX * sizeof (*args->pkg_dirs));

    args->llc_opts = malloc (LIST_ARG_MAX * sizeof (*args->llc_opts));
    if (args->llc_opts == NULL) error_errno ();
    free_on_exit (args->llc_opts);
    memset (args->llc_opts, 0, LIST_ARG_MAX * sizeof (*args->llc_opts));

    args->as_opts = malloc (LIST_ARG_MAX * sizeof (*args->as_opts));
    if (args->as_opts == NULL) error_errno ();
    free_on_exit (args->as_opts);
    memset (args->as_opts, 0, LIST_ARG_MAX * sizeof (*args->as_opts));

    args->ld_opts = malloc (LIST_ARG_MAX * sizeof (*args->ld_opts));
    if (args->ld_opts == NULL) error_errno ();
    free_on_exit (args->ld_opts);
    memset (args->ld_opts, 0, LIST_ARG_MAX * sizeof (*args->ld_opts));
}

/* Get the default machine type (32 or 64) */
static const char *get_machine (void)
{
    if (sizeof (void*) == 4)
        return "32";
    else if (sizeof (void*) == 8)
        return "64";
    else {
        error_message ("cannot detect architecture - stop");
        /* Will not make it here, but appease the compiler */
        return NULL;
    }
}

/* Return the argument to the option, advancing 'i' if needed
 * Precondition: argv[*i] has at least 'len' characters
 * argc: main()'s argc
 * argv: main()'s argv
 * i: pointer to loop variable iterating over arguments
 * len: length of this argument's name
 * meta: whether the option's argument may also be an option */
static char *consume (int argc, char **argv, int *i, int len, int meta)
{
    if (argv[*i][len] == 0) {
        // -o arg
        ++*i;
        if (*i >= argc)
            error_message ("expected argument to option %s", argv[*i - 1]);
        if (!meta && *argv[*i] == '-')
            error_message ("argument must not start with -",
                    argv[*i - 1]);
        return argv[*i];
    } else {
        // -oarg
        if (!meta && *(argv[*i] + len) == '-')
            error_message ("argument must not start with -");
        return argv[*i] + len;
    }
}
