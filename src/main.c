#include "csvlens.h"

#include <errno.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERROR_SIZE 256

static void usage(FILE *out) {
    fprintf(out,
        "CSVLens - streaming CSV inspection\n\n"
        "Usage:\n"
        "  csvlens info FILE [--delimiter CHAR]\n"
        "  csvlens head FILE [-n ROWS] [--delimiter CHAR]\n"
        "  csvlens select FILE --columns NAME,NAME [--delimiter CHAR]\n"
        "  csvlens find FILE --column NAME --contains TEXT [--delimiter CHAR]\n"
        "  csvlens stats FILE --column NAME [--delimiter CHAR]\n");
}

static const char *option_value(int argc, char **argv, const char *name) {
    int i;
    for (i = 3; i + 1 < argc; ++i) if (strcmp(argv[i], name) == 0) return argv[i + 1];
    return NULL;
}

static char delimiter_option(int argc, char **argv) {
    const char *value = option_value(argc, argv, "--delimiter");
    return value && value[0] ? value[0] : ',';
}

static int open_csv(const char *path, FILE **input) {
    *input = fopen(path, "rb");
    if (!*input) {
        fprintf(stderr, "csvlens: cannot open '%s': %s\n", path, strerror(errno));
        return 0;
    }
    return 1;
}

static int read_header(FILE *input, char delimiter, CsvRow *header) {
    char error[ERROR_SIZE];
    CsvStatus status = csv_read_row(input, delimiter, header, error, sizeof(error));
    if (status == CSVLENS_OK) return 1;
    fprintf(stderr, "csvlens: %s\n", status == CSVLENS_EOF ? "empty CSV file" : error);
    return 0;
}

static void write_row(const CsvRow *row, const int *indices, size_t count, char delimiter) {
    size_t i;
    for (i = 0; i < count; ++i) {
        size_t index = indices ? (size_t)indices[i] : i;
        if (i) fputc(delimiter, stdout);
        csv_write_field(stdout, index < row->count ? row->fields[index] : "", delimiter);
    }
    fputc('\n', stdout);
}

static int command_info(FILE *input, char delimiter) {
    CsvRow header, row;
    char error[ERROR_SIZE];
    size_t rows = 0, min_fields = (size_t)-1, max_fields = 0, i;
    if (!read_header(input, delimiter, &header)) return 2;
    while (1) {
        CsvStatus status = csv_read_row(input, delimiter, &row, error, sizeof(error));
        if (status == CSVLENS_EOF) break;
        if (status == CSVLENS_ERROR) { fprintf(stderr, "csvlens: %s\n", error); csv_free_row(&header); return 2; }
        if (row.count < min_fields) min_fields = row.count;
        if (row.count > max_fields) max_fields = row.count;
        ++rows;
        csv_free_row(&row);
    }
    printf("Columns: %zu\nRows: %zu\nConsistent width: %s\n", header.count, rows,
           rows == 0 || (min_fields == header.count && max_fields == header.count) ? "yes" : "no");
    for (i = 0; i < header.count; ++i) printf("  %zu: %s\n", i + 1, header.fields[i]);
    csv_free_row(&header);
    return 0;
}

static int command_head(FILE *input, char delimiter, size_t limit) {
    CsvRow row;
    char error[ERROR_SIZE];
    size_t shown = 0;
    while (shown <= limit) {
        CsvStatus status = csv_read_row(input, delimiter, &row, error, sizeof(error));
        if (status == CSVLENS_EOF) break;
        if (status == CSVLENS_ERROR) { fprintf(stderr, "csvlens: %s\n", error); return 2; }
        write_row(&row, NULL, row.count, delimiter);
        csv_free_row(&row);
        ++shown;
    }
    return 0;
}

static int parse_columns(CsvRow *header, const char *names, int **indices, size_t *count) {
    char *copy = malloc(strlen(names) + 1), *token;
    if (!copy) return 0;
    strcpy(copy, names);
    *indices = NULL; *count = 0;
    token = strtok(copy, ",");
    while (token) {
        int index = csv_find_column(header, token);
        int *next;
        if (index < 0) { fprintf(stderr, "csvlens: unknown column '%s'\n", token); free(copy); free(*indices); return 0; }
        next = realloc(*indices, (*count + 1) * sizeof(**indices));
        if (!next) { free(copy); free(*indices); return 0; }
        *indices = next; (*indices)[(*count)++] = index;
        token = strtok(NULL, ",");
    }
    free(copy);
    return *count > 0;
}

