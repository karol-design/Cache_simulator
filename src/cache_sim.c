/*
 * 1) Read a single entry (line) from the memory trace file and
 * 2) Simulate the behaviour of the cache memory according to the chosen mode
 * 3) Generate output file based on simulation results
 * Formatted with CLang altered Google style
 */
#include <stdio.h>
#include <stdlib.h>

const char *filename = "test_file.trc";

int main() {
    /* Open the trace file */
    FILE *file_p;
    file_p = fopen(filename, "r");  // Open the file in read mode (txt file) and return a pointer to it
    if (file_p == NULL) {           // Test if the file has been opened sucessfully
        printf("Main: ERROR, NULL pointer returned when opening %s\n", filename);
        exit(EXIT_FAILURE);
    } else {
        printf("Main: File %s opened sucessfully\n", filename);
    }

    // Read data from the the trace file: read/write flag and 4 digits of the hex address
    for (int i = 0; i < 10; i++) {
        char rw_flag;
        char addr_hex[4];
        fscanf(file_p, "%c %c%c%c%c \n", &rw_flag, &addr_hex[0], &addr_hex[1], &addr_hex[2], &addr_hex[3]);
        printf("Main: (%d) Acesss = %c | Addr = 0x%c%c%c%c\n", i, rw_flag, addr_hex[0], addr_hex[1], addr_hex[2], addr_hex[3]);
    }

    /* Close the trace file */
    if (fclose(file_p) == EOF) {  // Close the file and test if successfully
        printf("Main: ERROR, EOF error returned when closing %s\n");
    } else {
        printf("Main: File %s closed sucessfully\n", filename);
    }

    return 0;
}