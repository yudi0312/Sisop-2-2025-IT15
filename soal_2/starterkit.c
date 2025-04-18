#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>

// Definisikan buffer yang lebih besar
#define BUFFER_SIZE 4096  // Diperbesar untuk memastikan cukup

// Deklarasi fungsi
void log_activity(const char *format, ...);
void daemon_setup();
int base64_decode(const char *input, char *output);
int base64_char_value(char c);
int is_valid_base64(const char *input);
int process_quarantine();
int rename_decrypted_file(const char *filepath);
void save_pid();
void move_to_quarantine();
void start_decrypt_daemon();
void shutdown_program();
void base64_encode(const unsigned char *input, int length, char *output);
void move_from_quarantine_to_starter_kit();
void eradicate_quarantine();
void signal_handler(int sig);
int main(int argc, char *argv[]);

// Fungsi logging
void log_activity(const char *format, ...) {
        FILE *log_file = fopen("activity.log", "a");
        if (log_file) {
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            
            // Format tanggal dan waktu baru: [dd-mm-YYYY][HH:MM:SS]
            fprintf(log_file, "[%02d-%02d-%04d][%02d:%02d:%02d] - ",
                    tm_info->tm_mday, tm_info->tm_mon + 1, 1900 + tm_info->tm_year,
                    tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    
            va_list args;
            va_start(args, format);
            vfprintf(log_file, format, args);
            va_end(args);
            
            fprintf(log_file, "\n");
            fclose(log_file);
        }
    }

// Setup daemon
void daemon_setup() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);
    
    setsid();
    umask(0);
   
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

// Base64 decoding table
int base64_char_value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

// Fungsi decode Base64
int base64_decode(const char *input, char *output) {
    int i = 0, j = 0;
    int pad = 0;
    int val[4];
    
    while (input[i]) {
        while (input[i] && base64_char_value(input[i]) == -1 && input[i] != '=') i++;
        
        for (int k = 0; k < 4; k++) {
            if (input[i] == '=') {
                pad++;
                val[k] = 0;
            } else {
                val[k] = base64_char_value(input[i]);
            }
            i++;
        }
        
        output[j++] = (val[0] << 2) | ((val[1] & 0x30) >> 4);
        if (pad < 2) output[j++] = ((val[1] & 0x0F) << 4) | ((val[2] & 0x3C) >> 2);
        if (pad < 1) output[j++] = ((val[2] & 0x03) << 6) | val[3];
    }
    output[j] = '\0';
    return j;
}

int is_valid_base64(const char *input) {
    size_t len = strlen(input);
    if (len == 0 || len % 4 != 0) {
        return 0;
    }
    
    int padding = 0;
    for (size_t i = 0; i < len; i++) {
        if (input[i] == '=') {
            if (i < len - 2) return 0;  // Padding hanya di 2 karakter terakhir
            padding++;
            if (padding > 2) return 0;
        } else if (base64_char_value(input[i]) == -1) {
            return 0;  // Karakter tidak valid
        }
    }
    return 1;
}

// Proses file di quarantine
int process_quarantine() {
    DIR *dir = opendir("quarantine");
    if (!dir) return 0;
    
    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char filepath[4096];
            snprintf(filepath, sizeof(filepath), "quarantine/%s", entry->d_name);
            count += rename_decrypted_file(filepath);
        }
    }
    closedir(dir);
    return count;
}

// Ganti nama file dengan hasil decode
int rename_decrypted_file(const char *filepath) {
    char decoded[4096];
    const char *filename = strrchr(filepath, '/');
    filename = filename ? filename + 1 : filepath;

    // Validasi apakah filename adalah Base64 yang valid
    if (!is_valid_base64(filename)) {
        return 0;
    }

    if (base64_decode(filename, decoded) <= 0) {
        return 0;
    }

    // Hapus newline atau whitespace di akhir hasil decode
    size_t decoded_len = strlen(decoded);
    while (decoded_len > 0 && (decoded[decoded_len-1] == '\n' || 
           decoded[decoded_len-1] == '\r' || decoded[decoded_len-1] == ' ')) {
        decoded[--decoded_len] = '\0';
    }

    char new_path[4096];
    snprintf(new_path, sizeof(new_path), "quarantine/%s", decoded);

    if (rename(filepath, new_path) == 0) {
        return 1;
    }
    return 0;

    if (strlen(decoded) >= 4096) {
        log_activity("Error: Nama file dekripsi terlalu panjang");
        return 0;
    }
}