static int command_select(FILE *input, char delimiter, const char *columns) {
    CsvRow header, row;
    int *indices;
    size_t count;
    char error[ERROR_SIZE];
    if (!read_header(input, delimiter, &header)) return 2;
    if (!parse_columns(&header, columns, &indices, &count)) { csv_free_row(&header); return 2; }
    write_row(&header, indices, count, delimiter);
    csv_free_row(&header);
    while (1) {
        CsvStatus status = csv_read_row(input, delimiter, &row, error, sizeof(error));
        if (status == CSVLENS_EOF) break;
        if (status == CSVLENS_ERROR) { fprintf(stderr, "csvlens: %s\n", error); free(indices); return 2; }
        write_row(&row, indices, count, delimiter);
        csv_free_row(&row);
    }
    free(indices);
    return 0;
}

static int command_find(FILE *input, char delimiter, const char *column, const char *needle) {
    CsvRow header, row;
    char error[ERROR_SIZE];
    int index;
    if (!read_header(input, delimiter, &header)) return 2;
    index = csv_find_column(&header, column);
    if (index < 0) { fprintf(stderr, "csvlens: unknown column '%s'\n", column); csv_free_row(&header); return 2; }
    write_row(&header, NULL, header.count, delimiter);
    csv_free_row(&header);
    while (1) {
        CsvStatus status = csv_read_row(input, delimiter, &row, error, sizeof(error));
        if (status == CSVLENS_EOF) break;
        if (status == CSVLENS_ERROR) { fprintf(stderr, "csvlens: %s\n", error); return 2; }
        if ((size_t)index < row.count && strstr(row.fields[index], needle)) write_row(&row, NULL, row.count, delimiter);
        csv_free_row(&row);
    }
    return 0;
}

static int command_stats(FILE *input, char delimiter, const char *column) {
    CsvRow header, row;
    char error[ERROR_SIZE], *end;
    int index;
    size_t numeric = 0, missing = 0, invalid = 0;
    double min = DBL_MAX, max = -DBL_MAX, sum = 0.0;
    if (!read_header(input, delimiter, &header)) return 2;
    index = csv_find_column(&header, column);
    csv_free_row(&header);
    if (index < 0) { fprintf(stderr, "csvlens: unknown column '%s'\n", column); return 2; }
    while (1) {
        double value;
        CsvStatus status = csv_read_row(input, delimiter, &row, error, sizeof(error));
        if (status == CSVLENS_EOF) break;
        if (status == CSVLENS_ERROR) { fprintf(stderr, "csvlens: %s\n", error); return 2; }
        if ((size_t)index >= row.count || row.fields[index][0] == '\0') ++missing;
        else {
            errno = 0; value = strtod(row.fields[index], &end);
            if (errno || *end) ++invalid;
            else { if (value < min) min = value; if (value > max) max = value; sum += value; ++numeric; }
        }
        csv_free_row(&row);
    }
    printf("Column: %s\nNumeric values: %zu\nMissing values: %zu\nInvalid values: %zu\n", column, numeric, missing, invalid);
    if (numeric) printf("Minimum: %.10g\nMaximum: %.10g\nMean: %.10g\n", min, max, sum / (double)numeric);
    return 0;
}

int main(int argc, char **argv) {
    FILE *input;
    char delimiter;
    int result;
    if (argc == 2 && strcmp(argv[1], "--help") == 0) { usage(stdout); return 0; }
    if (argc < 3) { usage(stderr); return 2; }
    delimiter = delimiter_option(argc, argv);
    if (!open_csv(argv[2], &input)) return 2;
    if (strcmp(argv[1], "info") == 0) result = command_info(input, delimiter);
    else if (strcmp(argv[1], "head") == 0) {
        const char *value = option_value(argc, argv, "-n");
        result = command_head(input, delimiter, value ? (size_t)strtoul(value, NULL, 10) : 10);
    } else if (strcmp(argv[1], "select") == 0) {
        const char *columns = option_value(argc, argv, "--columns");
        if (!columns) { fprintf(stderr, "csvlens: --columns is required\n"); result = 2; }
        else result = command_select(input, delimiter, columns);
    } else if (strcmp(argv[1], "find") == 0) {
        const char *column = option_value(argc, argv, "--column");
        const char *needle = option_value(argc, argv, "--contains");
        if (!column || !needle) { fprintf(stderr, "csvlens: --column and --contains are required\n"); result = 2; }
        else result = command_find(input, delimiter, column, needle);
    } else if (strcmp(argv[1], "stats") == 0) {
        const char *column = option_value(argc, argv, "--column");
        if (!column) { fprintf(stderr, "csvlens: --column is required\n"); result = 2; }
        else result = command_stats(input, delimiter, column);
    } else { fprintf(stderr, "csvlens: unknown command '%s'\n", argv[1]); usage(stderr); result = 2; }
    fclose(input);
    return result;
}
