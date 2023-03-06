/*
 * 1) Read a single entry (line) from the memory trace file and
 * 2) Simulate the behaviour of the cache memory according to the chosen mode
 * 3) Generate output file based on simulation results
 * Formatted with CLang altered Google style
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

const char *filename = "test_file.trc";

/* Type definitions */
typedef unsigned int uint_t;

typedef struct addr_bitfields {  // Address bitfields structure type
    uint_t addr;                 // Full memory address
    uint_t mmtb;                 // Main Memory Tag Bits
    uint_t cmbid;                // Cache Memory Block ID
    uint_t offset;               // Block Offset
} addr_bitfields_t;

typedef struct cache_mode {  // Cache memory mode details structure type
    uint_t mode_id;
    uint_t cache_block_size;    // No of words in a CM block
    uint_t no_of_cache_blocks;  // No of CM block
    uint_t cache_size;          // Total no of words in CM
    uint_t write_policy;          // Write policy (true = WAWB; false = WAWT)
} cache_mode_t;

typedef struct cache_modes {  // Wrapper struct to hold all 16 cache modes structs
    cache_mode_t mode[16];
} cache_modes_t;

const uint_t cache_modes_config[16][2] = {
    // CM block size, and no. of blocks
    {16, 8},   // Modes 1 & 9
    {16, 16},  // Modes 2 & 10
    {16, 32},  // Modes 3 & 11
    {16, 64},  // Modes 4 & 12
    {4, 64},   // Modes 5 & 13
    {8, 32},   // Modes 6 & 14
    {32, 8},   // Modes 7 & 15
    {64, 4},   // Modes 8 & 16
};

// struct cache_memory {
//     uint_t tag_bits[];    // Tag bits for all blocks (CMBID)
//     uint_t valid_bits[];  // Valid bits for all blocks (CMBID)
// }

/* Function prototypes */
addr_bitfields_t
char_hex_to_bitfields(char *addr_hex_char, cache_mode_t cm);
FILE *open_file();
void close_file(FILE *fp);
cache_modes_t populate_cache_modes_array();

int main() {
    cache_modes_t cache_modes = populate_cache_modes_array();

    for (int i = 0; i < 16; i++) {  // For each mode
        printf("\nmain: Testing mode no. %d\n", cache_modes.mode[i].mode_id);
        FILE *trace_file_p = open_file();  // Open the trace file
        while (feof(trace_file_p) == 0) {  // Loop through all the memory accesses
            char rw_flag, addr[5];         // Create a char to hold read/write information and a string to hold mem address in hex
            // Copy the next mem. access information to rw_flag and addr variables
            fscanf(trace_file_p, "%c %c%c%c%c \n", &rw_flag, &addr[0], &addr[1], &addr[2], &addr[3]);
            addr_bitfields_t addr_bitfields = char_hex_to_bitfields(addr, cache_modes.mode[i]);  // Extract the bitfields

            printf("main: Addr = %d | MMTB = %d | CMBID = %d | Offset = %d\n", addr_bitfields.addr, addr_bitfields.mmtb, addr_bitfields.cmbid, addr_bitfields.offset);

            /* Simulate the cache behaviour */
            // Compare the MMTB with TB of the given CMBID
            // Test the Validity bit
            // If hit, then nothing; if miss, then update the cache
        }
        close_file(trace_file_p);  // Close the trace file
    }

    return 0;
}

/* ----------------- Other function definitions ----------------- */

/**
 * @brief Convert HEX address in a string to addr. bitfields structure
 * @param addr_hex_char Pointer to a string with the address in hexadecimal format
 * @param cm Cache mode info (struct) currently in use
 * @return Addr. bitfields structure with ADDR, MMTB, CMBID and Offset memebers
 */
addr_bitfields_t char_hex_to_bitfields(char *addr_hex_char, cache_mode_t cm) {
    addr_bitfields_t bf;

    uint_t cmbid_length = (uint_t)log2(cm.no_of_cache_blocks);  // Calculate how many CMBID bits are in the addr.
    uint_t offset_length = (uint_t)log2(cm.cache_block_size);   // Calculate how many Offset bits are in the addr.
    uint_t mmtb_length = (16 - cmbid_length - offset_length);   // Calculate how many MMTB bits are in the addr.

    bf.addr = (uint_t)strtol(addr_hex_char, NULL, 16);  // Convert str hex to an unsigned int

    // Remove CMBID and Offset bits and shift MMTB bits to correct positions
    bf.mmtb = (bf.addr) >> (cmbid_length + offset_length);

    bf.cmbid = (bf.addr) << mmtb_length;  // Remove MMTB bits and keep only CMBID and Offset bits
    bf.cmbid = bf.cmbid & (0x0000FFFF);
    bf.cmbid = (bf.cmbid) >> (mmtb_length + offset_length);  // Remove Offset bits and shift CMBID bits

    bf.offset = (bf.addr) << (cmbid_length + mmtb_length);
    bf.offset = bf.offset & (0x0000FFFF);                     // Remove MMTB and CMBID bits
    bf.offset = (bf.offset) >> (cmbid_length + mmtb_length);  // Shift Offset bits

    return bf;
}

/**
 * @brief Open the trace file and report on the result
 * @return Pointer to the file
 */
FILE *open_file() {
    FILE *fp = fopen(filename, "r");  // Open the file in read mode (txt file) and return a pointer to it
    if (fp == NULL) {                 // Test if the file has been opened sucessfully
        printf("open_file: ERROR, NULL pointer returned when opening %s\n", filename);
        exit(EXIT_FAILURE);
    } else {
        printf("open_file: File %s opened sucessfully\n", filename);
    }

    return fp;
}

/**
 * @brief Close the trace file and report on the result
 * @param fp Pointer to the file which is to be closed
 */
void close_file(FILE *fp) {
    if (fclose(fp) == EOF) {  // Close the file and test if successfully
        printf("close_file: ERROR, EOF error returned when closing %s\n");
    } else {
        printf("close_file: File %s closed sucessfully\n", filename);
    }
}

/**
 * @brief Populate chache modes array with the values from cache_modes_config array
 * @return Cache modes structure
 */
cache_modes_t populate_cache_modes_array() {
    cache_modes_t cm;  // Wrapper struct with an array of 16 cache mode structures
    for (int i = 0; i < 16; i++) {
        cm.mode[i].mode_id = (i + 1);
        cm.mode[i].cache_block_size = cache_modes_config[i%8][0];
        cm.mode[i].no_of_cache_blocks = cache_modes_config[i%8][1];
        cm.mode[i].cache_size = cm.mode[i].cache_block_size * cm.mode[i].no_of_cache_blocks;
        cm.mode[i].write_policy = (i > 8);
    }
    return cm;
}