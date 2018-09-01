#include <stdio.h>
#include <stdlib.h>

int test(int *a, int *b)
{
  return *a + *b;
}


void *table[] = {&test};

int
main(void)
{

}
