#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "chat.h"
#include "../util.h"
#include "../state.h"
#include "../config.h"
#include "../gfx/window.h"
#include "../world/save.h"
#include "../world/tick.h"

#include "../global.h"

chat_message_t chat_messages[MAX_CHAT_MESSAGES] = {0};
size_t chat_count = 0;
size_t chat_head = 0;

char chat_input[CHAT_INPUT_MAX];
size_t chat_input_len = 0;
bool chat_active = false;

block_command_t block_commands[] = {
    {"!air", AIR, 5},
    {"!stud", STUD, 6},
    {"!bk", BRICK, 4},
    {"!and", AND, 5},
    {"!or", OR, 4},
    {"!xor", XOR, 5},
    {"!nand", NAND, 6},
    {"!nor", NOR, 5},
    {"!xnor", XNOR, 6},
    {"!ff", FLIPFLOP, 4},
    {"!node", NODE, 6}
};

#define BLOCK_COMMANDS_COUNT (sizeof(block_commands)/sizeof(block_commands[0]))

void chat_render(void) {
    for (size_t i = 0; i < chat_count; i++) {
        size_t start = (chat_head + MAX_CHAT_MESSAGES - chat_count) % MAX_CHAT_MESSAGES;
        size_t index = (start + i) % MAX_CHAT_MESSAGES;

        const chat_message_t *message = &chat_messages[index]; 

        if (message->formatted == NULL) continue;

        int y = window.height - 
            CHAT_MESSAGE_FONTSIZE * MAX_CHAT_MESSAGES +
            i * CHAT_MESSAGE_FONTSIZE - CHAT_MESSAGE_FONTSIZE;

        renderer_text(0, y, CHAT_MESSAGE_SCALE, message->formatted, (vec3){1, 0.5, 1});
    }
}

static void handle_building_command(char *text) {
    if (!strncmp(text, "!b ", 3)) { // select building
        building_t building = {0};
        strtok(text, " ");

        building.id = name_building_id(strtok(NULL, " "));
        switch (building.id) {
            case MEMORY: building.state.memory.address_width = strtoul(strtok(NULL, " "), NULL, 10); break;
            default: chat_add_message("comm", "unknown building name"); return;
        }

        building.bit_width = strtoul(strtok(NULL, " "), NULL, 10);

        state.player.selected_building = building;
    }
    if (!strncmp(text, "!lm ", 4)) { // load memory
    	// TODO: if bitwidth is not 8, add a parameter to pack bytes
    	size_t size;
   		void *bin = readbin(text + 4, &size);
   		if (!bin) {
   			chat_add_message("comm", "failed to open the file");
   			return;
   		}
   		
		if (state.player.hovered_block && state.player.hovered_block->building &&
			state.player.hovered_block->building->id == MEMORY
		) {
			building_t *building = state.player.hovered_block->building;
			void *cells = building->state.memory.cells;
			size_t memory_size = building->state.memory.size;
			
			switch (building->bit_width) {
				case 8:
					for (size_t i = 0; i < size && i < memory_size; i++) {
						((uint8_t *)cells)[i] = ((uint8_t *)bin)[i];
					}
					break;
				case 16:
					for (size_t i = 0; i < size && i < memory_size; i++) {
						((uint16_t *)cells)[i] = ((uint8_t *)bin)[i];
					}
					break;
				case 32:
					for (size_t i = 0; i < size && i < memory_size; i++) {
						((uint32_t *)cells)[i] = ((uint8_t *)bin)[i];
					}				
					break;
				case 64:
					for (size_t i = 0; i < size && i < memory_size; i++) {
						((uint64_t *)cells)[i] = ((uint8_t *)bin)[i];
					}				
					break;					
				default:
					chat_add_message("comm", "memory bitwidth not supported");
					return;					
			}

			free(bin);
		} else
			chat_add_message("comm", "opened the file but cant load ingame, please select a memory building");
    }
}

void chat_handle_command(char *text) {
    char buf[256];

    handle_building_command(text);
    if (!strncmp(text, "!tps ", 5)) {
        int target = atoi(text + 5);

        if (target < 0) {
            chat_add_message("comm", "invalid tps");
            return;
        }

        tick_interval = 1.0 / target;

        snprintf(buf, sizeof(buf), "tps updated to interval %f", tick_interval);

        chat_add_message("comm", buf);
    }
    else if (!strncmp(text, "!save ", 6)) {
        const char *save_name = text + 6;

        save_save(save_name);

        snprintf(buf, sizeof(buf), "saved to %s", save_name);
        chat_add_message("comm", buf);
    }
    else if (!strncmp(text, "!setting ", 9)) {
        strtok(text, " ");
        char *key = strtok(NULL, " "),
             *value = strtok(NULL, " ");
        config_modify(key, value);
        snprintf(buf, sizeof(buf), "key %s changed to %s", key, value);
        chat_add_message("comm", buf);
    }    
    else if (!strncmp(text, "!system ", 8)) {
        FILE *fp = popen(text + 8, "r");
        if (!fp)
            return;

        while (fgets(buf, sizeof(buf), fp) != NULL) {
            chat_add_message("comm", buf); // print each line
        }

        pclose(fp);
    }
    else if (!strcmp(text, "!restart")) {
        state.restart = true;
    }  
    else if (!strcmp(text, "!wp")) {
        state.player.mode = MODE_WIRE_PLACE;
    }
    else if (!strcmp(text, "!wd")) {
        state.player.mode = MODE_WIRE_DESTROY;
    }
    for (int i = 0; i < BLOCK_COMMANDS_COUNT; i++) {
        block_command_t *bk_command = &block_commands[i];
        if (strncmp(text, bk_command->command, bk_command->len)) continue;
        state.player.selected_block = bk_command->gate;
        state.player.mode = MODE_BLOCK_PLACE;
    }

    return;
}

void chat_add_message(const char *name, const char *text) {
    chat_message_t *message = &chat_messages[chat_head];
    
    if (message->formatted) free(message->formatted);
    if (message->name) free((void*)message->name);
    if (message->message) free((void*)message->message);

    message->name = strdup(name);
    message->message = strdup(text);

    size_t buffer_size = snprintf(NULL, 0, "[%s]: %s", name, text) + 1;
    char *buffer = smalloc(buffer_size);
    snprintf(buffer, buffer_size, "[%s]: %s", name, text);

    message->formatted = buffer;

    chat_head = (chat_head + 1) % MAX_CHAT_MESSAGES;
    if (chat_count < MAX_CHAT_MESSAGES) {
        chat_count++;
    }
}

void chat_char_callback(unsigned int codepoint) {
    if (!chat_active) return;

    if (chat_input_len < CHAT_INPUT_MAX - 1) {
        chat_input[chat_input_len++] = (char)codepoint;
        chat_input[chat_input_len] = '\0';
    }    
}

void chat_key_callback(void) {
	if (!chat_active) return;
	
	if (window.keyboard.keys[GLFW_KEY_V].down && window.keyboard.keys[GLFW_KEY_V].mods & GLFW_MOD_CONTROL) {
		const char *text = glfwGetClipboardString(window.handle);
		if (!text) 
		    goto done;

		while (*text != '\0')
		    chat_input[chat_input_len++] = *text++;
		chat_input[chat_input_len] = '\0';
	}
done:
	window.keyboard.keys[GLFW_KEY_V].down = false;
}
