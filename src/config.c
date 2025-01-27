    // config.c
    #include "config.h"
    #include <strings.h>
    #include <string.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <ctype.h>
    #include <ncurses.h>

    // Helper function to trim leading whitespace
    static char* ltrim(char *s) {
        while(isspace((unsigned char)*s)) s++;
        return s;
    }

    // Helper function to trim trailing whitespace
    static void rtrim(char *s) {
        char *back = s + strlen(s);
        while(back > s && isspace((unsigned char)*(back-1))) back--;
        *back = '\0';
    }

    // Helper function to trim both leading and trailing whitespace
    static char* trim(char *s) {
        s = ltrim(s);
        rtrim(s);
        return s;
    }

    /**
     * Parses the key value from the configuration file.
     * Returns the corresponding key code or -1 on failure.
     */
    static int parse_key(const char *val);

    /**
     * Loads default keybindings into the provided KeyBindings structure.
     */
    void load_default_keybindings(KeyBindings *kb) {
        kb->key_up      = KEY_UP;
        kb->key_down    = KEY_DOWN;
        kb->key_left    = KEY_LEFT;
        kb->key_right   = KEY_RIGHT;
        kb->key_tab     = '\t';       // Tab
        kb->key_exit    = KEY_F(1);   // F1

        kb->key_edit    = 5;    // Ctrl+E (ASCII 5)
        kb->key_copy    = 3;    // Ctrl+C (ASCII 3)
        kb->key_paste   = 22;   // Ctrl+V (ASCII 22)
        kb->key_cut     = 24;   // Ctrl+X (ASCII 24)
        kb->key_delete  = 4;    // Ctrl+D (ASCII 4)
        kb->key_rename  = 18;   // Ctrl+R (ASCII 18)
        kb->key_new     = 14;   // Ctrl+N (ASCII 14)
        kb->key_save    = 19;   // Ctrl+S (ASCII 19)
        kb->key_new_dir = 14;
        // Dedicated editing keys
        kb->edit_up        = KEY_UP;
        kb->edit_down      = KEY_DOWN;
        kb->edit_left      = KEY_LEFT;
        kb->edit_right     = KEY_RIGHT;
        kb->edit_save      = 19;  // Ctrl+S
        kb->edit_quit      = 17;  // Ctrl+Q
        kb->edit_backspace = KEY_BACKSPACE; // ASCII Backspace

        // file 
        kb->info_label_width = 20; 
    }

    /**
     * Loads user configuration from a file, overriding defaults.
     * Returns the number of errors encountered. 0 means success.
     */
    int load_config_file(KeyBindings *kb, const char *filepath, char *error_buffer, size_t buffer_size) {
        FILE *fp = fopen(filepath, "r");
        if (!fp) {
            // No config file found; keep defaults
            snprintf(error_buffer, buffer_size, 
                    "Configuration file not found. Using default settings.\n");
            return 1; // Indicate one "non-fatal" error
        }

        char line[256];
        size_t error_count = 0;
        size_t line_number = 0;

        while (fgets(line, sizeof(line), fp)) {
            line_number++;

            // Remove trailing newline if present
            char *p = strchr(line, '\n');
            if (p) *p = '\0';

            // Remove inline comments by truncating at the first '#'
            p = strchr(line, '#');
            if (p) *p = '\0';

            // Trim the line
            char *trimmed_line = trim(line);

            // Skip empty lines
            if (strlen(trimmed_line) == 0)
                continue;

            // Must have key_name=VALUE
            char *eq = strchr(trimmed_line, '=');
            if (!eq) {
                // Malformed line
                if (error_count < buffer_size - 1) {
                    snprintf(error_buffer + strlen(error_buffer),
                            buffer_size - strlen(error_buffer) - 1,
                            "Line %zu: Malformed line (no '='): %s\n",
                            line_number, trimmed_line);
                }
                error_count++;
                continue;
            }

            *eq = '\0'; // split at '='
            char *name = trim(trimmed_line);
            char *val  = trim(eq + 1);

            // Skip if name or val are empty
            if (strlen(name) == 0 || strlen(val) == 0) {
                if (error_count < buffer_size - 1) {
                    snprintf(error_buffer + strlen(error_buffer),
                            buffer_size - strlen(error_buffer) - 1,
                            "Line %zu: Malformed line (empty key or value): %s=%s\n",
                            line_number, name, val);
                }
                error_count++;
                continue;
            }

            // Check for label_width config first (which requires integer parsing)
            if (strcasecmp(name, "info_label_width") == 0 ||
                strcasecmp(name, "label_width") == 0)
            {
                char *endptr;
                int user_val = strtol(val, &endptr, 10);
                if (*endptr != '\0') {
                    // Not a clean integer
                    if (error_count < buffer_size - 1) {
                        snprintf(error_buffer + strlen(error_buffer),
                                buffer_size - strlen(error_buffer) - 1,
                                "Line %zu: Invalid label width: %s\n",
                                line_number, val);
                    }
                    error_count++;
                } else {
                    kb->info_label_width = user_val;
                }
                continue; // we handled this line
            }

            // Else parse as a "key" using parse_key()
            int parsed = parse_key(val);
            if (parsed == -1) {
                // Invalid key value
                if (error_count < buffer_size - 1) {
                    snprintf(error_buffer + strlen(error_buffer),
                            buffer_size - strlen(error_buffer) - 1,
                            "Line %zu: Invalid key value for '%s': %s\n",
                            line_number, name, val);
                }
                error_count++;
                continue;
            }

            // Map the parsed key to the struct field
            if      (strcasecmp(name, "key_up")     == 0) kb->key_up      = parsed;
            else if (strcasecmp(name, "key_down")   == 0) kb->key_down    = parsed;
            else if (strcasecmp(name, "key_left")   == 0) kb->key_left    = parsed;
            else if (strcasecmp(name, "key_right")  == 0) kb->key_right   = parsed;
            else if (strcasecmp(name, "key_tab")    == 0) kb->key_tab     = parsed;
            else if (strcasecmp(name, "key_exit")   == 0) kb->key_exit    = parsed;
            else if (strcasecmp(name, "key_edit")   == 0) kb->key_edit    = parsed;
            else if (strcasecmp(name, "key_copy")   == 0) kb->key_copy    = parsed;
            else if (strcasecmp(name, "key_paste")  == 0) kb->key_paste   = parsed;
            else if (strcasecmp(name, "key_cut")    == 0) kb->key_cut     = parsed;
            else if (strcasecmp(name, "key_delete") == 0) kb->key_delete  = parsed;
            else if (strcasecmp(name, "key_rename") == 0) kb->key_rename  = parsed;
            else if (strcasecmp(name, "key_new")    == 0) kb->key_new     = parsed;
            else if (strcasecmp(name, "key_save")   == 0) kb->key_save    = parsed;
            else if (strcasecmp(name, "key_new_dir")== 0) kb->key_new_dir = parsed;
            // Editing mode keys
            else if (strcasecmp(name, "edit_up")        == 0) kb->edit_up        = parsed;
            else if (strcasecmp(name, "edit_down")      == 0) kb->edit_down      = parsed;
            else if (strcasecmp(name, "edit_left")      == 0) kb->edit_left      = parsed;
            else if (strcasecmp(name, "edit_right")     == 0) kb->edit_right     = parsed;
            else if (strcasecmp(name, "edit_save")      == 0) kb->edit_save      = parsed;
            else if (strcasecmp(name, "edit_quit")      == 0) kb->edit_quit      = parsed;
            else if (strcasecmp(name, "edit_backspace") == 0) kb->edit_backspace = parsed;
            else {
                // Unknown config key
                if (error_count < buffer_size - 1) {
                    snprintf(error_buffer + strlen(error_buffer),
                            buffer_size - strlen(error_buffer) - 1,
                            "Line %zu: Unknown configuration key: %s\n",
                            line_number, name);
                }
                error_count++;
            }
        }

        fclose(fp);
        return error_count;
    }


    /**
     * Utility function to parse textual representations of keys:
     *   parse_key("KEY_UP") -> KEY_UP
     *   parse_key("^C")     -> 3
     *   parse_key("F1")     -> KEY_F(1)
     *   parse_key("Tab")    -> '\t'
     *   parse_key("x")      -> 'x'
     *
     * Returns -1 on failure.
     */
    #include <ctype.h>
    #include <string.h>
    #include <ncurses.h>
    #include <stdio.h>
    #include <stdlib.h>

    /**
     * Utility function to parse textual representations of keys:
     *   parse_key("KEY_UP") -> KEY_UP
     *   parse_key("^C")     -> 3
     *   parse_key("F1")     -> KEY_F(1)
     *   parse_key("Tab")    -> '\t'
     *   parse_key("x")      -> 'x'
     *   parse_key("Shift+A") -> 'A'
     *
     * Returns -1 on failure.
     */
    static int parse_key(const char *val) {
        // Check for special ncurses keys
        if (strcasecmp(val, "KEY_UP") == 0)        return KEY_UP;
        if (strcasecmp(val, "KEY_DOWN") == 0)      return KEY_DOWN;
        if (strcasecmp(val, "KEY_LEFT") == 0)      return KEY_LEFT;
        if (strcasecmp(val, "KEY_RIGHT") == 0)     return KEY_RIGHT;

        if (strncasecmp(val, "KEY_F(", 6) == 0) {
            // e.g., KEY_F(1), KEY_F(2), extract number inside KEY_F(n)
            int fnum = atoi(val + 6);
            if (fnum >= 1 && fnum <= 63) { // ncurses typically supports F1-F63
                return KEY_F(fnum);
            }
            return -1;
        }

        if (strcasecmp(val, "KEY_BACKSPACE") == 0 ||
            strcasecmp(val, "Backspace") == 0)
        {
            return KEY_BACKSPACE;
        }

        // Support for F1 - F12
        for (int i = 1; i <= 12; i++) {
            char fn_key[5];
            snprintf(fn_key, sizeof(fn_key), "F%d", i);
            if (strcasecmp(val, fn_key) == 0) {
                return KEY_F(i);
            }
        }

        // Handle Ctrl+X keys: "^C" -> ASCII 3
        if (val[0] == '^' && val[1] != '\0' && val[2] == '\0') {
            char c = toupper(val[1]);
            if (c >= 'A' && c <= 'Z') {
                return c - 'A' + 1;  // ASCII control code
            }
            return -1; // Invalid
        }

        // Support "Tab" and "Space"
        if (strcasecmp(val, "Tab") == 0) return '\t';
        if (strcasecmp(val, "Space") == 0) return ' ';

        // Handle Shift-modified keys (e.g., "Shift+A" -> 'A')
        if (strncasecmp(val, "Shift+", 6) == 0) {
            char shift_key = val[6];

            // Convert letter keys to uppercase
            if (isalpha(shift_key)) {
                return toupper(shift_key);
            }

            // Handle Shift-modified number keys
            static const char shift_symbols[] = ")!@#$%^&*(";
            if (isdigit(shift_key)) {
                int num = shift_key - '0';
                return shift_symbols[num]; // Returns corresponding symbol
            }

            // Handle common Shift-symbol keys
            if (strcmp(val + 6, "Minus") == 0) return '_';
            if (strcmp(val + 6, "Equals") == 0) return '+';
            if (strcmp(val + 6, "LeftBracket") == 0) return '{';
            if (strcmp(val + 6, "RightBracket") == 0) return '}';
            if (strcmp(val + 6, "Semicolon") == 0) return ':';
            if (strcmp(val + 6, "Apostrophe") == 0) return '"';
            if (strcmp(val + 6, "Comma") == 0) return '<';
            if (strcmp(val + 6, "Period") == 0) return '>';
            if (strcmp(val + 6, "Slash") == 0) return '?';
            if (strcmp(val + 6, "Backslash") == 0) return '|';
            if (strcmp(val + 6, "Grave") == 0) return '~';

            return -1; // Invalid Shift-modified key
        }

        // Single character key (non-shifted)
        if (strlen(val) == 1) {
            return (unsigned char)val[0];
        }

        return -1; // Unknown key
    }

