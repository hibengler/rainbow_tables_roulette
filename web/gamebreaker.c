/* This code is a plug in module to rtr - our web server
RTR is a very efficient single threaded web server, and this code is probably better
off with a multithreaded deal, as it has several blocks due to memory mapping 
100 gigabyte files in virtual memory.  In 10 years, this is trivial,  but not now.

*/



#define RANGE_SIZE 30

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "util.h"
#include "bsearch.h"




/* we have our own rand and srand functions bitch! - these are like they are with windows 
These are 16 bit random - not good at all probably will be able to break
its synergy in 3 or 4 moves bitch!
if running on a real system - like linux, comment these out
*/
	unsigned int crt_rand_seed = 0;
void srand(unsigned int seed)
	{
	  crt_rand_seed = seed;
	}
	 
	int rand()
	{
	  crt_rand_seed = crt_rand_seed * 0x343FD + 0x269EC3;
	  return (crt_rand_seed >> 0x10) & 0x7FFF;
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


struct searcher *main_file;




char *guess () {
return(name[(rand()>>2) % 38]);
}

/* 
base - main page
*/

void get_pos(char *buf,long long *pos) {
char *z = buf;
while (*buf) {
  if (*buf=='|') z=buf+1;
  buf++;
  }
*pos = atoll(z);
}


int dirfield(char *x, char *y,int fieldnum)
{
int flag=0;
char *z =y;
while (fieldnum && *x) {
  if ((*x == '/')) fieldnum--;
  x++;
  }
while (*x && (*x != '/')) {
  char ch=*x;
  if (ch!='/') 
    *z++ = ch;
  x++;
  }
*z='\0';
if (*x == '/') flag=1;
return flag;
}




void dir_remainder_to_pipe(char *x, char *y,int fieldnum)
{
int flag=0;
char *z =y;
while (fieldnum && *x) {
  if ((*x == '/')) {
    fieldnum--;
    }
  x++;
  }
while (*x) {
  char ch=*x;
  if (ch=='/') {
    *z++ = '|';
    }
  else {
    *z++ = ch;
    }
  x++;
  }
*z='\0';
}

void add_from_range_url( char *obuf,char *url,int from_range, int to_range) {      
      char gbuf[10000];
      char *t;
      char *o=gbuf;
      t=url;
      if (*t) *o++= *t++;
      while ((*t)&&(*t) != '/') {
        *o++ = *t++;
         }
      if (*t=='/') t++;
      if (from_range) {
        while ((*t)&&(*t) != '/') {
           t++;
	   }
        if (*t=='/') t++;
	}
      *o = '\0';
      if (to_range) {
        sprintf(obuf,"%s/_%d/%s",gbuf,to_range,t);
	}
      else {
        sprintf(obuf,"%s/%s",gbuf,t);
	}
      
}




void remove_range_url_trail( char *obuf,char *url,int from_range) {      
      char gbuf[10000];
      char *t;
      char *o=gbuf;
      t=url;
      if (*t) *o++= *t++;
      while ((*t)&&(*t) != '/') {
        *o++ = *t++;
         }
      if (*t=='/') t++;
      if (from_range) {
        while ((*t)&&(*t) != '/') {
           t++;
	   }
        if (*t=='/') t++;
	}
      *o = '\0';
      sprintf(obuf,"%s/%s",gbuf,t);
int l=strlen(obuf);
while ((l)&&obuf[l-1] != '/') {
  l--;
  }
obuf[l]='\0';
      
}



char *format_ccard(char *buf,char *in) {
int x=0;
while (*in) {
  if ((x)&&(x % 4)==0) {
    *buf++ = '-';
    }
  *buf++ = *in++;
  x=x+1;
  }
return buf;
}



char *html_header(char *buf,char *cookie_names[],char *cookie_values[]) {
sprintf(buf,"HTTP/1.1 200 OK\nContent-Type: text/html\n\n"
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n"
);
return buf;
}

char *html_footer(char *buf) {
*buf='\0';
return buf;
}

char * error_404(char *b,int *pbuflength) {
sprintf(b,"HTTP/1.1 404 Not Found\n"
"Content-Type: text/html; charset=iso-8859-1\n\n"
"<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
"<html><head>\n"
"<title>404 Not Found</title>\n"
"</head><body>\n"
"<h1>Not Found</h1>\n"
"<p>The requested URL was not found on this server.</p>\n"
"<hr>\n"
"</body></html>\n");
*pbuflength = strlen(b);
return b;
}



char *show_pos(char *buf,char *number,char *choice,int only) {

static int bcolors[38] = {
1,2,1,2,1,2,1,2,1,2,2,1,2,1,2,1,2,1,2,2,1,2,1,2,1,2,1,1,2,1,2,1,2,1,2,1};

int span;
char *bgcolor;
if ((strcmp(number,"0")==0)||(strcmp(number,"00")==0)) {
  span=3;
  bgcolor="forestgreen";
  }
else {
  span=2;
  int r;
  r=atoi(number);
  r=r-1;
  if (bcolors[r]==1) bgcolor="red"; else bgcolor="black";
  }

  
if (only) {
  if (strcmp(number,choice)==0) {
    sprintf(buf,"<td colspan=%d bgcolor=%s><a href=\"%s/\"><font size=5 color=#ffffff><center>%s</center></font></a></td>",
          span,bgcolor,number,number);
    }
  else {
    sprintf(buf,"<td colspan=%d bgcolor=%s><font size=5 color=#404040><center>%s</center></font></td>",
          span,bgcolor,number);
    }
  }
else {
  if (strcmp(number,choice)==0) {
    sprintf(buf,"<td colspan=%d bgcolor=%s><a href=\"%s/\"><font size=5 color=#eeeeee><center>%s</center></font></a></td>",
          span,bgcolor,number,number);
    }
  else {
    sprintf(buf,"<td colspan=%d bgcolor=%s><a href=\"%s/\"><font size=5 color=#bbbbbb><center>%s</center></font></td>",
          span,bgcolor,number,number);
    }
  }
return buf;
}


char *roulette_table(char *b,char *choice,int only)
{
char *e=b;
char b1[200];
char b2[200];
char b3[200];
sprintf(b,"<table bgcolor=forestgreen>"
"<tr>%s%s</tr>"
,show_pos(b1,"0",choice,only),show_pos(b2,"00",choice,only));
e += strlen(b);
int i;
for (i=1;i<36;i+=3) {
  char a1[10];
  char a2[10];
  char a3[10];
  sprintf(a1,"%d",i);
  sprintf(a2,"%d",i+1);
  sprintf(a3,"%d",i+2);
  sprintf(e,"<tr>%s%s%s</tr>\n",
       show_pos(b1,a1,choice,only),
       show_pos(b2,a2,choice,only),
       show_pos(b3,a3,choice,only)
       );
  e += strlen(e);
  }
sprintf(e,"</table>\n");
return(b);
}


static int easter=0;

char * display_page(    char *b, /* buffer to append stuff to */
int *buflength,
    char *url
)
{
char *f1;
char *f2;
char *e=b;
f1=strdup(url);
f2 = strdup(url);
int i=0;
char *f11=f1;
char *f22=f2;
char *x=f1;
char *y=f2;
easter=0;
if (strncmp(x,"/real",4)==0) {
 easter=1;
 x +=4;
 y+=4;
 f11+=4;
 f22 +=4;
 }
while (*x) {
  if (*x == '/') {
    i++;
    *x='|';
    *y = *x;
    if (i==7+1) { /* eithth slash is the 7th item */
      x[0] = '\0';
      }
    }
  x++;
  y++;
  }
if ((y>f22)&&(y[-1]=='|')) {
  i--;
  y[-1]='\0';
  }
else { /* need the trailing / duede!*/
  fprintf(stderr,"huh?\n");
  return(error_404(b,buflength));
  }


fprintf(stderr,"f11 %s f22 %s\n",f11,f22);
int found;
char results[1000];
long long nextline;
found =search_first_range_over(main_file,f11+1,results,&nextline,0);
fprintf(stderr,"search %s found %s fields %d\n",f1+1,results,0);
int l=strlen(f11+1);
do { /* just to get out of this loop with a break */
  if (strncmp(results,f11+1,l) ==0 ){
    char results2[1000];
    search_next_range_over(main_file,f11+1,results2,&nextline,0);
    search_next_range_over(main_file,f11+1,results2,&nextline,0);
    fprintf(stderr,"next %s\n",results2);

    
    if ((i>=8)||(strncmp(results2,f11+1,l) !=0 )) {
      /* great - we have a match.  Now lets make sure that they are
        following along.*/
      if ((!easter)&&(i==10)) { /* spoil the fun, man I am evil :)*/
        html_header(b,NULL,NULL);
        sprintf(e,"I cannot predict this far..... for free. It takes too much time to reset the quantum bits."
	      );
        e = e+ strlen(e);
        break; /* we are done */
        }
      if ((!easter)&&(i>10)) { /* so darn evil */
        return(error_404(b,buflength));
	}
      /* lets build the string */
      char buf4[10000];
      unsigned int a;
      long long al;
      char buf3[10000];
      field(results,buf3,7);
      al = atoll(buf3);
      a=al;
      srand(a);
      buf3[0]='\0';
      int k;
      for (k=0;k<i;k++) {
        sprintf(buf4,"%s",guess());
	strcat(buf3,"|");
	strcat(buf3,buf4);
	}
      fprintf(stderr,"buf3 is %s\n",buf3);
      fprintf(stderr,"f22 is %s\n",f22);
      /* make sure that we are on target */
      if (strcmp(buf3,f22)!=0) {
        return(error_404(b,buflength));
	}
      /* Ok, they are following along! cool.  Lets give them the answer */
      html_header(b,NULL,NULL);
      char buf2[20000];
      e = e+ strlen(e);
      char * answer = guess();
      sprintf(e,"<body background="
      "\"http://www.seekcodes.com/backgrounds/Abstact/mountain-at-sunset.jpg\" background-repeat=0>" 
    "<center><font color=cyan>Synergy is reached and sustained.  If you have a god,  better pray thanks right about now<br>\n"
             "<font size=10><br>The next number WILL BE <b>%s</b><br></font>\n%s",answer
	      ,roulette_table(buf2,answer,1));
      e = e+ strlen(e);
      if (easter) {
        sprintf(e,"<font color=white size=6>Hi Smart Person! OK Here are the rest of the numbers:<hr>%s \n",answer);
	e += strlen(e);
	int i,j;
	for (i=0;i<10;i++) {
	  for (j=0;j<10;j++) {
	    answer = guess();
	    sprintf(e,"  %s  ",answer);
	    e += strlen(e);
	    }
	  }
	}
      else {
        sprintf(e, "<font size=11>Bet everything on %s</font><br>\n",
	  answer);
        e = e+ strlen(e);
        sprintf(e, "<font size=10>%s %s %s %s %s %s %s %s</font>\n",
	  answer,answer,answer,answer,answer,answer,answer,answer);
        e = e+ strlen(e);
	}
      break;      
      
      }
    else {
      html_header(b,NULL,NULL);
      char buf2[20000];
      e = e+ strlen(b);
      if (i==7) {
        sprintf(e,"I am sorry,  but the synergy is lost.  Someone in an alternate universe is getting rich.\n");
        e = e+ strlen(e);
	}
      else {
        char *answer=guess();
      
        sprintf(e,"OK, we are %d deep now,  another %d steps or less and we will have synergy<br>\n"
              "You <b><i>should</i></b> take my advice here.  But if you don't,  it is ok- "
	      "other forces mighe be directing you to choose another one,  which is adding to the "
	      "strength of the quantum entanglement.  However, if you tell me a different result "
	      "than what actually happened,  the quantum link is usally corrupted."
	      "I would bet another seven bucks on square <b>%s</b>.  Then tell me the result<br>%s\n"
	      ,i,7-i,answer,roulette_table(buf2,answer,0
	      )
	      );
        e = e+ strlen(e);
	}
      break;
      }
    }
  else {
    if (i==7) {
      html_header(b,NULL,NULL);
      char buf2[20000];
      e = e+ strlen(b);
      sprintf(e,"I am sorry,  but the synergy is lost."
      "Check your numbers for mistakes or something\n");
      e = e+ strlen(e);
      break;
      }
    
    return error_404(b,buflength);
    }
  } while (0); /* just used so we can break out of this code */
free(f1);
free(f2);
*buflength = (e-b);
return b;
}    



int init (int argc, char *argv[]) {
int i;
for (i=1;i<argc;i++) {
    srandom(atoi(argv[i]));
  }
  

main_file = new_searcher("roulette.txt",13,0,0);
return 0;
}


int init_external_call() {
return (init(0,NULL));
}









void welcome_page(char *b,int *blen) {
html_header(b,NULL,NULL);
char buf[20000];
char *b2 = b+strlen(b);
rand();
char *choice=guess();
sprintf(b2,
"<title>Game breaker</title>"
"<head>Game breaker</head>"
"<hr>Using quantum entanglement [praying],  I am able to play and win the game.<br>"
"It takes about 6 or 7 plays, depending.<br>"
"It does not always work, as quantum is kinda quirky,<br>"
"But it works pretty well<br>"
"To Play,  Start up the game,  bet maybee 7 bucks on number <b>%s</b>."
"And tell me what number comes up<br><br>%s",choice,roulette_table(buf,choice,0));
*blen = strlen(b);
}



char  *make_external_call(char *buf,char *url,int *buflength) {
char *b=malloc(1000000);
char *e;
if (!b) return(strdup(""));
fprintf(stderr,"%s\n",url);
char f1[20000];
int offset=2;
int searching=dirfield(url,f1,1);

if (strcmp(f1,"")!=0) {
  return(display_page(b,buflength,url));
  }
else {
  welcome_page(b,buflength);
  return b;
  }
*buflength = (e-b);
return(b);
}
