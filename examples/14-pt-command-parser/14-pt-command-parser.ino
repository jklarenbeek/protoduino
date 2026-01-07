/**
 * EXAMPLE 1: Serial Command Parser with Interactive Menu System
 *
 * This example demonstrates a robust command-line interface using protothreads.
 * It features:
 * - Non-blocking command parsing with line buffering
 * - Interactive menu system with multiple command handlers
 * - Error handling and user feedback
 * - Efficient use of PT_WAIT_UNTIL for serial availability
 *
 * Commands supported:
 * - help: Display available commands
 * - led <on|off>: Control LED state
 * - info: Display system information
 * - echo <text>: Echo back user input
 * - reset: Reset the command system
 */

#include <protoduino.h>
#include <sys/mem.h>
#include <sys/serial/SerialClass.hpp>
#include <string.h>

#define CMD_BUFFER_SIZE 64
#define LED_PIN 13

// Command buffer structure
struct cmd_buffer_t {
  char buffer[CMD_BUFFER_SIZE];
  uint8_t index;
  bool complete;
};

// Command parser protothread state
struct cmd_parser_pt {
  lc_t lc;
  cmd_buffer_t cmd_buf;
  int_fast16_t byte_read;
};

// LED control protothread state
struct led_control_pt {
  lc_t lc;
  bool led_state;
  uint32_t blink_timer;
};

static struct cmd_parser_pt cmd_parser;
static struct led_control_pt led_control;
static uint32_t uptime_seconds = 0;

// Forward declarations
void process_command(const char* cmd);
void print_prompt(void);
void print_help(void);

/**
 * Command parser protothread
 * Reads serial input character by character and builds complete command lines
 */
static ptstate_t parse_commands(struct cmd_parser_pt* self)
{
  PT_BEGIN(self);

  // Initialize command buffer
  self->cmd_buf.index = 0;
  self->cmd_buf.complete = false;

  SerialLine.println(F("================================="));
  SerialLine.println(F("Serial Command Parser v1.0"));
  SerialLine.println(F("Type 'help' for available commands"));
  SerialLine.println(F("================================="));
  print_prompt();

  forever: while(1)
  {
    // Wait for serial data to be available
    PT_WAIT_UNTIL(self, serial0_read_available() > 0);

    // Read one byte
    self->byte_read = serial0_read8();

    if (self->byte_read == -1) {
      PT_WAIT_ONE(self);
      continue;
    }

    char ch = (char)self->byte_read;

    // Echo character back (for user feedback)
    if (ch >= 32 && ch < 127) {
      serial0_write8(ch);
    }

    // Handle special characters
    if (ch == '\r' || ch == '\n') {
      if (self->cmd_buf.index > 0) {
        serial0_write8('\r');
        serial0_write8('\n');

        // Null terminate the command
        self->cmd_buf.buffer[self->cmd_buf.index] = '\0';

        // Process the command
        process_command(self->cmd_buf.buffer);

        // Reset buffer
        self->cmd_buf.index = 0;

        // Print new prompt
        print_prompt();
      } else {
        // Empty command, just print new prompt
        serial0_write8('\r');
        serial0_write8('\n');
        print_prompt();
      }

      PT_WAIT_ONE(self);
      continue;
    }

    // Handle backspace
    if (ch == '\b' || ch == 127) {
      if (self->cmd_buf.index > 0) {
        self->cmd_buf.index--;
        // Send backspace sequence
        serial0_write8('\b');
        serial0_write8(' ');
        serial0_write8('\b');
      }
      PT_WAIT_ONE(self);
      continue;
    }

    // Add character to buffer if there's space
    if (self->cmd_buf.index < CMD_BUFFER_SIZE - 1 && ch >= 32 && ch < 127) {
      self->cmd_buf.buffer[self->cmd_buf.index++] = ch;
    }

    PT_WAIT_ONE(self);
  }

  PT_END(self);
}

/**
 * LED control protothread
 * Handles LED blinking when enabled
 */
