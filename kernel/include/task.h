#ifndef __TASK_H__
#define __TASK_H__

#include <stdint.h>
#include <stddef.h>
#include <paging.h>
#include <vfs.h>
#include <kernel.h>

#define KRN_STAT_ACTIVE_TASK    1
#define KRN_STAT_RES_TASK       2
#define KRN_STAT_IOWAIT_TASK    3
#define KRN_STAT_ZOMBIE_TASK    4
#define KRN_STAT_PROCWAIT_TASK  5

#define EMPTY_PID               (task_t *)0xffffffff
#define TASK_RESERVED_SPACE     0x10000
#define TASK_BASE               0x1000000


// signals

#define SIGABRT                 0
#define SIGFPE                  1
#define SIGILL                  2
#define SIGINT                  3
#define SIGSEGV                 4
#define SIGTERM                 5

#define SIG_ERR                 0xffffffff

/* i386 cpu struct */
typedef struct {

    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t eip;
    uint32_t cs;
    uint32_t ds;
    uint32_t es;
    uint32_t fs;
    uint32_t gs;
    uint32_t ss;
    uint32_t eflags;

} cpu_t;

typedef struct {

    int status;
    int parent;

    int64_t return_value;
    int *waitstat;

    pt_entry_t *page_directory;

    cpu_t cpu;

    char pwd[2048];

    char iowait_dev[2048];
    uint64_t iowait_loc;
    int iowait_type;
    uint8_t iowait_payload;
    int iowait_handle;
    uint32_t iowait_ptr;
    int iowait_len;
    int iowait_done;

    size_t heap_base;
    size_t heap_size;

    // signals
    size_t sigabrt;
    size_t sigfpe;
    size_t sigill;
    size_t sigint;
    size_t sigsegv;
    size_t sigterm;

    file_handle_t *file_handles;
    int file_handles_ptr;

} task_t;

typedef struct {
    char *path;
    char *stdin;
    char *stdout;
    char *stderr;
    char *pwd;
    char *unused0;
    char *unused1;
    int argc;
    char **argv;
    char **environ;
} task_info_t;

extern task_t **task_table;
extern int current_task;
extern int ts_enable;

void task_init(void);
int general_execute(task_info_t *);
int general_execute_block(task_info_t *);
void task_scheduler(void);
void task_quit(int, int64_t);

int kexec(  char *path, char **argv, char **envp,
            char *stdin, char *stdout, char *stderr,
            char *pwd );


#endif
