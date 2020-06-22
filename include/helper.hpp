#include <Arduino.h>

#ifndef HELPER_HPP
#define HELPER_HPP

template <typename T>
inline void print(Stream& s, T last)
{
  s.print(last);
  s.print("\r\n");
}

template <typename T, typename... Args>
inline void print(Stream& s, T head, Args... tail)
{
  s.print(head);
  print(s, tail...);
}

/* // C++17 version 
template <class... Ts>
void print_all(Stream& s, Ts const&... args) {
    ((s.print(args)), ...);
}*/

// generic solution
template <class T>
uint8_t numDigits(T number)
{
  uint8_t digits = 0;
  if (number <= 0) digits = 1; // remove this line if '-' counts as a digit
  while (number) {
    number /= 10;
    digits++;
  }
  return digits;
}
#endif