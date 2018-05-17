#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[]) {
  char s1[4];
  char s2[] = "test         ";
  strncpy(s1, s2, 4);
  printf("%s+\n",s1);
  printf("%s\n",s1);
  return 0;
}
