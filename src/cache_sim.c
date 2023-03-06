/*
 * 1) Read a single entry (line) from the memory trace file and
 * 2) Simulate the behaviour of the cache memory according to the chosen mode
 * 3) Generate output file based on simulation results
 * Formatted with CLang altered Google style
 */
#include <stdio.h>
#include <stdlib.h>

#define MMTB_LENGTH 3    // No. of addr bits for: Main Memory Tag Bits
#define CMBID_LENGTH 7   // No. of addr bits for: Cache Memory Block ID
#define OFFSET_LENGTH 6  // No. of addr bits for: Block Offset

const char *filename = "test_file.trc";

/* Type definitions */
typedef unsigned int uint_t;
typedef struct addr_bitfields {  // Address bitfields structure type
    uint_t addr;                 // Full memory address
    uint_t mmtb;                 // Main Memory Tag Bits
    uint_t cmbid;                // Cache Memory Block ID
    uint_t offset;               // Block Offset
} addr_bitfields_t;

/* Function prototypes */
addr_bitfields_t char_hex_to_bitfields(char *addr_hex_char);

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

    // Read all the data from the the trace file: read/write flag and 4 digits of the hex address
    while (feof(file_p) == 0) {
        static int i = 0;
        char rw_flag;
        char addr_hex[5] = "    ";  // Create a string to hold mem address in hex
        fscanf(file_p, "%c %c%c%c%c \n", &rw_flag, &addr_hex[0], &addr_hex[1], &addr_hex[2], &addr_hex[3]);
        addr_bitfields_t addr_bitfields = char_hex_to_bitfields(addr_hex);
        printf("Main: (%d) Acesss = %c | Addr = 0x%c%c%c%c\n", i++, rw_flag, addr_hex[0], addr_hex[1], addr_hex[2], addr_hex[3]);

        printf("Main: Addr = %d | MMTB = %d | CMBID = %d | Offset = %d\n",
               addr_bitfields.addr, addr_bitfields.mmtb, addr_bitfields.cmbid, addr_bitfields.offset);
    }

    /* Close the trace file */
    if (fclose(file_p) == EOF) {  // Close the file and test if successfully
        printf("Main: ERROR, EOF error returned when closing %s\n");
    } else {
        printf("Main: File %s closed sucessfully\n", filename);
    }

    return 0;
}

/**
 * @brief Convert HEX address in a string to addr. bitfields structure
 * @param addr_hex_char Pointer to a string with the address in hexadecimal format
 * @return Addr. bitfields structure with DDR, MMTB, CMBID and Offset memebers
 */
addr_bitfields_t char_hex_to_bitfields(char *addr_hex_char) {
    addr_bitfields_t bf;
    bf.addr = (uint_t)strtol(addr_hex_char, NULL, 16);  // Convert str hex to an unsigned int

    // Remove CMBID and Offset bits and shift MMTB bits to correct positions
    bf.mmtb = (bf.addr) >> (CMBID_LENGTH + OFFSET_LENGTH);

    bf.cmbid = (bf.addr) << MMTB_LENGTH;  // Remove MMTB bits and keep only CMBID and Offset bits
    bf.cmbid = bf.cmbid & (0x0000FFFF);
    bf.cmbid = (bf.cmbid) >> (MMTB_LENGTH + OFFSET_LENGTH);  // Remove Offset bits and shift CMBID bits

    bf.offset = (bf.addr) << (CMBID_LENGTH + MMTB_LENGTH);
    bf.offset = bf.offset & (0x0000FFFF);                     // Remove MMTB and CMBID bits
    bf.offset = (bf.offset) >> (CMBID_LENGTH + MMTB_LENGTH);  // Shift Offset bits

    return bf;
}