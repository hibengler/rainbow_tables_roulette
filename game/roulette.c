/* V1.1
V1.1 fixed a bug - when you tell it to bet on 0, it chooses 00 and vise versa. Not any more
V1.0 original roulette
*/


#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#define PLATFORM_WINDOWS
#endif
#ifdef _WIN64
#define PLATFORM_WINDOWS
#endif
#ifdef __linux__
#define PLATFORM_UNIX
#endif


#ifdef PLATFORM_WINDOWS
#include <windows.h>
#define SystemFunction036 NTAPI SystemFunction036
#include <NTSecAPI.h>
#undef SystemFunction036




unsigned int seed_from_very_random_source() {
HINSTANCE hLib=LoadLibrary("ADVAPI32.DLL");
if (hLib) {
BOOLEAN (* pfn)() =
(BOOLEAN (*)())
    GetProcAddress(hLib,"SystemFunction036");
  if (pfn) {
    char buff[32];
    ULONG ulCbBuff = sizeof(buff);
    if(pfn(buff,ulCbBuff)) {
      unsigned int a;
      a = *((unsigned int *)buff);
      FreeLibrary(hLib);
	  return a;
      }
    else {
      fprintf(stderr,"minor error\n");
      exit(-1);
      }
    FreeLibrary(hLib);
    }
  }
fprintf(stderr,"major error\n");
exit(-1);
}



#endif




#ifdef __linux__
unsigned int seed_from_very_random_source() {
/* first thing - get a truely random number from /dev/random*/
FILE *rfd;
rfd = fopen("/dev/random","r");
if (!rfd) {
  fprintf(stderr,"Sorry, but I have no source of random numbers,  I cannot play\n");
  exit(-1);
  }
unsigned int a;
fread(&a,sizeof(unsigned int),1,rfd);
fclose(rfd);
return a;
}

#endif




int sleep(int x) {
system("sleep 1");
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


int get_bet()
{
	char buf[100000];
    scanf("%s",buf);
    int a;
    a=0;
    sscanf(buf,"%d",&a);
    if (a>36) return -1;
    if (a<0) return -1;
    if (a==0) {
		if (strcmp(buf,"0")==0) return 1; /* In ver 1.1 this was 0 by mistake */
		if (strcmp(buf,"00")==0) return 0; /* in ver 1.1 this was 1 by mistake */
		return -1;
	}
	return a+1;
}



int main() {
/* first thing - get a truely random number from microsofts  random number gen or /dev/random
well, I don't know that the number is anyways.*/
unsigned int a;
a = seed_from_very_random_source();
srand(a);

printf("Simple Roulette\n");
printf("Go ahead and place one chip - 1 through 36, 0 or 00\n");
printf("This is simple because we only let you bet on the one number. In regular roulette you can bet all kinds of ways\n");
printf("If you win it is 35 to 1, unless you bet on 0,  or 00,  then the win is 17 to 1.\n");
printf("This gives me,  the house a 5.3%% advantage,  it helps me pay for the electricity\n");
printf("I have deducted $100.00 from your bank account. Once done, I will give back the winnings\n");
printf("Here we go\n");

double bank=100.;
while (1) {
  double bet;
  if (bank<0.01) {
    printf("You are out. Thanks for the money!\n");
    break;
    }
  printf("How much do you want to bet (0 to finish)?\n");
  scanf("%lf",&bet);
  if (bet==0.) {
    if (bank>100.)
      printf("Ahhhh, you beat me.  Heres your $%10.2lf\n",bank);
    else
      printf("OK, thanks for playing. heres your $%3.2lf\n",bank);
    break;
    }
  if (bet > bank) {
    printf("Your bet is too big.\n");
    continue;
    }
  if (bet<1.00) {
    printf("Your bet is too small.\n");
    continue;
    }
  /* the bet is good */
  printf("Which number do you want to bet on (0, 00, or 1-36)\n");
  int num=-1;
  num =get_bet();
  if ((num<0)) {
    printf("Thats not a valid bet\n");
    continue;
    }

  printf("OK.  you are betting %10.2lf on %s: Let me spin.",bet,name[num]);
  fflush(stdout);
  printf(".");
  fflush(stdout);
  sleep(1);
  printf(".");
  fflush(stdout);
  sleep(1);

  int spin = (rand()>>2) % 38;
  printf(" %s\n",name[spin]);

  if (spin==num) {
    double win;
    if (num<2) {
      win = bet*18;
      }
    else {
      win=bet*36;
      }
    bank += win - bet;
    printf("You win $%10.2lf\n",win);
    }
  else {
    bank -= bet;
    printf("Lost that one.  Thanks for the $%10.2lf\n",bet);
    }
  printf("You have $%10.2lf left\n",bank);
  }
exit(0);
}
