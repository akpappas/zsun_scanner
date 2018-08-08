#include "proc_limits.h"
#include "myhash.h"
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

float getMemoryUsage()
{
    FILE *mem_fp = fopen("/proc/meminfo", "r");
    if (mem_fp == NULL)
    {
        perror("fopen");
    }
    ssize_t read;
    char *line;
    size_t len = 0;
    double available, total;
    while (read = getline(&line, &len, mem_fp) != -1) {
        if (strstr(line, "MemAvailable") != NULL) {
            char buffer[32];
            len = strlen(line);
            size_t j = 0;
            for (size_t i = 0; i < len; i++) {
                if (line[i] >= '0' && line[i] <= '9') {
                    buffer[j] = line[i];
                    j++;
                }
            }
            available = atof(buffer);
        } else if (strstr(line, "MemTotal") != NULL) {
            char buffer[32];
            len = strlen(line);
            int j = 0;
            for (size_t i = 0; i < len; i++) {
                if (line[i] >= '0' && line[i] <= '9') {
                    buffer[j] = line[i];
                    j++;
                }
            }
            total = atof(buffer);
        }
    }
    float available_percent = available / total;
    fclose(mem_fp);
    return 1 - available_percent;
}

long int get_kb_occupied()
{
    pid_t pid = getpid();
    char buf[128];
    sprintf(buf,"/proc/%d/status",pid);
    FILE *mem_fp = fopen(buf, "r");
    if (mem_fp == NULL)
    {
        puts("cant read proc");
        printf("%s %d buf",buf,pid);
        perror("fopen");
    }
    ssize_t read;
    char *line=NULL;
    size_t len = 0;
    long int rss=0;
    while (read = getline(&line, &len, mem_fp) != -1) {
        if (strstr(line, "RSS") != NULL) {
            char buffer[32];
            len = strlen(line);
            size_t j = 0;
            for (size_t i = 0; i < len; i++) {
                if (line[i] >= '0' && line[i] <= '9') {
                    buffer[j] = line[i];
                    j++;
                }
            }
            rss=atol(buffer);
        }
    }
    fclose(mem_fp);
    return rss;
}

void track_filesize(char* filename,FILE **fp)
{
    struct stat info;
    stat(filename,&info);
    if ( info.st_size > 25*(1<<20) )
    {
        char buff[64];
        sprintf(buff,"%s_%ld",filename,time(NULL));
        fclose(*fp);
        int success= rename(filename,buff);
        if (!success)
        {
            puts("cant rename");
            perror("rename");
        }
        *fp = fopen(filename,"a");
        if (*fp == NULL)
        {
            puts("cant reopen timestamps");
            perror("fopen");
            abort();
        }
    }
    return;
}

void check_for_storage()
{
    char pwd[PATH_MAX];
    getcwd(pwd, PATH_MAX);
    struct statvfs info;
    statvfs(pwd, &info);
    time_t min = time(NULL);
    char *minfile = NULL;
    if (info.f_bfree <  1 << 12) {
        printf("Not enough space");
        DIR *dirp = opendir("ssid_log");
        if (dirp == NULL) {
            puts("cant open dir");
            perror("opendir");
            abort();
        }
        struct dirent *ep;
        struct stat file_stats;
        int init =0;
        while (info.f_bfree < 1 << 13) {
            int n = 0;
            while (ep = readdir(dirp)) {
                if (!strncmp(ep->d_name, "..", strlen(ep->d_name)) || !strncmp(ep->d_name, ".", strlen(ep->d_name))) {
                    continue;
                }
                stat(ep->d_name, &file_stats);
                if (file_stats.st_ctim.tv_sec < min) {
                    min = file_stats.st_ctim.tv_sec;
                    minfile = strdup(ep->d_name);
                }
                n++;
                init++;
            }
            if (init==0)
            {
                puts("Haven't stored any files. SD full");
                abort();
            }
            if (n == 0) {
                puts("No files left to delete");
                break;
            }
            char *rm_file = malloc((strlen("ssid_log/") + strlen(minfile) + 1) * sizeof(char));
            mempcpy(mempcpy(rm_file, "ssid_log/", strlen("ssid_log/")), minfile,  strlen(minfile) + 1);
            free(rm_file);
        }
    }
    free(minfile);
}




