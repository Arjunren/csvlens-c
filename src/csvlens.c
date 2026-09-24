#include "csvlens.h"

#include <stdlib.h>
#include <string.h>

static int grow_buffer(char **buffer, size_t *capacity, size_t needed) {
    char *next;
    size_t new_capacity = *capacity ? *capacity : 64;
    while (new_capacity < needed) {
        if (new_capacity > ((size_t)-1) / 2) return 0;
        new_capacity *= 2;
    }
    next = realloc(*buffer, new_capacity);
    if (!next) return 0;
    *buffer = next;
    *capacity = new_capacity;
    return 1;
}

static int add_field(CsvRow *row, const char *buffer, size_t length) {
    char **fields = realloc(row->fields, (row->count + 1) * sizeof(*fields));
    char *value;
    if (!fields) return 0;
    row->fields = fields;
    value = malloc(length + 1);
    if (!value) return 0;
    memcpy(value, buffer, length);
    value[length] = '\0';
    row->fields[row->count++] = value;
    return 1;
}

void csv_free_row(CsvRow *row) {
    size_t i;
    if (!row) return;
    for (i = 0; i < row->count; ++i) free(row->fields[i]);
    free(row->fields);
    row->fields = NULL;
    row->count = 0;
}

CsvStatus csv_read_row(FILE *input, char delimiter, CsvRow *row, char *error, size_t error_size) {
    char *field = NULL;
    size_t length = 0, capacity = 0;
    int c, quoted = 0, started = 0;
    row->fields = NULL;
    row->count = 0;

    while ((c = fgetc(input)) != EOF) {
        started = 1;
        if (quoted) {
            if (c == '"') {
                int next = fgetc(input);
                if (next == '"') {
                    if (!grow_buffer(&field, &capacity, length + 2)) goto memory_error;
                    field[length++] = '"';
                } else {
                    quoted = 0;
                    if (next != EOF) ungetc(next, input);
                }
            } else {
                if (!grow_buffer(&field, &capacity, length + 2)) goto memory_error;
                field[length++] = (char)c;
            }
        } else if (c == '"' && length == 0) {
            quoted = 1;
        } else if (c == delimiter || c == '\n' || c == '\r') {
            if (!add_field(row, field ? field : "", length)) goto memory_error;
            length = 0;
            if (c == '\r') {
                int next = fgetc(input);
                if (next != '\n' && next != EOF) ungetc(next, input);
            }
            if (c != delimiter) {
                free(field);
                return CSVLENS_OK;
            }
        } else {
            if (!grow_buffer(&field, &capacity, length + 2)) goto memory_error;
            field[length++] = (char)c;
        }
    }

    if (ferror(input)) {
        snprintf(error, error_size, "failed while reading input");
        free(field);
        csv_free_row(row);
        return CSVLENS_ERROR;
    }
    if (!started && length == 0 && row->count == 0) {
        free(field);
        return CSVLENS_EOF;
    }
    if (quoted) {
        snprintf(error, error_size, "unterminated quoted field");
        free(field);
        csv_free_row(row);
        return CSVLENS_ERROR;
    }
    if (!add_field(row, field ? field : "", length)) goto memory_error;
    free(field);
    return CSVLENS_OK;

memory_error:
    snprintf(error, error_size, "out of memory");
    free(field);
    csv_free_row(row);
    return CSVLENS_ERROR;
}

void csv_write_field(FILE *output, const char *value, char delimiter) {
    const char *p;
    int quote = strchr(value, delimiter) || strchr(value, '"') || strchr(value, '\n') || strchr(value, '\r');
    if (quote) fputc('"', output);
    for (p = value; *p; ++p) {
        if (*p == '"') fputc('"', output);
        fputc(*p, output);
    }
    if (quote) fputc('"', output);
}

int csv_find_column(const CsvRow *header, const char *name) {
    size_t i;
    for (i = 0; i < header->count; ++i) {
        if (strcmp(header->fields[i], name) == 0) return (int)i;
    }
    return -1;
}
