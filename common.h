/*
 * common.h
 * Copyright (C) 2015 SrikanthMalipatel <SrikanthMalipatel@Srikanth>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef COMMON_H
#define COMMON_H

// This contains all common definitions for client and server

// macros for constants
#define MAX_CREATOR_LEN 100

// all possible commands. update this when adding a new command
typedef enum {
    COMMAND_HELP=1,
    COMMAND_CREATOR,
    COMMAND_DISPLAY,
    COMMAND_NONE=-1,
}CommandID;
 
#endif /* !COMMON_H */
