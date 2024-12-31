#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <float.h>
#include <stdint.h>

#define MAX_NAME_LEN 100
#define HASH_TABLE_SIZE 1000003 // A large prime for better hash distribution due to the big data set

typedef struct WeatherStation {
    char name[MAX_NAME_LEN];
    double min_temp;
    double max_temp;
    double total_temp;
    int count;
    struct WeatherStation* next; 
} WeatherStation;

WeatherStation* hash_table[HASH_TABLE_SIZE] = {NULL};

uint32_t hash(const char* str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % HASH_TABLE_SIZE;
}

WeatherStation* find_or_create_station(const char* name) {
    uint32_t index = hash(name);
    WeatherStation* current = hash_table[index];

    // Searching for the station in l.l at this hash index
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }

    // If not found, creating a new station
    WeatherStation* new_station = (WeatherStation*)malloc(sizeof(WeatherStation));
    if (!new_station) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    strcpy(new_station->name, name);
    new_station->min_temp = DBL_MAX;
    new_station->max_temp = DBL_MIN;
    new_station->total_temp = 0;
    new_station->count = 0;
    new_station->next = hash_table[index];
    hash_table[index] = new_station;

    return new_station;
}

void process_line(char* line) {
    char* name = strtok(line, ";");
    if (!name) return;
    char* temp_str = strtok(NULL, "\n");
    if (!temp_str) return;

    double temp = atof(temp_str);

    WeatherStation* station = find_or_create_station(name);
    if (temp < station->min_temp) station->min_temp = temp;
    if (temp > station->max_temp) station->max_temp = temp;
    station->total_temp += temp;
    station->count++;
}

void process_file_fread(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        process_line(line);
    }

    fclose(file);
}

void process_file_mmap(const char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat");
        close(fd);
        exit(EXIT_FAILURE);
    }

    char* addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    madvise(addr, sb.st_size, MADV_SEQUENTIAL);

    char* start = addr;
    char* end = addr + sb.st_size;

    while (start < end) {
        char* newline = memchr(start, '\n', end - start);
        size_t line_length;
        if (newline) {
            line_length = newline - start;
        } else {
            line_length = end - start;
        }

        char line[line_length + 1];
        memcpy(line, start, line_length);
        line[line_length] = '\0';
        process_line(line);

        if (newline) {
            start = newline + 1;
        } else {
            break;
        }
    }

    munmap(addr, sb.st_size);
    close(fd);
}

void print_results() {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        WeatherStation* current = hash_table[i];
        while (current) {
            double mean_temp = current->total_temp / current->count;
            printf("%s min=%.2f mean=%.2f max=%.2f\n",
                   current->name, current->min_temp, mean_temp, current->max_temp);
            current = current->next;
        }
    }
}

void reset_stations() {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        WeatherStation* current = hash_table[i];
        while (current) {
            WeatherStation* to_free = current;
            current = current->next;
            free(to_free);
        }
        hash_table[i] = NULL;
    }
}

int main() {
    // const char* filename = "q1-10mil.txt";
    const char* filename = "q1-50mil.txt";
    // const char* filename = "tests.txt";


    struct timespec start, end;

    // Process using fread
    clock_gettime(CLOCK_MONOTONIC, &start);
    process_file_fread(filename);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double fread_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    print_results();

    // Reset stations to mmap processing
    reset_stations();

    // Process using mmap
    clock_gettime(CLOCK_MONOTONIC, &start);
    process_file_mmap(filename);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double mmap_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // Print statements
    printf("\nTime taken using fread: %.2f seconds\n", fread_time);
    printf("Time taken using mmap: %.2f seconds\n", mmap_time);
    printf("Total time taken to process the dataset: %.2f seconds\n", fread_time + mmap_time);

    return 0;
}


// Writeup: 
//  When comparing fread and mmap for handling large datasets,
//  mmap is better in terms of efficiency. fread uses multiple 
//  read system calls to load chunks of data into a buffer, 
//  which adds significant overhead due to frequent kernel-user 
//  space context switches—about 200,000 calls for a 50-million-line 
//  file with a 256-byte buffer. Each read involves transferring data 
//  between the kernel and user space, creating a performance bottleneck. 
//  On the other hand, mmap maps the file directly into memory, allowing 
//  the program to access it as if it were an array, without repeatedly 
//  invoking read system calls. Under the hood, mmap leverages virtual 
//  memory, loading file pages into memory on demand via page faults, 
//  which is far more efficient. Using strace, we observe that mmap 
//  involves only a handful of system calls (mmap, munmap, madvise), 
//  while fread makes thousands. The difference is clear: fread takes
//  ~1.4 seconds longer to process the file than mmap. For large files, 
//  mmap is faster and more efficient since it reduces the system call 
//  overhead and handles file access more directly.