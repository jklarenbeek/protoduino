#include <protoduino.h>
#include <sys/process.h>
#include <sys/autostart.h>

// Placeholder ELF binary (small AVR ELF, hardcoded or from file)
const uint8_t sample_elf[] = {/* TODO: Insert actual ELF binary data */};
uint16_t elf_size = sizeof(sample_elf);

// Loader Process (static, loads dynamic)
PROCESS(loader_process, "ELF Loader", 1);
PROCESS_THREAD(loader_process, ev, data)
{
  PT_BEGIN(&self->pt);
  // Load dynamic process
  struct process *dyn_proc = process_load_elf(sample_elf, elf_size, "DynamicProc");
  if (dyn_proc)
  {
    process_start(dyn_proc, NULL);
    SerialLine.println("Loaded dynamic process.");
  }
  else
  {
    SerialLine.println("Load failed.");
  }
  // Later, unload
  delay(5000);
  process_unload(dyn_proc);
  SerialLine.println("Unloaded dynamic process.");
  PT_END(&self->pt);
}

AUTOSTART_PROCESSES(&loader_process);

void setup()
{
  SerialLine.begin(9600);
  process_init();
  autostart_start();
}

void loop()
{
  process_run();
  delay(1);
}