#ifndef tools_h
#define tools_h
#include <Arduino.h>

#ifdef DEBUG_ENABLE

  #define PRINTCHAR(a) {  Serial.print(a); }
  #define PRINT(a) Serial.print( (a))
  #define PRINTLN(a) { Serial.println( a);}
  #define PRINT2(a,b) Serial.print( String(a,b))
  #define PRINTLN2(a,b) { Serial.println( String(a,b));}
  #define PRINT_RAM(text) { PRINT(text); \
                            PRINT( ": FreeHeap:"); \
                            PRINT( ESP.getFreeHeap()); \
                            PRINT( " MaxAllocHeap:"); \
                            PRINTLN( ESP.getMaxAllocHeap()); }

  #define MS(...)  {  \
                    unsigned long timeStart = millis(); \
                    PRINTLN("- Executing: " #__VA_ARGS__); \
                    __VA_ARGS__; \
                    PRINT("- Time:");\
                    PRINT( millis() - timeStart); \
                    PRINTLN(" ms");\
                  }

  #define MS1(...)  {  \
                    unsigned long timeStart = millis(); \
                    __VA_ARGS__; \
                    PRINT("- Executed: " #__VA_ARGS__ " Time:"); \
                    PRINT( millis() - timeStart); \
                    PRINTLN(" ms");\
                  }

  #define MS50(...)  {  \
                    unsigned long timeStart = millis(); \
                    __VA_ARGS__; \
                    if ((millis() - timeStart) > 50) { \
                      PRINT("- Executed " #__VA_ARGS__ " Time:"); \
                      PRINT( millis() - timeStart); \
                      PRINTLN(" ms");\
                    } \
                  }

#else
  #define PRINT_RAM(text) {}
  #define PRINTCHAR(a) {}
  #define PRINT(a) {}
  #define PRINTLN(a) {}
  #define PRINT2(a,b) {}
  #define PRINTLN2(a,b) {}
  #define MS(...) __VA_ARGS__;
  #define MS1(...) __VA_ARGS__;
  #define MS50(...) __VA_ARGS__;
#endif //DEBUG_ENABLE

#endif //tools_h