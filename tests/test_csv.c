#include "csvlens.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_quoted_fields(void) {
    FILE *file = tmpfile();
    CsvRow row;
    char error[128];
    assert(file != NULL);
    fputs("name,note\r\n\"Ada, A.\",\"line 1\nline 2\"\r\n\"quote\",\"said \"\"hello\"\"\"", file);
    rewind(file);
    assert(csv_read_row(file, ',', &row, error, sizeof(error)) == CSVLENS_OK);
    assert(row.count == 2 && strcmp(row.fields[0], "name") == 0);
    csv_free_row(&row);
    assert(csv_read_row(file, ',', &row, error, sizeof(error)) == CSVLENS_OK);
    assert(strcmp(row.fields[0], "Ada, A.") == 0);
    assert(strcmp(row.fields[1], "line 1\nline 2") == 0);
    csv_free_row(&row);
    assert(csv_read_row(file, ',', &row, error, sizeof(error)) == CSVLENS_OK);
    assert(strcmp(row.fields[1], "said \"hello\"") == 0);
    csv_free_row(&row);
    assert(csv_read_row(file, ',', &row, error, sizeof(error)) == CSVLENS_EOF);
    fclose(file);
}

static void test_column_lookup(void) {
    char *fields[] = {"id", "display name", "score"};
    CsvRow row = {fields, 3};
    assert(csv_find_column(&row, "display name") == 1);
    assert(csv_find_column(&row, "missing") == -1);
}

int main(void) {
    test_quoted_fields();
    test_column_lookup();
    puts("csvlens tests passed");
    return 0;
}