static ptstate_t control_led(struct led_control_pt* self)
{
  PT_BEGIN(self);

  pinMode(LED_PIN, OUTPUT);
  self->led_state = false;
  self->blink_timer = 0;

  forever: while(1)
  {
    if (self->led_state) {
      // Blink LED when on
      digitalWrite(LED_PIN, HIGH);
      PT_WAIT_DELAY(self, 500);

      digitalWrite(LED_PIN, LOW);
      PT_WAIT_DELAY(self, 500);
    } else {
      // Keep LED off
      digitalWrite(LED_PIN, LOW);
      PT_WAIT_DELAY(self, 100);
    }
  }

  PT_END(self);
}

/**
 * Uptime counter protothread
 * Increments uptime counter every second
 */
static ptstate_t update_uptime(struct pt* self)
{
  PT_BEGIN(self);

  forever: while(1)
  {
    PT_WAIT_DELAY(self, 1000);
    uptime_seconds++;
  }

  PT_END(self);
}

/**
 * Print command prompt
 */
void print_prompt(void)
{
  SerialLine.print(F("> "));
  SerialLine.flush();
}

/**
 * Print help text
 */
void print_help(void)
{
  SerialLine.println(F("\nAvailable commands:"));
  SerialLine.println(F("  help         - Display this help message"));
  SerialLine.println(F("  led on       - Turn on LED (blinking)"));
  SerialLine.println(F("  led off      - Turn off LED"));
  SerialLine.println(F("  info         - Display system information"));
  SerialLine.println(F("  echo <text>  - Echo back the text"));
  SerialLine.println(F("  reset        - Reset the command parser"));
}

/**
 * Process parsed command
 */
void process_command(const char* cmd)
{
  // Remove leading/trailing whitespace
  while (*cmd == ' ') cmd++;

  if (strlen(cmd) == 0) {
    return;
  }

  // Parse command
  if (strcmp(cmd, "help") == 0) {
    print_help();
  }
  else if (strncmp(cmd, "led ", 4) == 0) {
    const char* arg = cmd + 4;
    if (strcmp(arg, "on") == 0) {
      led_control.led_state = true;
      SerialLine.println(F("LED turned ON (blinking)"));
    }
    else if (strcmp(arg, "off") == 0) {
      led_control.led_state = false;
      SerialLine.println(F("LED turned OFF"));
    }
    else {
      SerialLine.println(F("Error: Invalid argument. Use 'led on' or 'led off'"));
    }
  }
  else if (strcmp(cmd, "info") == 0) {
    SerialLine.println(F("\n--- System Information ---"));
    SerialLine.print(F("Uptime: "));
    SerialLine.print(uptime_seconds);
    SerialLine.println(F(" seconds"));
    SerialLine.print(F("LED State: "));
    SerialLine.println(led_control.led_state ? F("ON") : F("OFF"));
    SerialLine.print(F("Free RAM: "));
    SerialLine.print(free_memory());
    SerialLine.println(F(" bytes"));
  }
  else if (strncmp(cmd, "echo ", 5) == 0) {
    const char* text = cmd + 5;
    SerialLine.print(F("Echo: "));
    SerialLine.println(text);
  }
  else if (strcmp(cmd, "reset") == 0) {
    SerialLine.println(F("Resetting command parser..."));
    PT_INIT(&cmd_parser);
  }
  else {
    SerialLine.print(F("Unknown command: "));
    SerialLine.println(cmd);
    SerialLine.println(F("Type 'help' for available commands"));
  }
}


void setup()
{
  serial0_open(9600);

  PT_INIT(&cmd_parser);
  PT_INIT(&led_control);
}

void loop()
{
  static struct pt uptime_pt;
  static bool uptime_init = false;

  if (!uptime_init) {
    PT_INIT(&uptime_pt);
    uptime_init = true;
  }

  // Run all protothreads
  parse_commands(&cmd_parser);
  control_led(&led_control);
  update_uptime(&uptime_pt);
}