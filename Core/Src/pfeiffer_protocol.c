/*
 * pfeiffer_protocol.c
 *
 *  Created on: Jul 17, 2025
 *      Author: johny
 */

#include "pfeiffer_protocol.h"
#include <stdio.h>   // For sprintf, fprintf, sscanf
#include <stdlib.h>  // For malloc, free, atoi, atof
#include <string.h>  // For strlen, strncpy, strdup

// --- Helper Functions ---

int calculate_checksum(const char *data_segment) {
    int sum_ascii = 0;
    for (int i = 0; i < strlen(data_segment); i++) {
        sum_ascii += (int)data_segment[i];
    }
    return sum_ascii % 256;
}


//NOTE: If memory is not allocated properly using malloc, then the code will run into a HardFault Interrupt.
//
Telegram parse_telegram(const char *raw_telegram_str) {
	Telegram error;
	char temp[32];

	// Copy the string into the buffer
	strcpy(temp, raw_telegram_str);
    size_t len = strlen(raw_telegram_str); //

    Telegram telegram;
    telegram.error_code = 0;

    //-------Received Checksum------------
    char received_checksum_str[4];
    strncpy(received_checksum_str, &temp[len - 4], 3);		//stores the checksum.
    received_checksum_str[3] = '\0';						//for atoi()
    telegram.checksum = atoi(received_checksum_str);
    telegram.cr = temp[len - 1];							//stores Carriage return


    //create memory for data str
    telegram.command_str = (char *)malloc(telegram.data_length + 1);
    //-------Calculate checksum------------
    char *data_for_checksum_calc = (char *)malloc(len - 4); 		//add 1 for the null terminator.
    if (data_for_checksum_calc == NULL) {
        error.error_code = 1;
        strcpy(telegram.command_str, "000001");
        return error;

    }
    strncpy(data_for_checksum_calc, temp, len - 4); 		//copy ascii chars till the checksum field of the telegram.i.e. index 0 to index len-5
    data_for_checksum_calc[len - 4] = '\0';
    int calculated_checksum = calculate_checksum(data_for_checksum_calc);



    //-------Compare checksum-------------
    if (telegram.checksum != calculated_checksum) {
        telegram.checksum_ok = false;
        free(data_for_checksum_calc);
        error.error_code = 2;
        strcpy(telegram.command_str, "000002");
        return error;

    }
    telegram.checksum_ok = true;



    //-------Separate fields--------------
    char temp_buffer[20] = {0};


    strncpy(temp_buffer, data_for_checksum_calc, 3); temp_buffer[3] = '\0'; 	//copies first 3 chars as slave addr to temp buffer.
    telegram.slave_address = atoi(temp_buffer);
    sscanf(data_for_checksum_calc + 8, "%2d", &telegram.data_length);        //obtain length of data by adding an offset of 8.
    strncpy(telegram.action_char, "10\0", 3);//stores the action char
    strncpy(temp_buffer, data_for_checksum_calc + 5, 3); temp_buffer[3] = '\0';	//copies the 3 chars of the param_number to temp_buffer by adding an offset of 5.
    telegram.param_number = atoi(temp_buffer);


    //-------Retrieve and Store Data---------
    int data_start_index = 10; //the index where the actual data starts always.

    //checks if out of bounds
	if (data_start_index + (telegram.data_length) > strlen(data_for_checksum_calc)) {
		free(data_for_checksum_calc);
		error.error_code = 3;
		strcpy(telegram.command_str, "000003");
		return error;

	}
    strncpy(telegram.command_str, data_for_checksum_calc + data_start_index, telegram.data_length);	//copy data chars to data str
    telegram.command_str[telegram.data_length] = '\0';


	if (telegram.command_str == NULL) {
		free(data_for_checksum_calc);
		error.error_code = 4;
		strcpy(telegram.command_str, "000004");
		return error;
	}


    free(data_for_checksum_calc);
    //remember to free telegram memory.
    return telegram;
}


void free_telegram_memory(Telegram telegram) {
    	free(telegram.command_str);
}


extern char telegram[32];
extern char received_telegram[32];
void build_telegram(int slave_address, const char* action_char, int param_number, int data_length, const char *data_value_str) {
    char addr_str[4];
    char param_str[4];
    char length_str[3];
    char checksum_str[4];



    sprintf(addr_str, "%03d", slave_address);
    sprintf(param_str, "%03d", param_number);
    sprintf(length_str, "%02d", data_length);

    //telegram_len = addr + action + zero + param + length + data + checksum + carriage return
    size_t telegram_len = 3 + 1 + 1 + 3 + 2 + data_length + 3 + 1;

    char telegram_buffer[32] = {0};

    //creates a buffer for the checksum string. -3 for ignoring the checksum field length and -1 for the carriage return field
    char *checksum_data_part_buffer = (char *)malloc(telegram_len - 3 - 1);
    sprintf(checksum_data_part_buffer, "%s%s%s%s%s",
                    addr_str, action_char, param_str, length_str, data_value_str);

    int checksum_val = calculate_checksum(checksum_data_part_buffer);
    sprintf(checksum_str, "%03d", checksum_val);

    sprintf(telegram_buffer, "%s%s%c",
    		checksum_data_part_buffer,
            checksum_str,
            (char)CR_ASCII);

    free(checksum_data_part_buffer);
    strcpy(telegram,telegram_buffer);
    strcpy(received_telegram,telegram_buffer);
}




