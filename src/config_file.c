/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#include "config_file.h"
#include "error.h"
#include "internal_paths.h"
#include "free_on_exit.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LINE_LENGTH 512

/* Try strdup() and fail on OOM */
static char *xstrdup (const char *s)
{
    char *dup = strdup (s);
    if (!dup) error_errno ();
    return dup;
}

/* Get an entire line into the buffer, not including \n. If the line doesn't
 * fit, complain.
 * buf: buffer to read into
 * sz: size of buffer
 * f: file to read from
 * lineno: which line we are on (for error messsages - 1-based)
 * return: feof? */
static int get_line (char *buf, size_t sz, FILE *f, size_t lineno)
{
    size_t i;
    int found_eol = 0;
    for (i = 0; i < (sz - 1); ++i) {
        int c = fgetc (f);
        if (c == EOF && feof (f)) {
            found_eol = 1;
            break;
        } else if (c == EOF) {
            error_errno ();
        } else if (c == '\n') {
            found_eol = 1;
            break;
        } else
            buf[i] = (char) c;
    }
    if (!found_eol) {
        error_message ("line %d in configuration file too long", lineno);
    }
    buf[i] = 0;
    return feof (f);
}

void load_config (struct config_file *cfg)
{
    FILE *f;
    char const *path;
    size_t lineno;
    char line[LINE_LENGTH];

    memset (cfg, 0, sizeof (*cfg));

    /* path should be CONFIG_FILE_PATH or getenv("ALCO_CONFIG") */
    path = getenv ("ALCO_CONFIG");
    if (!path) path = CONFIG_FILE_PATH;

    f = fopen (path, "r");
    if (!f) {
        warning_message ("could not open configuration file \"%s\";"
                " using defaults", path);
        return;
    }

    for (lineno = 1; !get_line (line, LINE_LENGTH, f, lineno); ++lineno) {
        /* Decomment */
        char *comm = strchr (line, '#');
        if (comm) *comm = 0;

        char *key, *value, *temp;

        /* Find the start of the key */
        for (key = line; *key; ++key) {
            if (*key != ' ' && *key != '\t') break;
        }
        if (!*key)
            /* No key found */
            continue;

        /* Find the end of the key */
        for (temp = key + 1; *temp; ++temp) {
            if (*temp == ' ' || *temp == '\t') {
                *temp = 0;
                break;
            }
        }

        /* Find the start of the value */
        for (value = temp + 1; *value; ++value) {
            if (*value != ' ' && *value != '\t') break;
        }
        if (!*value) {
            /* No value found */
            error_message ("line %d in configuration file %s invalid",
                    lineno, path);
        }

        /* Mask off extra space after the value */
        for (temp = value + 1; *temp; ++temp) {
            if (*temp == ' ' || *temp == '\t') {
                *temp = 0;
                break;
            }
        }

        /* Store the value */
        char *copy = xstrdup (value);
        free_on_exit (copy);
        if (!strcmp (key, "crt1-32")) {
            cfg->crt1_32 = copy;
        } else if (!strcmp (key, "crti-32")) {
            cfg->crti_32 = copy;
        } else if (!strcmp (key, "crtn-32")) {
            cfg->crtn_32 = copy;
        } else if (!strcmp (key, "ldso-32")) {
            cfg->ldso_32 = copy;
        } else if (!strcmp (key, "runtime-32")) {
            cfg->runtime_32 = copy;
        } else if (!strcmp (key, "crt1-64")) {
            cfg->crt1_64 = copy;
        } else if (!strcmp (key, "crti-64")) {
            cfg->crti_64 = copy;
        } else if (!strcmp (key, "crtn-64")) {
            cfg->crtn_64 = copy;
        } else if (!strcmp (key, "ldso-64")) {
            cfg->ldso_64 = copy;
        } else if (!strcmp (key, "runtime-64")) {
            cfg->runtime_64 = copy;
        } else if (!strcmp (key, "llc")) {
            cfg->llc = copy;
        } else if (!strcmp (key, "llvm-as")) {
            cfg->llvm_as = copy;
        } else if (!strcmp (key, "as")) {
            cfg->as = copy;
        } else if (!strcmp (key, "ld")) {
            cfg->ld = copy;
        } else {
            error_message ("line %d in configuration file %s: "
                    "invalid key \"%s\"\n"
                    "Valid keys are the same as for -path - try running with "
                    "-path=help.",
                    lineno, path, key);
        }
    }

    fclose (f);
}