// Simpan PID
void save_pid() {
    FILE *pid_file = fopen(".quarantine.pid", "w");
    if (pid_file) {
        fprintf(pid_file, "%d", getpid());
        fclose(pid_file);
    }
}

// Fungsi untuk memindahkan file dari starter_kit ke quarantine
void move_to_quarantine() {
        struct stat st;
        if (stat("starter_kit", &st) == -1) {
            log_activity("Error: Direktori starter_kit tidak ada");
            printf("Direktori starter_kit tidak ditemukan\n");
            return;
        }

    DIR *dir = opendir("starter_kit");
    if (!dir) {
        log_activity("Direktori starter_kit tidak ditemukan");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char src[4096], dest[4096];
            snprintf(src, sizeof(src), "starter_kit/%s", entry->d_name);

            // Encode nama file ke Base64
            char encoded[4096];
            base64_encode((unsigned char *)entry->d_name, strlen(entry->d_name), encoded);

            snprintf(dest, sizeof(dest), "quarantine/%s", encoded);

            if (rename(src, dest) == 0) {
                log_activity("File dipindah: %s → quarantine/%s", entry->d_name);
            } else {
                log_activity("Gagal memindahkan: %s", entry->d_name);
            }
        }
    }
    closedir(dir);
}

// Main daemon process
// Proses utama daemon
void start_decrypt_daemon() {
    daemon_setup();
    save_pid();
    mkdir("quarantine", 0755);
    move_to_quarantine(); // Pindahkan file dari starter_kit
    log_activity("Successfully started decryption process with PID %d.", getpid());
    
    int attempts = 0;
    while (1) {
        int processed = process_quarantine();
        if (processed == 0) {
            if (++attempts >= 3) { // Cek 3 kali berturut-turut
                log_activity("Tidak ada file baru. Daemon berhenti.");
                break;
            }
        } else {
            attempts = 0;
        }
        sleep(5);
    }
    remove(".quarantine.pid");
}

void shutdown_program() {
    FILE *pid_file = fopen(".quarantine.pid", "r");
    if (!pid_file) {
        log_activity("Shutdown gagal: File PID tidak ditemukan");
        printf("Daemon tidak aktif\n");
        return;
    }
    
    int pid;
    if (fscanf(pid_file, "%d", &pid) != 1) {
        log_activity("Shutdown gagal: Format PID tidak valid");
        printf("File PID korup\n");
        fclose(pid_file);
        return;
    }
    fclose(pid_file);

    // Cek apakah proses benar-benar ada
    if (kill(pid, 0) == -1) {
        if (errno == ESRCH) {
            log_activity("Shutdown gagal: Proses %d tidak ditemukan", pid);
            printf("Proses tidak aktif\n");
        } else {
            log_activity("Shutdown gagal: Gagal cek proses %d (%s)", pid, strerror(errno));
        }
        remove(".quarantine.pid");
        return;
    }

    if (kill(pid, SIGTERM) == 0) {
        log_activity("Daemon dimatikan (PID: %d)", pid);
        printf("Daemon dimatikan (PID: %d)\n", pid);
        remove(".quarantine.pid");
    } else {
        log_activity("Shutdown gagal: Gagal mengirim sinyal ke %d (%s)", pid, strerror(errno));
        printf("Gagal mematikan daemon\n");
    }
}

