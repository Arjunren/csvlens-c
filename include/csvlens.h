#ifndef CSVLENS_H
#define CSVLENS_H

#include <stddef.h>
#include <stdio.h>

typedef struct {
    char **fields;
    size_t count;
} CsvRow;

typedef enum {
    CSVLENS_OK = 1,
    CSVLENS_EOF = 0,
    CSVLENS_ERROR = -1
} CsvStatus;

CsvStatus csv_read_row(FILE *input, char delimiter, CsvRow *row, char *error, size_t error_size);
void csv_free_row(CsvRow *row);
void csv_write_field(FILE *output, const char *value, char delimiter);
int csv_find_column(const CsvRow *header, const char *name);

#endif
