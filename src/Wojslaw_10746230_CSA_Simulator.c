/**
 * @file    cache_sim.c
 * @brief   Cache Memory Controller simulator (16 modes, .trc input, .csv output with sim stats)
 * @author  Karol Wojslaw (karol.wojslaw@student.manchester.ac.uk, student_ID: 10746230)
 * Formatted with CLang, altered Google style
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* Macros and const definitions */

#define WAWB 1  // Define write policy macros
#define WAWT 0

#define DEBUG_MESSAGES_ON 0  // Turn debug message ON/OFF with this macro
#define OUTPUT_CSV_HEADER 1  // Turn CSV header ON/OFF with this macro

static const char *input_file_name = "test_file.trc";
static const char *output_file_name = "Wojslaw_10746230_CSA_Results.csv";

/* Type definitions */
typedef unsigned int uint_t;
typedef unsigned int bool;

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
    uint_t write_policy;        // Write policy (true = WAWB; false = WAWT)
} cache_mode_t;

typedef struct cache_modes_arr {  // Wrapper struct to hold all 16 cache modes structs
    cache_mode_t cm_mode[16];
} cache_modes_arr_t;

static const uint_t cache_modes_config[16][2] = {
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

typedef struct cache_mem {  // Cache memory structure type
    uint_t tag_bits[64];    // Tag bits for all blocks (max size for CMBID used)
    uint_t valid_bits[64];  // Valid bits for all blocks (max size for CMBID used)
    uint_t dirty_bits[64];  // Dirty biits for all blocks (max size for CMBID used)
} cache_mem_t;

typedef struct cache_mem_stats {  // Cache memory statistcs structure
    uint_t mode_ID;               // Mode ID in which Cache Memory controller operates
    uint_t NRA;                   // Total number of read accesses to the external memory
    uint_t NWA;                   // Total number of write accesses to the external memory
    uint_t NCRH;                  // Number of cache read hits
    uint_t NCRM;                  // Number of cache read misses
    uint_t NCWH;                  // Number of cache write hits
    uint_t NCWM;                  // Number of cache write misses
} cache_mem_stats_t;

typedef struct cache_mem_stats_arr {  // Wrapper struct to hold all 16 cache statistics structs
    cache_mem_stats_t cm_stats[16];
} cache_mem_stats_arr_t;

/* Function prototypes */
addr_bitfields_t hex_to_bitfields(uint_t _addr, cache_mode_t cm_mode);
FILE *open_file(void);
void close_file(FILE *fp);
cache_modes_arr_t populate_modes_array(void);
void simulate_cache(cache_mem_t *cm, cache_mode_t cm_mode, addr_bitfields_t bf, char rw, cache_mem_stats_t *cm_stats);
void initialise_cache(cache_mem_t *cm);
void initialise_cache_stats(cache_mem_stats_t *cm_stats, uint_t _mode_ID);
void print_stats(cache_mem_stats_arr_t *stats);
void output_stats(cache_mem_stats_arr_t *stats);

/* Main function */

int main() {
    cache_modes_arr_t modes = populate_modes_array();
    cache_mem_stats_arr_t stats;
    cache_mem_t cm;

    for (uint_t i = 0; i < 16; i++) {  // For each Cache mode
        printf("\nmain: Testing mode no. %u, Write policy: %u\n", modes.cm_mode[i].mode_id, modes.cm_mode[i].write_policy);
        FILE *trace_file_p = open_file();               // Open the trace file
        initialise_cache(&cm);                          // Initialise cache memory for the next simulation
        initialise_cache_stats(&stats.cm_stats[i], i);  // Initialise cache memory stats

        while (feof(trace_file_p) == 0) {                                             // Loop through all memory accesses
            char rw_access;                                                           // Read/write information
            uint_t mem_addr;                                                          // Memory address
            fscanf(trace_file_p, "%c %x \n", &rw_access, &mem_addr);                  // Copy mem. access info. to rw_flag and addr variables
            addr_bitfields_t addr_bf = hex_to_bitfields(mem_addr, modes.cm_mode[i]);  // Extract the bitfields
            if (DEBUG_MESSAGES_ON) {
                printf("main: Addr %-5u | MMTB %-5u | CMBID %-5u | Offset %-5u | R/W %-2u\n", addr_bf.addr, addr_bf.mmtb, addr_bf.cmbid, addr_bf.offset, rw_access);
            }

            simulate_cache(&cm, modes.cm_mode[i], addr_bf, rw_access, &stats.cm_stats[i]);
        }

        close_file(trace_file_p);  // Close the trace file
    }

    print_stats(&stats);
    output_stats(&stats);

    return 0;
}

/* Function definitions */

/**
 * @brief Convert HEX address in a string to addr. bitfields structure
 * @param addr_hex_char Pointer to a string with the address in hexadecimal format
 * @param cm Cache mode info (struct) currently in use
 * @return Addr. bitfields structure with ADDR, MMTB, CMBID and Offset memebers
 */
addr_bitfields_t hex_to_bitfields(uint_t _addr, cache_mode_t cm_mode) {
    addr_bitfields_t bf;
    bf.addr = _addr;

    uint_t cmbid_length = (uint_t)log2(cm_mode.no_of_cache_blocks);  // Calculate how many CMBID bits are in the addr.
    uint_t offset_length = (uint_t)log2(cm_mode.cache_block_size);   // Calculate how many Offset bits are in the addr.
    uint_t mmtb_length = (16 - cmbid_length - offset_length);        // Calculate how many MMTB bits are in the addr.

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
    FILE *fp = fopen(input_file_name, "r");  // Open the file in read mode (txt file) and return a pointer to it
    if (fp == NULL) {                        // Test if the file has been opened sucessfully
        printf("open_file: ERROR, NULL pointer returned when opening %s\n", input_file_name);
        exit(EXIT_FAILURE);
    } else {
        printf("open_file: File %s opened sucessfully\n", input_file_name);
    }

    return fp;
}

/**
 * @brief Close the trace file and report on the result
 * @param fp Pointer to the file which is to be closed
 */
void close_file(FILE *fp) {
    if (fclose(fp) == EOF) {  // Close the file and test if successfully
        printf("close_file: ERROR, EOF error returned when closing %s\n", input_file_name);
    } else {
        printf("close_file: File %s closed sucessfully\n", input_file_name);
    }
}

/**
 * @brief Populate chache modes array with the values from cache_modes_config array
 * @return Cache modes structure
 */
cache_modes_arr_t populate_modes_array() {
    cache_modes_arr_t cm;  // Wrapper struct with an array of 16 cache mode structures
    for (uint_t i = 0; i < 16; i++) {
        cm.cm_mode[i].mode_id = (i + 1);
        cm.cm_mode[i].cache_block_size = cache_modes_config[i % 8][0];
        cm.cm_mode[i].no_of_cache_blocks = cache_modes_config[i % 8][1];
        cm.cm_mode[i].cache_size = cm.cm_mode[i].cache_block_size * cm.cm_mode[i].no_of_cache_blocks;
        cm.cm_mode[i].write_policy = (i < 8);
    }
    return cm;
}

/**
 * @brief Initialise the Cache Memory by reseting all its status bits (tag, valid, dirty)
 */
void initialise_cache(cache_mem_t *cm) {
    for (int i = 0; i < 64; i++) {  // Reset tag, valid and dirty bits for all blocks in the cache
        cm->tag_bits[i] = 0;
        cm->valid_bits[i] = 0;
        cm->dirty_bits[i] = 0;
    }
}

/**
 * @brief Initialise Cache Memory statistics structure by reseting all its entries
 */
void initialise_cache_stats(cache_mem_stats_t *cm_stats, uint_t _mode_ID) {
    cm_stats->mode_ID = _mode_ID;
    cm_stats->NCRH = 0;
    cm_stats->NCRM = 0;
    cm_stats->NCWH = 0;
    cm_stats->NCWM = 0;
    cm_stats->NRA = 0;
    cm_stats->NWA = 0;
}

/**
 * @brief Simulate the Cache Memory according to the Cache Mode and Unique mem. access bitfields / rw flag
 * @param cm Pointer to the cache memory structure
 * @param cm_mode Cache memory controller mode info structure
 * @param bf Bitfield structure for the specific memory access
 * @param rw Read/Write access type
 * @param cm_stats Pointer to the cache memory statistics structure
 */
void simulate_cache(cache_mem_t *cm, cache_mode_t cm_mode, addr_bitfields_t bf, char rw, cache_mem_stats_t *cm_stats) {
    // Test if Valid bit of the CM block is set (Something stored in the CM block)
    bool valid_bit_test = (cm->valid_bits[bf.cmbid] == 1);

    // Test if the tag bits for the block are iidentical to MM tag bits (Correct MM block stored in CM)
    bool tag_bit_test = (cm->tag_bits[bf.cmbid] == bf.mmtb);

    if (DEBUG_MESSAGES_ON) {
        printf("simulate_cache: Valid bit test %u | Tag bit test %u\n", valid_bit_test, tag_bit_test);
    }

    if (rw == 'R') {  // Read access
        if (valid_bit_test && tag_bit_test) {
            (cm_stats->NCRH)++;  // Increment Read Hit Count
            if (DEBUG_MESSAGES_ON) {
                printf("simulate_cache: Read Hit++\n");
            }
        } else {
            (cm_stats->NCRM)++;  // Increment Read Miss Count
            if (DEBUG_MESSAGES_ON) {
                printf("simulate_cache: Read Miss++\n");
            }
            // Test if there are any data that needs to be written back to CM before block replacement
            if ((cm_mode.write_policy == WAWB) && valid_bit_test && (cm->dirty_bits[bf.cmbid] == 1)) {
                (cm_stats->NWA) += cm_mode.cache_block_size;  // Increment MM Write Access Count by the no. of words per block
                if (DEBUG_MESSAGES_ON) {
                    printf("simulate_cache: NWA++ (x Block Size)\n");
                }
            }
            cm->tag_bits[bf.cmbid] = bf.mmtb;             // Set CM block's tag bits to MMTB
            cm->valid_bits[bf.cmbid] = 1;                 // Set CM block's Valid bit to 1
            cm->dirty_bits[bf.cmbid] = 0;                 // Set CM block's Dirty bit to 0
            (cm_stats->NRA) += cm_mode.cache_block_size;  // Increment MM Read Access Count by the no. of words per block
        }
    }

    if (rw == 'W') {  // Write access
        if (valid_bit_test && tag_bit_test) {
            if (DEBUG_MESSAGES_ON) {
                printf("simulate_cache: Write Hit++\n");
            }
            (cm_stats->NCWH)++;  // Increment Write Hit Count
            if (cm_mode.write_policy == WAWT) {
                (cm_stats->NWA)++;  // Increment MM Write Access Count by 1 (for WT)
                if (DEBUG_MESSAGES_ON) {
                    printf("simulate_cache: NWA++\n");
                }
            }
        } else {
            (cm_stats->NCWM)++;  // Increment Write Miss Count
            if (DEBUG_MESSAGES_ON) {
                printf("simulate_cache: Write Miss++\n");
            }
            (cm_stats->NRA) += cm_mode.cache_block_size;  // Increment MM Read Access Count by the no. of words per block

            // Test the dirty bit and writing policy to see if the block should be written back to MM
            bool mem_write_required = ((cm_mode.write_policy == WAWB) && (cm->dirty_bits[bf.cmbid] == 1) && (cm->valid_bits[bf.cmbid] == 1));
            if (mem_write_required) {
                (cm_stats->NWA) += cm_mode.cache_block_size;  // Increment MM Write Access Count by the no. of words per block
                if (DEBUG_MESSAGES_ON) {
                    printf("simulate_cache: NWA++ (x Block Size)\n");
                }
            } else if ((cm_mode.write_policy == WAWT)) {
                (cm_stats->NWA)++;  // Increment MM Write Access Count by 1 (for WT)
                if (DEBUG_MESSAGES_ON) {
                    printf("simulate_cache: NWA++\n");
                }
            }

            cm->tag_bits[bf.cmbid] = bf.mmtb;  // Set CM block's tag bits to MMTB
            cm->valid_bits[bf.cmbid] = 1;      // Set CM block's Valid bit to 1
        }
        cm->dirty_bits[bf.cmbid] = 1;  // Set CM block's Dirty bit to 1
    }
}

/**
 * @brief Print simulation results for all modes of cache controller operation
 * @param stats Pointer to the stats array wrapper structure
 */
void print_stats(cache_mem_stats_arr_t *stats) {
    printf("\n\n \t----------------------\tSimulation results (statistics)\t---------------------- \n\n");
    for (int i = 0; i < 16; i++) {  // Print simulation results for each mode
        printf("ID: %-5u\tNCRH: %-5u\tNCRM: %-5u\tNCWH: %-5u\tNCWM: %-5u\tNRA: %-5u\tNWA: %-5u\n",
               stats->cm_stats[i].mode_ID,
               stats->cm_stats[i].NCRH,
               stats->cm_stats[i].NCRM,
               stats->cm_stats[i].NCWH,
               stats->cm_stats[i].NCWM,
               stats->cm_stats[i].NRA,
               stats->cm_stats[i].NWA);
    }
}

/**
 * @brief Output simulation results for all modes of cache controller operation to CSV file
 * @param stats Pointer to the stats array wrapper structure
 */
void output_stats(cache_mem_stats_arr_t *stats) {
    FILE *fp = fopen(output_file_name, "w+");  // Open the file in write mode (txt file) and return a pointer to it
    if (fp == NULL) {                          // Test if the file has been opened sucessfully
        printf("\noutput_stats: ERROR, NULL pointer returned when opening %s\n", output_file_name);
        exit(EXIT_FAILURE);
    } else {
        printf("\noutput_stats: File %s opened sucessfully\n", output_file_name);
    }

    if (OUTPUT_CSV_HEADER) {
        fprintf(fp, "trace_file_name, mode_ID, NRA, NWA, NCRH, NCRM, NCWH, NCWM\n");  // Print data header row into the csv file
    }

    for (int i = 0; i < 16; i++) {  // Print simulation results for each mode
        fprintf(fp, "%s, %u, %u, %u, %u, %u, %u, %u\n",
                input_file_name,
                stats->cm_stats[i].mode_ID,
                stats->cm_stats[i].NRA,
                stats->cm_stats[i].NWA,
                stats->cm_stats[i].NCRH,
                stats->cm_stats[i].NCRM,
                stats->cm_stats[i].NCWH,
                stats->cm_stats[i].NCWM);
    }

    if (fclose(fp) == EOF) {  // Close the file and test if successfully
        printf("output_stats: ERROR, EOF error returned when closing %s\n", output_file_name);
    } else {
        printf("output_stats: File %s closed sucessfully\n", output_file_name);
    }
}