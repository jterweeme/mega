#include "util.h"

__extension__ typedef int __guard __attribute__((mode (__DI__)));
extern "C" int __cxa_guard_acquire(__guard *);
extern "C" void __cxa_guard_release (__guard *);
extern "C" void __cxa_guard_abort (__guard *);

extern "C" void __cxa_pure_virtual(void);

int __cxa_guard_acquire(__guard *g) {return !*(char *)(g);};
void __cxa_guard_release (__guard *g) {*(char *)g = 1;};
void __cxa_guard_abort (__guard *) {};

void __cxa_pure_virtual(void) {};

uint32_t millis()
{
    return g_millis;
}

size_t Prynt::write(const uint8_t *buffer, size_t size)
{
    size_t n = 0;

    while (size--)
        n += write(*buffer++);
  
    return n;
}

size_t Prynt::print(const char str[])
{
    return write(str);
}

size_t Prynt::print(char c)
{
  return write(c);
}

size_t Prynt::print(unsigned char b, int base)
{
  return print((unsigned long) b, base);
}

size_t Prynt::print(int n, int base)
{
  return print((long) n, base);
}

size_t Prynt::print(unsigned int n, int base)
{
  return print((unsigned long) n, base);
}

size_t Prynt::print(long n, int base)
{
  if (base == 0) {
    return write(n);
  } else if (base == 10) {
    if (n < 0) {
      int t = print('-');
      n = -n;
      return printNumber(n, 10) + t;
    }
    return printNumber(n, 10);
  } else {
    return printNumber(n, base);
  }
}


size_t Prynt::print(unsigned long n, int base)
{
  if (base == 0) return write(n);
  else return printNumber(n, base);
}

size_t Prynt::printNumber(unsigned long n, uint8_t base)
{
  char buf[8 * sizeof(long) + 1]; // Assumes 8-bit chars plus zero byte.
  char *str = &buf[sizeof(buf) - 1];

  *str = '\0';

    if (base < 2)
        base = 10;

    do {
    unsigned long m = n;
    n /= base;
    char c = m - base * n;
    *--str = c < 10 ? c + '0' : c + 'A' - 10;
    } while(n);

    return write(str);
}

clock_time_t clock_time(void)
{
    return (clock_time_t) millis();
}

void uip_timer_set(struct uip_timer *t, clock_time_t interval)
{
    t->interval = interval;
    t->start = clock_time();
}

void uip_timer_reset(struct uip_timer *t)
{
    t->start += t->interval;
}

void uip_timer_restart(struct uip_timer *t)
{
    t->start = clock_time();
}

int uip_timer_expired(struct uip_timer *t)
{
    return (clock_time_t)(clock_time() - t->start) >= (clock_time_t)t->interval;
}






