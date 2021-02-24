#ifndef SERIALLOGGER
#define SERIALLOGGER

#include <Arduino.h>

class SerialLogger
{
public:
  SerialLogger();
  void Info(String message);
  void Error(String message);
};

extern SerialLogger Logger;

#endif // SERIALLOGGER
