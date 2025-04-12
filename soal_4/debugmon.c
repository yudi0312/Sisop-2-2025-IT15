#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#define PROC_PATH "/proc"
#define LOGFILE "debugmon.log"

// Mengambil UID dari nama user
uid_t get_uid(const char *username) {
    struct passwd *pw = getpwnam(username);
    if (!pw) {
        fprintf(stderr, "User '%s' tidak ditemukan.\n", username);
        exit(EXIT_FAILURE);
    }
    return pw->pw_uid;
}

// Mengecek apakah string hanya berisi angka
int is_numeric(const char *str) {
    while (*str) {
        if (!isdigit(*str)) return 0;
        str++;
    }
    return 1;
}

// Menulis catatan ke file log
void write_log(const char *process_name, const char *status) {
    FILE *log = fopen(LOGFILE, "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);

    char time_str[64];
    strftime(time_str, sizeof(time_str), "[%d:%m:%Y]-[%H:%M:%S]", local_time);

    fprintf(log, "%s_%s_%s\n", time_str, process_name, status);
    fclose(log);
}

// Menjadikan proses sebagai daemon
void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    setsid();

    if (chdir("/home/yudi0312/Sisop-2-2025-IT15/soal_4/") < 0) exit(EXIT_FAILURE);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_RDWR);
}

// Memantau proses user dan mencatat log sesuai mode
void monitor_user(const char *username, int fail_mode) {
    uid_t target_uid = get_uid(username);

    while (1) {
        DIR *dir = opendir(PROC_PATH);
        if (!dir) {
            sleep(5);
            continue;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (!is_numeric(entry->d_name)) continue;

            int pid = atoi(entry->d_name);
            char path[256], line[256], proc_name[256] = "-";
            FILE *fp;
            uid_t proc_uid;

            snprintf(path, sizeof(path), PROC_PATH"/%d/status", pid);
            fp = fopen(path, "r");
            if (!fp) continue;

            while (fgets(line, sizeof(line), fp)) {
                if (sscanf(line, "Uid: %d", &proc_uid) == 1) break;
            }
            fclose(fp);

            if (proc_uid != target_uid) continue;

            snprintf(path, sizeof(path), PROC_PATH"/%d/comm", pid);
            fp = fopen(path, "r");
            if (fp) {
                fgets(proc_name, sizeof(proc_name), fp);
                proc_name[strcspn(proc_name, "\n")] = 0;
                fclose(fp);
            }

            if (strcmp(proc_name, "debugmon") == 0) {
                write_log(proc_name, "RUNNING");
            } else if (fail_mode) {
                if (kill(pid, SIGKILL) == 0) {
                    write_log(proc_name, "FAILED");
                }
            }
        }
        closedir(dir);
        sleep(5);
    }
}

// Menghentikan proses debugmon milik user
void stop_user_daemon(const char *username) {
    DIR *dir = opendir(PROC_PATH);
    if (!dir) return;

    uid_t uid_target = get_uid(username);
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (!is_numeric(entry->d_name)) continue;

        int pid = atoi(entry->d_name);
        char path[256], line[1024], exe_path[256];
        uid_t uid_proc;

        snprintf(path, sizeof(path), PROC_PATH"/%d/status", pid);
        FILE *fp = fopen(path, "r");
        if (!fp) continue;

        while (fgets(line, sizeof(line), fp)) {
            if (sscanf(line, "Uid: %d", &uid_proc) == 1) break;
        }
        fclose(fp);
        if (uid_proc != uid_target) continue;

        snprintf(path, sizeof(path), PROC_PATH"/%d/exe", pid);
        ssize_t len = readlink(path, exe_path, sizeof(exe_path) - 1);
        if (len == -1) continue;
        exe_path[len] = '\0';

        if (!strstr(exe_path, "debugmon")) continue;

        snprintf(path, sizeof(path), PROC_PATH"/%d/cmdline", pid);
        fp = fopen(path, "r");
        if (!fp) continue;
        size_t read_bytes = fread(line, 1, sizeof(line) - 1, fp);
        fclose(fp);

        line[read_bytes] = '\0';

        char *args[10];
        int argc = 0;
        char *ptr = line;
        while (ptr < line + read_bytes && argc < 10) {
            args[argc++] = ptr;
            ptr += strlen(ptr) + 1;
        }

        if (argc >= 3 && strstr(args[0], "debugmon") && strcmp(args[2], username) == 0) {
            if (kill(pid, SIGTERM) == 0) {
                printf("The debugmon daemon process (PID %d) for user '%s' has been successfully stopped.\n", pid, username);
            }
        }
    }
    closedir(dir);
}

// Fungsi utama program
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage:\n");
        printf("  %s list <user>    - Display all running processes of the user\n", argv[0]);
        printf("  %s daemon <user>  - Start debugmon as a daemon to monitor the user\n", argv[0]);
        printf("  %s stop <user>    - Stop the debugmon daemon monitoring the user\n", argv[0]);
        printf("  %s fail <user>    - Kill all user processes and block new ones\n", argv[0]);
        printf("  %s revert <user>  - Revert fail mode and allow user processes again\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "list") == 0) {
        uid_t uid = get_uid(argv[2]);
        DIR *dir = opendir(PROC_PATH);
        if (!dir) {
            perror("Gagal membuka /proc");
            return EXIT_FAILURE;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (!is_numeric(entry->d_name)) continue;

            int pid = atoi(entry->d_name);
            char path[256], line[256], proc_name[256] = "-";
            FILE *fp;
            uid_t proc_uid;
            double mem_kb = 0;

            snprintf(path, sizeof(path), PROC_PATH"/%d/status", pid);
            fp = fopen(path, "r");
            if (!fp) continue;

            while (fgets(line, sizeof(line), fp)) {
                if (sscanf(line, "Uid: %d", &proc_uid) == 1) break;
            }
            fclose(fp);
            if (proc_uid != uid) continue;

            snprintf(path, sizeof(path), PROC_PATH"/%d/comm", pid);
            fp = fopen(path, "r");
            if (fp) {
                fgets(proc_name, sizeof(proc_name), fp);
                proc_name[strcspn(proc_name, "\n")] = 0;
                fclose(fp);
            }

            snprintf(path, sizeof(path), PROC_PATH"/%d/statm", pid);
            fp = fopen(path, "r");
            if (fp) {
                long pages;
                if (fscanf(fp, "%ld", &pages) == 1) {
                    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024;
                    mem_kb = pages * page_size_kb;
                }
                fclose(fp);
            }

            printf("PID: %-6d CMD: %-20s CPU: %.2f%% MEM: %.2f KB\n", pid, proc_name, 0.0, mem_kb);
        }
        closedir(dir);

    } else if (strcmp(argv[1], "daemon") == 0) {
        daemonize();
        monitor_user(argv[2], 0);

    } else if (strcmp(argv[1], "fail") == 0) {
        daemonize();
        monitor_user(argv[2], 1);

    } else if (strcmp(argv[1], "stop") == 0 || strcmp(argv[1], "revert") == 0) {
        stop_user_daemon(argv[2]);

    } else {
        printf("Command not recognized.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
