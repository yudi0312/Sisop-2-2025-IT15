#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <stdarg.h>  // WAJIB DITAMBAHKAN

// Deklarasi fungsi (PROTOTIPE) 
void log_activity(const char *format, ...);  // <-- PERUBAHAN DI SINI
void setup_starterkit();
void move_to_quarantine(char *filename);
void move_to_starterkit();
void run_command(char *argv[]);
void save_pid();
int read_pid();
void shutdown_program();
int base64_char_to_value(char c);
int base64_decode(const char *input, unsigned char *output);
void decrypt_filename(char *filepath);
void check_and_decrypt_quarantine();
void start_decrypt();
void remove_files_in_directory(const char *dir_path);
void daemonize();

// Helper function: Menjalankan command
void run_command(char *argv[]) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork gagal");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        execvp(argv[0], argv);
        perror("exec gagal");
        exit(EXIT_FAILURE);
    } else {
        waitpid(pid, NULL, 0);
    }
}

// Definisi fungsi log_activity
void log_activity(const char *format, ...) {
    FILE *log_file = fopen("activity.log", "a");
    if (log_file) {
        time_t now = time(NULL);
        fprintf(log_file, "[%ld] ", now);

        va_list args;
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);

        fprintf(log_file, "\n");
        fclose(log_file);
    }
}

// Setup starter kit
void setup_starterkit() {
    char *wget_cmd[] = {"wget", "https://drive.usercontent.google.com/u/0/uc?id=1_5GxIGfQr3mNKuavJbte_AoRkEQLXSKS&export=download", "-O", "starter_kit.zip", NULL};
    run_command(wget_cmd);

    char *mkdir_cmd[] = {"mkdir", "-p", "starter_kit", NULL};
    run_command(mkdir_cmd);

    char *unzip_cmd[] = {"unzip", "starter_kit.zip", "-d", "starter_kit", NULL};
    run_command(unzip_cmd);

    char *rm_cmd[] = {"rm", "starter_kit.zip", NULL};
    run_command(rm_cmd);
    log_activity("Starter kit setup selesai");
}

// Fungsi Base64
int base64_char_to_value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

int base64_decode(const char *input, unsigned char *output) {
    int i = 0, j = 0;
    int input_len = strlen(input);
    int pad = 0;

    while (i < input_len) {
        // Skip karakter non-Base64
        while (i < input_len && base64_char_to_value(input[i]) == -1 && input[i] != '=') i++;

        int values[4] = {0};
        for (int k = 0; k < 4; k++) {
            if (i >= input_len) break;
            
            if (input[i] == '=') pad++;
            else values[k] = base64_char_to_value(input[i]);
            i++;
        }

        output[j++] = (values[0] << 2) | ((values[1] & 0x30) >> 4);
        if (pad < 2) output[j++] = ((values[1] & 0x0F) << 4) | ((values[2] & 0x3C) >> 2);
        if (pad < 1) output[j++] = ((values[2] & 0x03) << 6) | values[3];
    }
    return j - pad;
}

// Decrypt filename
void decrypt_filename(char *filepath) {
    char *filename = strrchr(filepath, '/');
    filename = filename ? filename + 1 : filepath;

    unsigned char decoded[256];
    int len = base64_decode(filename, decoded);
    if (len <= 0) {
        log_activity("Dekripsi gagal: nama file tidak valid");
        return;
    }
    decoded[len] = '\0';

    char new_path[512];
    snprintf(new_path, sizeof(new_path), "quarantine/%s", decoded);

    if (rename(filepath, new_path) != 0) {
        log_activity("Gagal dekripsi file: %s", filename);
    } else {
        log_activity("File didekripsi: %s -> %s", filename, decoded);
    }
}

// Pindahkan file ke karantina
void move_to_quarantine(char *filename) {
    char src[512], dest[512];
    snprintf(src, sizeof(src), "starter_kit/%s", filename);
    snprintf(dest, sizeof(dest), "quarantine/%s", filename);

    if (rename(src, dest) != 0) {
        perror("Gagal pindah ke karantina");
        log_activity("Gagal pindah ke karantina");
    } else {
        log_activity("File dipindah ke karantina");
    }
}

