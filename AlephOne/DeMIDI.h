/*
   Parse MIDI messages into raw pitch and expression.
   This is how an internal engine gets built.
   Its main interface with the outside world is putch/flush.
 */

void DeMIDI_start();
void DeMIDI_stop();

void DeMIDI_putch(char c);
void DeMIDI_flush();