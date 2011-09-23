/* Copyright (c) 2011, Christopher Pavlina. All rights reserved. */

#ifndef _CONFIG_FILE_H
#define _CONFIG_FILE_H 1

/* Code for reading in the paths config file */
struct config_file {
    char const *crt1_32, *crti_32, *crtn_32, *ldso_32, *runtime_32,
         *crt1_64, *crti_64, *crtn_64, *ldso_64, *runtime_64,
         *llc, *llvm_as, *as, *ld;
};

/* Load the configuration file. Note that while the strings inside the struct
 * are malloc()ed, since they'll be held for the program's entire existence,
 * there's no reason to bother free()ing. If the file doesn't mention a certain
 * path, the path in the struct will be NULL. */
void load_config (struct config_file *cfg);

#endif /* _CONFIG_FILE_H */