// Pindahkan file kembali ke starter_kit
void move_to_starterkit() {
    DIR *dir = opendir("quarantine");
    if (!dir) {
        perror("Gagal buka karantina");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char src[512], dest[512];
            snprintf(src, sizeof(src), "quarantine/%s", entry->d_name);
            snprintf(dest, sizeof(dest), "starter_kit/%s", entry->d_name);
            if (rename(src, dest) != 0) {
                perror("Gagal mengembalikan file");
                log_activity("Gagal mengembalikan file");
            } else {
                log_activity("File dikembalikan");
            }
        }
    }
    closedir(dir);
}

// Daemon process
void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    setsid();
    chdir("/");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

void check_and_decrypt_quarantine() {
    // Pindahkan semua file dari starter_kit ke quarantine
    DIR *dir_starter = opendir("starter_kit");
    if (dir_starter) {
        struct dirent *entry;
        while ((entry = readdir(dir_starter)) != NULL) {
            if (entry->d_type == DT_REG) {
                move_to_quarantine(entry->d_name);
            }
        }
        closedir(dir_starter);
    }

    // Dekripsi file di quarantine
    DIR *dir_quarantine = opendir("quarantine");
    if (!dir_quarantine) return;

    struct dirent *entry;
    while ((entry = readdir(dir_quarantine)) != NULL) {
        if (entry->d_type == DT_REG) {
            char path[512];
            snprintf(path, sizeof(path), "quarantine/%s", entry->d_name);
            decrypt_filename(path);
        }
    }
    closedir(dir_quarantine);
}

void start_decrypt() {
    daemonize();
    save_pid();
    log_activity("Daemon decrypt aktif");
    
    while (1) {
        check_and_decrypt_quarantine();
        sleep(1); // Periksa setiap 1 detik
    }
}

// PID handling
void save_pid() {
    FILE *f = fopen(".starterkit.pid", "w");
    if (f) {
        fprintf(f, "%d", getpid());
        fclose(f);
    }
}

int read_pid() {
    FILE *f = fopen(".starterkit.pid", "r");
    if (!f) return -1;
    int pid;
    fscanf(f, "%d", &pid);
    fclose(f);
    return pid;
}

void shutdown_program() {
    int pid = read_pid();
    if (pid == -1) {
        log_activity("Daemon tidak aktif");
        return;
    }
    if (kill(pid, SIGTERM) == 0) {
        log_activity("Daemon dimatikan (PID: %d)", pid);
        remove(".starterkit.pid");
    } else {
        log_activity("Gagal mematikan daemon (PID: %d)", pid);
    }
}

// Hapus semua file di direktori
void remove_files_in_directory(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char path[512];
            snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);
            if (remove(path) == 0) {
                log_activity("File dihapus dari karantina: %s", entry->d_name);
            } else {
                log_activity("Gagal menghapus file: %s", entry->d_name);
            }
        }
    }
    closedir(dir);
    log_activity("Semua file di karantina telah dihapus");
}

int main(int argc, char *argv[]) {
    mkdir("quarantine", 0777); // Pastikan direktori karantina ada
    if (access("starter_kit", F_OK) == -1) setup_starterkit();

    if (argc < 2) {
        printf("Penggunaan: ./starterkit [--decrypt|--quarantine|--return|--shutdown|--eradicate]\n");
        return 0;
    }

    if (strcmp(argv[1], "--quarantine") == 0) {
        DIR *dir = opendir("starter_kit");
        if (!dir) return EXIT_FAILURE;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_REG) move_to_quarantine(entry->d_name);
        }
        closedir(dir);
    } else if (strcmp(argv[1], "--return") == 0) {
        move_to_starterkit();
    } else if (strcmp(argv[1], "--decrypt") == 0) {
        start_decrypt();
    } else if (strcmp(argv[1], "--shutdown") == 0) {
        shutdown_program();
    } else if (strcmp(argv[1], "--eradicate") == 0) {
        remove_files_in_directory("quarantine");
        log_activity("Karantina dihapus");
    } else {
        fprintf(stderr, "Opsi tidak valid\n");
        return EXIT_FAILURE;
    }

    return 0;
}