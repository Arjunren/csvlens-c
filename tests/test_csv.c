#include "csvlens.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static void test_quoted_fields(void) {
    FILE *file = tmpfile();
    CsvRow row;
    char error[128];
    CHECK(file != NULL);
    fputs("name,note\r\n\"Ada, A.\",\"line 1\nline 2\"\r\n\"quote\",\"said \"\"hello\"\"\"", file);
    rewind(file);
    CHECK(csv_read_row(file, ',', &row, error, sizeof(error)) == CSVLENS_OK);
    CHECK(row.count == 2 && strcmp(row.fields[0], "name") == 0);
    csv_free_row(&row);
    CHECK(csv_read_row(file, ',', &row, error, sizeof(error)) == CSVLENS_OK);
    CHECK(strcmp(row.fields[0], "Ada, A.") == 0);
    CHECK(strcmp(row.fields[1], "line 1\nline 2") == 0);
    csv_free_row(&row);
    CHECK(csv_read_row(file, ',', &row, error, sizeof(error)) == CSVLENS_OK);
    CHECK(strcmp(row.fields[1], "said \"hello\"") == 0);
    csv_free_row(&row);
    CHECK(csv_read_row(file, ',', &row, error, sizeof(error)) == CSVLENS_EOF);
    fclose(file);
}

static void test_column_lookup(void) {
    char *fields[] = {"id", "display name", "score"};
    CsvRow row = {fields, 3};
    CHECK(csv_find_column(&row, "display name") == 1);
    CHECK(csv_find_column(&row, "missing") == -1);
}

int main(void) {
    test_quoted_fields();
    test_column_lookup();
    puts("csvlens tests passed");
    return 0;
}
