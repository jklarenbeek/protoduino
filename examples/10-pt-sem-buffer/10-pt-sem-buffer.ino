/*
 * Copyright (c) 2004-2005, Swedish Institute of Computer Science.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 *
 * This file is part of the protothreads library.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 * $Id: example-buffer.c,v 1.5 2005/10/07 05:21:33 adam Exp $
 */

/**
 * The following example shows how the producer-consumer problem, also
 * known as the bounded buffer problem, can be solved using
 * protothreads and semaphores.
 *
 * The program uses three protothreads: one protothread that
 * implements the consumer, one thread that implements the producer,
 * and one protothread that drives the two other protothreads. The
 * program uses three semaphores: "full", "empty" and "mutex". The
 * "mutex" semaphore is used to provide mutual exclusion for the
 * buffer, the "empty" semaphore is used to block the consumer is the
 * buffer is empty, and the "full" semaphore is used to block the
 * producer is the buffer is full.
 *
 * The "driver_thread" holds two protothread state variables,
 * "pt_producer" and "pt_consumer". It is important to note that both
 * these variables are declared as <i>static</i>. If the static
 * keyword is not used, both variables are stored on the stack. Since
 * protothreads do not store the stack, these variables may be
 * overwritten during a protothread wait operation. Similarly, both
 * the "consumer" and "producer" protothreads declare their local
 * variables as static, to avoid them being stored on the stack.
 * 
 */

#include <protoduino.h>
#include <sys/pt-sem.h>
#include <dbg/examples.h>

#define NUM_ITEMS 32
#define BUFSIZE 8

static int buffer[BUFSIZE];
static int bufptr;

static void add_to_buffer(int item)
{
  buffer[bufptr] = item;
  bufptr = (bufptr + 1) % BUFSIZE;
}

static int get_from_buffer(void)
{
  int item;
  item = buffer[bufptr];

  bufptr = (bufptr + 1) % BUFSIZE;
  return item;
}

static int produce_item(void)
{
  static int item = 0;

  return item++;
}

static void consume_item(int item)
{
  SerialLine.print("Item ");
  SerialLine.print(item);
  SerialLine.println(" consumed");
}

static struct pt_sem full, empty;
 
static PT_THREAD(producer(struct pt *pt))
{
  static int produced;
  
  PT_BEGIN(pt);
  
  for(produced = 0; produced < NUM_ITEMS; ++produced) {
  
    PT_SEM_WAIT(pt, &full);
    
    add_to_buffer(produce_item());
    
    PT_SEM_SIGNAL(pt, &empty);
  }

  PT_END(pt);
}
 
static PT_THREAD(consumer(struct pt *pt))
{
  static int consumed;
  
  PT_BEGIN(pt);
 
  for(consumed = 0; consumed < NUM_ITEMS; ++consumed) {
    
    PT_SEM_WAIT(pt, &empty);
    
    consume_item(get_from_buffer());    
    
    PT_SEM_SIGNAL(pt, &full);
  }
 
  PT_END(pt);
}
 
static PT_THREAD(driver_thread(struct pt *pt))
{
  static struct pt pt_producer, pt_consumer;
 
  PT_BEGIN(pt);
  
  PT_SEM_INIT(&empty, 0);
  PT_SEM_INIT(&full, BUFSIZE);
 
  PT_INIT(&pt_producer);
  PT_INIT(&pt_consumer);
 
  PT_WAIT_THREAD(pt, producer(&pt_producer) &
		     consumer(&pt_consumer));
 
  PT_END(pt);
}

struct pt driver_pt;

void setup()
{
  print_setup();
}

void loop()
{
  PT_INIT(&driver_pt);

  while(PT_ISRUNNING(driver_thread(&driver_pt))) {

    /*
     * When running this example on a multitasking system, we must
     * give other processes a chance to run too and therefore we call
     * usleep() resp. Sleep() here. On a dedicated embedded system,
     * we usually do not need to do this.
     */
#ifdef _WIN32
    Sleep(0);
#elif defined(ARDUINO)
    delay(2000);
#else
    usleep(10);
#endif
  }

}
