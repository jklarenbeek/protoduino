# Protoduino Module Compilation Guide

## Overview

This guide explains how to compile standalone modules that can be loaded into Protoduino at runtime.

## Module Structure

A Protoduino module must follow this structure:

```cpp
// module_example.cpp
#include <protoduino.h>

// Module entry point - MUST be named "module_main"
extern "C" ptstate_t module_main(struct pt *pt)
{
    PT_BEGIN(pt);

    // Your module code here

    PT_END(pt);
}
```

## Compilation Process

### Step 1: Compile to Object File

Use avr-gcc to compile your module:

```bash
avr-gcc -c \
  -Os \
  -mmcu=atmega328p \
  -DF_CPU=16000000UL \
  -I/path/to/protoduino/src \
  -ffunction-sections \
  -fdata-sections \
  -fpic \
  -o module_example.o \
  module_example.cpp
```

### Step 2: Link as Relocatable

Link as position-independent code:

```bash
avr-ld \
  -r \
  --emit-relocs \
  -o module_example.elf \
  module_example.o
```

### Step 3: Extract Binary Segments

Extract code and data segments:

```bash
avr-objcopy -O binary \
  --only-section=.text \
  module_example.elf \
  module_example.text

avr-objcopy -O binary \
  --only-section=.data \
  module_example.elf \
  module_example.data
```

### Step 4: Create Module Package

Create a Python script to package the module:

```python
#!/usr/bin/env python3
# module_packager.py

import struct
import sys

def package_module(text_file, data_file, output_file):
    # Read segments
    with open(text_file, 'rb') as f:
        text = f.read()

    with open(data_file, 'rb') as f:
        data = f.read()

    # Create header
    header = struct.pack('<HH', len(text), len(data))

    # Write package
    with open(output_file, 'wb') as f:
        f.write(header)
        f.write(text)
        f.write(data)

    print(f"Module packaged: {output_file}")
    print(f"  Code size: {len(text)} bytes")
    print(f"  Data size: {len(data)} bytes")
    print(f"  Total: {len(header) + len(text) + len(data)} bytes")

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print("Usage: module_packager.py <text> <data> <output>")
        sys.exit(1)

    package_module(sys.argv[1], sys.argv[2], sys.argv[3])
```

### Step 5: Upload Module

Use Python to send the module via serial:

```python
#!/usr/bin/env python3
# module_uploader.py

import serial
import time
import sys

def upload_module(port, baudrate, module_name, module_file):
    ser = serial.Serial(port, baudrate, timeout=1)
    time.sleep(2)  # Wait for Arduino reset

    # Send load command
    cmd = f"load {module_name}\n"
    ser.write(cmd.encode())

    # Wait for prompt
    time.sleep(1)

    # Send module data
    with open(module_file, 'rb') as f:
        data = f.read()
        ser.write(data)

    # Wait for response
    time.sleep(1)
    response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
    print(response)

    ser.close()

if __name__ == '__main__':
    if len(sys.argv) != 5:
        print("Usage: module_uploader.py <port> <baud> <name> <file>")
        sys.exit(1)

    upload_module(sys.argv[1], int(sys.argv[2]), sys.argv[3], sys.argv[4])
```

## Example Usage

### 1. Write Module

```cpp
// blink_fast.cpp
#include <protoduino.h>

// External symbols (provided by main program)
extern "C" {
    void pinMode(uint8_t pin, uint8_t mode);
    void digitalWrite(uint8_t pin, uint8_t value);
    uint8_t digitalRead(uint8_t pin);
    unsigned long millis(void);
}

#define LED_PIN 13
#define BLINK_INTERVAL 100

extern "C" ptstate_t module_main(struct pt *pt)
{
    static unsigned long last_toggle = 0;
    static uint8_t state = 0;

    PT_BEGIN(pt);

    pinMode(LED_PIN, 1); // OUTPUT = 1

    while(1) {
        PT_WAIT_UNTIL(pt, (millis() - last_toggle) > BLINK_INTERVAL);

        state = !state;
        digitalWrite(LED_PIN, state);
        last_toggle = millis();
    }

    PT_END(pt);
}
```

### 2. Compile

```bash
./compile_module.sh blink_fast.cpp
```

### 3. Upload

```bash
python3 module_uploader.py /dev/ttyUSB0 9600 blink blink_fast.mod
```

### 4. Control via CLI

```
> load blink
> start blink
> list
> stop blink
> unload blink
```

## Makefile Example

```makefile
# Makefile for Protoduino modules

MCU = atmega328p
F_CPU = 16000000UL
INCLUDES = -I../../src

%.o: %.cpp
	avr-gcc -c -Os -mmcu=$(MCU) -DF_CPU=$(F_CPU) \
	  $(INCLUDES) -ffunction-sections -fdata-sections -fpic \
	  -o $@ $<

%.elf: %.o
	avr-ld -r --emit-relocs -o $@ $<

%.mod: %.elf
	avr-objcopy -O binary --only-section=.text $< $@.text
	avr-objcopy -O binary --only-section=.data $< $@.data
	python3 module_packager.py $@.text $@.data $@
	rm -f $@.text $@.data

clean:
	rm -f *.o *.elf *.mod *.text *.data

.PHONY: clean
```

## Limitations

1. **Memory**: Limited by available RAM on Arduino
2. **No Dynamic Linking**: External symbols must be registered before loading
3. **No C++ Features**: Limited C++ support (no exceptions, RTTI)
4. **Flash Storage**: Modules are stored in RAM, not flash
5. **Size**: Keep modules small (< 1KB recommended)

## Advanced Features

### Persistent Modules (EEPROM)

Store modules in EEPROM for persistence across reboots:

```cpp
#include <EEPROM.h>

int loader_save_to_eeprom(const char *name, uint16_t addr) {
    struct loader_module *mod = loader_get_module(name);
    if (!mod) return LOADER_ERR_NOTFOUND;

    // Write header
    EEPROM.put(addr, mod->code_size);
    EEPROM.put(addr + 2, mod->data_size);

    // Write code
    for (uint16_t i = 0; i < mod->code_size; i++) {
        EEPROM.write(addr + 4 + i, ((uint8_t *)mod->code)[i]);
    }

    return LOADER_OK;
}
```

### Module Signing

Add cryptographic signatures to verify module integrity:

```cpp
#define MODULE_SIGNATURE_SIZE 32

struct module_header {
    uint16_t code_size;
    uint16_t data_size;
    uint8_t signature[MODULE_SIGNATURE_SIZE];
};
```

## Troubleshooting

1. **Module won't load**: Check module size and available RAM
2. **Symbol not found**: Ensure symbol is exported in main program
3. **Module crashes**: Check for stack overflow or invalid memory access
4. **Upload fails**: Increase timeout, check serial connection