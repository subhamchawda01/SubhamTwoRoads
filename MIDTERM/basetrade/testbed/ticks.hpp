#include <stdio.h>
#include <iostream>
#include <sys/time.h>
#include <time.h>

typedef unsigned long long ticks;
static __inline__ ticks GetTicks(void) {
  unsigned a, d;
  // asm volatile("cpuid":"=a"(a),"=d"(d):"0"(0):"ecx","ebx");
  asm volatile("rdtsc" : "=a"(a), "=d"(d));
  return ((ticks)a) | (((ticks)d) << 32);
}
