#ifndef _HELP_FUNCTIONS_H
#define _HELP_FUNCTIONS_H

#include <ctype.h>

struct mirror_initiator_command{
    char *content_server_ip;
    int argc;
    char **argv;
};

struct mirror_initiator_command create_command(char *mycmd, int length);

void delete_command(struct mirror_initiator_command scmd);

int is_number(char *str);

int int_cmp(const void *a, const void *b);

int safe_read(int fd, char *buffer, int length);
#endif
