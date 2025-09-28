/*
 * pfeiffer_protocol.h
 *
 *  Created on: Jul 17, 2025
 *      Author: johny
 */

#ifndef SRC_PFEIFFER_PROTOCOL_H_
#define SRC_PFEIFFER_PROTOCOL_H_


#include <stdio.h>   // For sprintf, fprintf, sscanf
#include <stdlib.h>  // For malloc, free, atoi, atof
#include <string.h>  // For strlen, strncpy, strdup
#include <stdbool.h> // For bool type
#include <stddef.h>  // For size_t
#include <math.h>

// --- Constants ---
#define CR_ASCII 13

// --- Structures ---
typedef struct {
    int slave_address;		//address of slave unit. eg."001" or "255"
    char slave_address_char[3];

    char action_char[3];		// 0 = R (Read), 1 = RW (Read-Write)

    int param_number;		//command being referenced. e.g. control command or data query command
    char param_number_char[3];

    int data_length;
    char data_length_char[2];

    char *data_value_str; 	// Raw string for data payload
    union {				//based on data_type, we store the data in one of variables in the union.
        int int_val;		// for U_INTEGER, U_SHORT_INT
        float float_val;	// for U_REAL, U_EXPO_NEW
        bool bool_val; 		// for BOOLEAN_OLD and BOOLEAN_new
        char str_val[17]; // For STRING_6, STRING_8, and STRING_16, plus null terminator
    } parsed_data;

    char *command_str;

    int checksum;
    char checksum_char[3];

    char cr;			//carriage return
    char *raw_telegram; // Original raw telegram string, dynamically allocated, maybe useful for debugging.
    bool checksum_ok; 	// Indicates if checksum passed during parsing.
    int error_code;
} Telegram;

// --- Function Prototypes ---

// Helper function for checksum calculation
int calculate_checksum(const char *data_segment);

// Builds a telegram string into the global variable defined in main.
void build_telegram(int slave_address, const char* action_char, int param_number, int data_length, const char *data_value_str);

/* Parses a raw telegram string into a Telegram structure.
   The caller is responsible for freeing telegram->raw_telegram and telegram->data_value_str.
 */
Telegram parse_telegram(const char *raw_telegram_str);

// To free dynamically allocated memory within Telegram struct
void free_telegram_memory(Telegram telegram);


#endif /* SRC_PFEIFFER_PROTOCOL_H_ */
