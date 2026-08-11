#ifndef PROCESSOR_H
#define PROCESSOR_H

#define NO_OF_REGISTERS 256
#define CONST_VALUE_MIN 0
#define CONST_VALUE_MAX 255

void reset();
void fetch();
void decode();
void execute();

#endif