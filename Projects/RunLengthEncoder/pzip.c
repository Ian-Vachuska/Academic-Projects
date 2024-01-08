#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>//linux


//#define CORES 4

volatile int turn = 1;
int CORES;
size_t segsize = sizeof(int) + sizeof(char);

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;

typedef struct {
    char *fn;
    int myturn;
} param_t;

typedef struct {
    int runl;
    char last, first;
} seg_t;

typedef struct {
    size_t fsize;
    seg_t *seg;
    char *tfn;
} ret_t;

int filePreviouslyEncoded(char *fn, char **fnames, int readuntil, int mode);

void waitMyTurn(int myturn);

void *myThread(void *param);

seg_t *outputThread(seg_t *prev, ret_t *curr);

int encodeSingle(char* fn);

void *encode(FILE *fpi, FILE *fpo);
//-------------------------------------------------------------------------------------------
int filePreviouslyEncoded(char *fn, char **fnames, int readuntil, int mode) {
    //return == -1 on error
    if (fnames == NULL) { return -1; }
    if (readuntil == -1) { return 0; }
    int i = 0;
    while (fnames[i] != NULL && i <= readuntil) {
        if (strcmp(fn, fnames[i]) == 0) {//strings equal
            //return > 0 on if file was found
            return i + 1;
        }
        i++;
    }
    //add file to list for next thread
    if(mode) {
        fnames[i] = fn;
    }
    //return == zero if file was not found
    return 0;
}

void waitMyTurn(int myturn) {
    pthread_mutex_lock(&m);
    while (turn < (myturn - CORES)) {
        pthread_cond_wait(&c, &m);
    }
    pthread_mutex_unlock(&m);
}

void *myThread(void *_param) {
    param_t *param = (param_t *) _param;
    waitMyTurn(param->myturn);

    int chr;
    FILE *fpi = fopen(param->fn, "r");
    if (fpi == NULL || (chr = fgetc(fpi)) == EOF) {
        printf("pzip: file1 [file2 ...]\n");
        exit(1);
    }
    ungetc(chr, fpi);
    FILE *fpo = fopen(strncat(param->fn, "_t\0", strlen(param->fn) + 3), "w");
    if (fpo == NULL) {
        exit(1);
    }

    ret_t *ret = encode(fpi, fpo);
    if(ret == NULL){
        exit(1);
    }
    ret->tfn = param->fn;
    free(param);
    return (void *) ret;
}
int encodeSingle(char* fn){
    FILE *singlefile = fopen(fn, "r");
    if(singlefile == NULL) {
        return -1;
    }
    ret_t *singleret = encode(singlefile, stdout);
    fwrite(&(singleret->seg->runl), sizeof(int), 1, stdout);
    putc(singleret->seg->last, stdout);
    free(singleret);
    return 0;
}
//-------------------------------------------------------------------------------------------
void *encode(FILE *fpi, FILE *fpo) {
    ret_t *ret = calloc(sizeof(ret_t),1);
    ret->fsize = 0;
    ret->tfn = NULL;
    ret->seg = calloc(sizeof(seg_t),1);
    ret->seg->runl = 1;
    int curr = fgetc(fpi);
    ret->seg->first = (char)curr;
    ret->seg->last = ret->seg->first;
    int next;
    while (!feof(fpi)) {
        //    sanity check      next cant equal EOF and still loop
        next = fgetc(fpi);
        while ((next != EOF) && (curr == next)) {
            ret->seg->runl++;
            next = fgetc(fpi);
        }
        if (feof(fpi)) {
            //return before writing last int/char
            fclose(fpi);
            if(fpo&&fpo!=stdout){
                fclose(fpo);
            }
            ret->seg->last = (char) curr;
            return (void *) ret;
            //break;
        }
        //write segment
        fwrite(&(ret->seg->runl), sizeof(int), 1, fpo);
        putc((char)curr, fpo);
        ret->fsize++;
        //reset runl
        ret->seg->runl = 1;
        curr = next;
    }
    fclose(fpi);
    fclose(fpo);
    free(ret->seg);
    free(ret);
    //file is empty, this shouldnt happen but handle it
    return NULL;
}

//-------------------------------------------------------------------------------------------
seg_t *outputThread(seg_t *prev, ret_t *curr) {
    unsigned long rc;
    FILE *fpo = fopen(curr->tfn, "r");
    if (fpo == NULL) {
        exit(1);
    }
    //if edge chars match
    seg_t *temp = malloc(sizeof(seg_t));
    temp->runl = curr->seg->runl;
    temp->first = curr->seg->first;
    temp->last = curr->seg->last;
    if (prev->last == curr->seg->first) {
        //sum rl
        temp->runl += prev->runl;
    }else {
        //put 1 int and 1 char(prev)
        if (prev->runl != 0) {
            fwrite(&prev->runl, sizeof(int), 1, stdout);
            putchar(prev->last);
        }
    }
    unsigned char buffer[segsize*(curr->fsize)];
    rc = fread(buffer, segsize, curr->fsize, fpo);
    if(rc != 0){
        fwrite(&buffer, segsize, curr->fsize, stdout);
    }
    free(prev);
    fclose(fpo);
    return temp;//prev == curr
}
//-------------------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("pzip: file1 [file2 ...]\n");
        exit(1);
    }
    CORES = get_nprocs_conf();
    if(argc == 2)
    {
        return encodeSingle(argv[1]);
    }
    pthread_t p[argc - 1];//thread array
    //init encoded file array
    int index[argc - 1];
    char **encodedfiles = calloc(argc, sizeof(char *));
    for (int fcount = 1; fcount < argc; fcount++) {
        index[fcount - 1] = filePreviouslyEncoded(
                argv[fcount],encodedfiles, argc, argc);
        if (!index[fcount - 1]) {
            //need to implement threadsleep for later threads
            param_t *param = malloc(sizeof(param_t));
            param->fn = malloc((strlen(argv[fcount]) + 3) * sizeof(char));
            strcpy(param->fn, argv[fcount]);
            param->myturn = fcount;
            pthread_create(&p[fcount - 1], NULL, myThread, (void *) param);
        }
    }
    ret_t **rets = malloc((argc - 1) * sizeof(ret_t));
    ret_t *ret = NULL;
    seg_t *prev = malloc( sizeof(seg_t));
    prev->runl = 0;
    prev->first = EOF;
    prev->last = EOF;
    for (int fcount = 1; fcount < argc; fcount++) {
        pthread_join(p[fcount - 1], (void **) &ret);
        rets[fcount - 1] = ret;
        int j = 0;
        pthread_mutex_lock(&m);
        turn++;
        pthread_cond_broadcast(&c);
        pthread_mutex_unlock(&m);
        do {
            if (j > 0) {
                fcount++;
                if (fcount >= argc) {
                    break;
                }
                ret = &ret[index[fcount - 1]-1];
            }
            prev = outputThread(prev, ret);
            j++;
        }//only read fnames that come before this fname
        while (filePreviouslyEncoded(argv[fcount], &argv[1], fcount - 1,0));
    }
    fwrite(&(prev->runl), sizeof(int), 1, stdout);
    putchar(prev->last);

    free(prev);
    for(int j = 0;j<argc-1;j++){
        if(index[j] == 0){
            remove(rets[j]->tfn);
            free(rets[j]);
        }
    }
    free(rets);
    free(encodedfiles);
    return 0;
}