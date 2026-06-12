#ifndef APPS_H
#define APPS_H

/* satoi no está en usrlib.h para evitar conflicto con test_util.h */
int satoi(const char *str, int *ok);

int cmd_mem(char **argv, int argc);
int cmd_ps(char **argv, int argc);
int cmd_loop(char **argv, int argc);
int cmd_kill(char **argv, int argc);
int cmd_nice(char **argv, int argc);
int cmd_block(char **argv, int argc);

#endif
