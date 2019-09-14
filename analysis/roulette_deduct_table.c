/* This is a second proof in concept, showing that although using the random number generator srand
might have random numbers of low peridocity,  but since everyone starts at the beginning of these
chains,  there are only 4 billion chains.
Because of this,  looking at the random numbers makes it very predictable in determining
what the next random number is,  which can give an advantage in many circumstances.

For this example,  we are going to have a random generate letters call
lets say that it generates digits (0-9) randomly.  And let say that the algorithm is known,
as well as the platform, etc.

Lets say the psuedo code is something like this:

srand((something from /dev/random))
while (TRUE)
  printf("%c",(random()>>2) % 10 + '0');



It is easy to make a table that can help in predicting the next number.  And once we have enough
digits,  it is possible to predict with certainty.


This example is for roulette,  with witch you can guess the random key in 6, ocasionally 7 guesses.



Our first iteration will take the function - and print out the following:

res1 | res2 | res3 | res4 | res5 | res6 | res7 | key


And there will be 4 billion keys.

With this sorted, it is easy to see the results, and easy to calculate things.




*/

#include <stdio.h>
#include <stdlib.h>




/*********************************************************************
 *              srand (MSVCRT.@)
 */
int random_seed;
void  srand( unsigned int seed )
{
    random_seed = seed;
}

/*********************************************************************
 *              rand (MSVCRT.@)
 */
int rand(void)
{
    /* this is the algorithm used by MSVC, according to
     * http://en.wikipedia.org/wiki/List_of_pseudorandom_number_generators */
    random_seed = random_seed * 214013 + 2531011;
    return (random_seed >> 16) & 0x7fff;
}




char *name[38] = {"00",
"0",
"1",
"2",
"3","4","5","6","7","8","9","10",
"11","12","13","14","15","16","17","18","19",
"20","21","22","23","24","25","26","27","28","29",
"30","31","32","33","34","35","36"
};


char *generate_numbers(char *buf,int length) {
int i;
for (i=0;i<length;i++) {
  int n=(rand()>>2) % 38;
  *(buf++) = n;
  }
*(buf++) = '\0';
return buf;
}





void generate_full_table(FILE *fd) {
unsigned int key = 0;
do {
  char buf[200];
  srand(key);
  generate_numbers(buf,7);
  fprintf(fd,"%s|%s|%s|%s|%s|%s|%s|%u\n",name[buf[0]],name[buf[1]],name[buf[2]],name[buf[3]],name[buf[4]],name[buf[5]],name[buf[6]],key);
  key++;
  } while (key);
}


int main() {
generate_full_table(stdout);
exit(0);
}