// Fungsi Base64 encoding
void base64_encode(const unsigned char *input, int length, char *output) {
    const char *base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i = 0, j = 0;
    unsigned char array3[3], array4[4];
    int pad = 0;

    while (length--) {
        array3[i++] = *(input++);
        if (i == 3) {
            array4[0] = (array3[0] & 0xfc) >> 2;
            array4[1] = ((array3[0] & 0x03) << 4) + ((array3[1] & 0xf0) >> 4);
            array4[2] = ((array3[1] & 0x0f) << 2) + ((array3[2] & 0xc0) >> 6);
            array4[3] = array3[2] & 0x3f;

            for (i = 0; i < 4; i++) {
                output[j++] = base64_chars[array4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for (int k = i; k < 3; k++) {
            array3[k] = '\0';
        }

        array4[0] = (array3[0] & 0xfc) >> 2;
        array4[1] = ((array3[0] & 0x03) << 4) + ((array3[1] & 0xf0) >> 4);
        array4[2] = ((array3[1] & 0x0f) << 2) + ((array3[2] & 0xc0) >> 6);

        for (int k = 0; k < i + 1; k++) {
            output[j++] = base64_chars[array4[k]];
        }

        while (i++ < 3) {
            output[j++] = '=';
        }
    }

    output[j] = '\0';
}

// Pindahkan file dari karantina ke starter_kit dengan encode nama
void move_from_quarantine_to_starter_kit() {
    struct stat st = {0};
    if (stat("starter_kit", &st) == -1) {
        mkdir("starter_kit", 0755);
    }

    DIR *dir = opendir("quarantine");
    if (!dir) {
        log_activity("Direktori quarantine tidak ditemukan");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char src[4096], dest[4096];
            snprintf(src, sizeof(src), "quarantine/%s", entry->d_name);

            // Decode nama file dari Base64
            char decoded[4096];
            if (is_valid_base64(entry->d_name)) {
                base64_decode(entry->d_name, decoded);
            } else {
                strncpy(decoded, entry->d_name, sizeof(decoded));
                decoded[sizeof(decoded) - 1] = '\0';
            }

            snprintf(dest, sizeof(dest), "starter_kit/%s", decoded);

            if (rename(src, dest) == 0) {
                log_activity("File dikembalikan: %s → %s", decoded);
            } else {
                log_activity("Gagal mengembalikan: %s", entry->d_name);
            }
        }
    }
    closedir(dir);
}

// Fungsi menghapus semua file dalam karantina
void eradicate_quarantine() {
    if (rmdir("quarantine") == 0) {
        log_activity("Direktori quarantine dihapus");
    }

    DIR *dir = opendir("quarantine");
    if (!dir) {
        log_activity("Gagal membuka direktori karantina");
        return;
    }

    struct dirent *entry;
    int deleted_count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        // Hanya proses file regular, skip direktori dan special files
        if (entry->d_type == DT_REG) {
            char filepath[4096];
            snprintf(filepath, sizeof(filepath), "quarantine/%s", entry->d_name);
            
            if (unlink(filepath) == 0) {
                log_activity("File dihapus: %s", entry->d_name);
                deleted_count++;
            } else {
                log_activity("Gagal menghapus: %s (Error: %s)", entry->d_name, strerror(errno));
            }
        }
    }
    closedir(dir);

    log_activity("Penghapusan selesai. Total file dihapus: %d", deleted_count);
}

void signal_handler(int sig) {
    log_activity("Menerima sinyal terminasi (%d)", sig);
    move_from_quarantine_to_starter_kit();
    exit(EXIT_SUCCESS);
}

// Fungsi main() WAJIB ADA
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: Jumlah argumen salah\n");
        printf("Penggunaan: ./starterkit [--decrypt|--shutdown|--quarantine|--return|--eradicate]\n");
        exit(EXIT_FAILURE);
    }

    if (argc == 2) {
        if (strcmp(argv[1], "--decrypt") == 0) {
            start_decrypt_daemon();
        } else if (strcmp(argv[1], "--shutdown") == 0) {
            shutdown_program();
        } else if (strcmp(argv[1], "--quarantine") == 0) {
            move_to_quarantine();
        } else if (strcmp(argv[1], "--return") == 0) {
            move_from_quarantine_to_starter_kit();
        } else if (strcmp(argv[1], "--eradicate") == 0) { // <-- Tambahan baru
            eradicate_quarantine();
        } else {
            printf("Perintah tidak valid.\n");
        }
    } else {
        printf("Penggunaan: ./starterkit [--decrypt|--shutdown|--quarantine|--return|--eradicate]\n");
    }
    return 0;
}