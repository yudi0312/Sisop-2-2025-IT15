#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define LOG_FILE "debugmon.log"
#define MAX_CMD 256

void log_process(const char* proc_name, const char* status) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) {
        perror("fopen");
        return;
    }
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(log, "[%02d:%02d:%04d]-[%02d:%02d:%02d]_%s_%s\n",
        tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
        tm.tm_hour, tm.tm_min, tm.tm_sec,
        proc_name, status);
    fclose(log);
}

void list_processes(const char* user) {
    char command[MAX_CMD];
    snprintf(command, MAX_CMD, "ps -u %s -o pid,comm,pcpu,pmem", user);
    system(command);
}

void run_daemon(const char* user) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    } else if (pid > 0) {
        printf("Debugmon daemon started.\n");
        exit(0);
    }

    setsid(); 
    fclose(stdin); fclose(stdout); fclose(stderr); 

    while (1) {
        char command[MAX_CMD];
        snprintf(command, MAX_CMD, "ps -u %s -o comm=", user);
        FILE* fp = popen(command, "r");
        if (!fp) exit(1);

        char proc[128];
        while (fgets(proc, sizeof(proc), fp)) {
            proc[strcspn(proc, "\n")] = 0;
            log_process(proc, "RUNNING");
        }
        pclose(fp);
        sleep(5); 
    }
}

void stop_daemon(const char* user) {
    system("pkill -f './debugmon daemon'");
    log_process("stop", "RUNNING");
}

void fail_user_processes(const char* user) {
    char command[MAX_CMD];
    snprintf(command, MAX_CMD, "ps -u %s -o pid=", user);
    FILE* fp = popen(command, "r");
    if (!fp) exit(1);

    char pid_str[16];
    while (fgets(pid_str, sizeof(pid_str), fp)) {
        int pid = atoi(pid_str);
        if (pid > 0) {
            kill(pid, SIGKILL);
            log_process(pid_str, "FAILED");
        }
    }
    pclose(fp);

    FILE* lock = fopen(".debugmon_fail_mode", "w");
    if (lock) fclose(lock);
}

void revert_user_processes(const char* user) {
    remove(".debugmon_fail_mode");
    log_process("revert", "RUNNING");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <command> <user>\n", argv[0]);
        return 1;
    }

    const char* command = argv[1];
    const char* user = argv[2];

    if (access(".debugmon_fail_mode", F_OK) == 0 &&
        strcmp(command, "revert") != 0) {
        printf("User is locked by debugmon (fail mode). Only 'revert' allowed.\n");
        return 1;
    }

    if (strcmp(command, "list") == 0) {
        list_processes(user);
    } else if (strcmp(command, "daemon") == 0) {
        run_daemon(user);
    } else if (strcmp(command, "stop") == 0) {
        stop_daemon(user);
    } else if (strcmp(command, "fail") == 0) {
        fail_user_processes(user);
    } else if (strcmp(command, "revert") == 0) {
        revert_user_processes(user);
    } else {
        printf("Unknown command.\n");
    }

    return 0;
}
