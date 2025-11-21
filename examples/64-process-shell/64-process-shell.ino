#include <protoduino.h>
#include <sys/process.h>
#include <sys/autostart.h>
#include <utf/vt100.h>
#include <utf/utf8-stream.h>

// Shell Process
PROCESS(shell_process, "VT100 Shell", 0);
PROCESS_THREAD(shell_process, ev, data)
{
  static struct echo_pt pt_echo;
  PT_BEGIN(&self->pt);
  pt_echo.stream = &SerialLine;
  SerialLine.print(VT_CLEAR_SCREEN VT_CURSOR_HOME "Protoduino Shell> ");
  PT_FOREACH(&self->pt, &pt_echo, getch(&pt_echo))
  {
    if (pt_echo.value == KEY_ENTER)
    {
      SerialLine.println();
      // Parse command (placeholder: echo back)
      SerialLine.print("Command: ");
      // TODO: Implement parsing for ps, kill, pipe, load, etc.
      SerialLine.println("echo");
      SerialLine.print("Protoduino Shell> ");
    }
    else
    {
      utf8_putr(&SerialLine, pt_echo.value); // Echo input
    }
  }
  PT_ENDEACH(&self->pt);
  PT_END(&self->pt);
}

AUTOSTART_PROCESSES(&shell_process);

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