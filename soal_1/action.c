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