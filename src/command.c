#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/unique_id.h"

#include "command.h"
#include "hardware.h"
#include "version.h"

// Incoming command buffer
#define CMD_BUF_LEN 65536
char cmd_buf[CMD_BUF_LEN]; 

// Buffer for storing board id
#define BID_BUF_LEN 2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1
char bid_buf[BID_BUF_LEN]; 

bool cmd_ready = false;           // Indicate whether command ready to be processed
bool rx_available = false;        // Indicate whether a character is avaialble on stdin

// This function is set as the callback when chars are available on stdin
void rx_handler(void* ptr) {
	rx_available = true;
}

// This function is called in the main loop every time there's a character available
void cmd_read_char() {
	static uint32_t rx_counter = 0;  // Keep track of cursor position in receive buffer
	static int rx_tmp;               // Temporary buffer for received character

    // Read in single char
    rx_tmp = stdio_getchar_timeout_us(0);

	// If the new character is received successfully, process it
	if (rx_tmp != -2) {
    	// If character is LF or CR or we ran out of space, terminate string and hand off
    	// into another buffer for further processing. The non-zero length check prevents
    	// extra line terminations, so CRLF doesn't result in a new zero-length string
    	if (rx_counter != 0 && (rx_tmp == 10 || rx_tmp == 13 || rx_counter == CMD_BUF_LEN - 1)) {
			// This throws away the last character if the buffer is filled up,
			// but overrunning the buffer would truncate the command regardless,
			// so it doesn't matter if it happens one character sooner.
			cmd_buf[rx_counter] = '\0';

			//printf("Command read-in successful: %s.\n", cmd_buf);
			// Indicate that command is ready for processing
    		cmd_ready = true;
			// Reset counter
			rx_counter = 0;
    	}
    	// If the received character is printable, convert it to uppercase and append to command buffer
    	else if (isprint(rx_tmp)) {
        	cmd_buf[rx_counter++] = toupper((char)rx_tmp);
			//printf("Read in character %c.\n", (char)rx_tmp);
    	}
	}
	else {
		// If we couldn't get a character, indicate that the buffer is empty
		rx_available = false;
	}
}

// Decode command buffer. Called when the cmd_ready flag is set.
void cmd_decode() {
	// This part isn't performance-critical, so we just use local variables
	char* cmd_word;
	char* next_token;
	
	printf("Begin decoding:\r\n");
	
	// Read in first token and store progress in next_token
	cmd_word = strtok_r(cmd_buf, " ", &next_token);
	printf("\tCommand is: %s\r\n", cmd_word);

	if (!strcmp(cmd_word, "*IDN?") || !strcmp(cmd_word, "IDN?")) {
		print_id();
	}

}

void print_id() {
	static bool firstrun = true;

	if (firstrun) {
		// Update ID string on first run
		pico_get_unique_board_id_string(bid_buf, BID_BUF_LEN);
		firstrun = false;
	}

	printf(
		"pico-pulse v%d.%d, board id: %s\r\n",
		VERSION_MAJOR,
		VERSION_MINOR,
		bid_buf
	);
}
