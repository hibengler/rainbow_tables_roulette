



#include <sys/poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <time.h>
#include <linux/tcp.h>
#include <unistd.h>
/*
This is a web server,  we will do a simple one for now.  
So when there is a hit,  it gets called.
It parses the header and sends a reponse based on the header.

The header tends to have range requests.  Range 0-1 is a special one.

*/

/* note - look at the following web site for performance tips

http://www.ibm.com/developerworks/linux/library/l-hisock.html
*/


#define READ_COUNT 2048
#define MAX_ACCESSORIES 10000
#define MAX_SOCKETS 10000
#define MAX_POLLS 131071
/* max polls = max_sockets *2+1 + 10 for fudge
+1 because of listening on main port.
*2 because one poll for the socket,  one poll for the file/pipe/whatever */
#define LISTENQUEUE_SIZE 10000

/* first to call
handle_web_event_01_accept
handle_web_event_02_receive
  handle_web_event_01_accept
  handle_web_event_02_receive
  handle_web_event_01_accept
  handle_web_event_02_receive
  handle_send_out_web_header_and_data_01
  */

/* action is a generic action that requires work */
struct action {
FILE *file;
int filedes;
long position;
long end_position;
int type; /* type of action - defines what the heck this is. */
int parentfd; /* parent file descriptor */
struct sockaddr_in *client_sap;
struct sockaddr_in *server_sap;
int poll_position;
char *buf1; /* buffer to read in */
int buf1_length;
char *buf2;
int buf2_length;
int reader_fd;
int mode;  /* 
for read mode of 0 - havent started IO.
              mode of 1 - started IO
	      mode of 2 - finished IO
For write - 2 - writing out now
1 - ready to write out something
0 - also ready to write out something
*/
int accessory;
};
/* for a request action,  the request string goes in buf1 and the 
header response goes in buf2 
and reader_fd points to the reading file action.
The reading file action has buf1 as an input buffer,  and buf2 as an output 
buffer -  There are two types- stream and file io.  File io can be seeked, 
stream can not be seeked.
First,  we open some movie files,  then we will move on to 
generating a movie from sound and pictures.



*/




/* url_access -
this is a url that is being read currently - and has a file descriptor 
*/
struct url_access {
char *url;
char *filename;
long size;
char *type;
int stream_only;
time_t time_opened;
int filedes;
int request_sd;
};


#define MAX_ACCESSORIES 10000
struct url_access accessories[MAX_ACCESSORIES];
int no_accessories=0;

struct action actions[10000];
int no_actions=0;

struct pollfd fds[10000];
int no_fds=0;

void d(char *x) {
fprintf(stderr,"%s\n",x);
}

#define ACTION_WEB_LISTENER 1
#define ACCEPT_INCOMING_WEB_REQUEST 2
#define SEND_OUT_WEB_HEADER 3

#define SEND_OUT_WEB_HEADER_AND_DATA 4
#define READ_FROM_FILE_OR_STREAM 5
#define WRITE_DATA_FROM_STREAM 6


/* ice */
#define ICE_SEND_THE_ARE_WE_UP_REQUEST 7


char *event_type_string[8] = {"","ACTION_WEB_LISTENER","ACCEPT_INCOMING_WEB_REQUEST",
"SEND_OUT_WEB_HEADER",
"SEND_OUT_WEB_HEADER_AND_DATA",
"READ_FROM_FILE_OR_STREAM",
"WRITE_DATA_FROM_STREAM",
"ICE_SEND_THE_ARE_WE_UP_REQUEST"};


char * convert_URL_to_filename(struct action *a,char *url)
{
while ((*url)&&(*url == '/')) url++;
if (url[0]=='\0') {
  url="index.html";
  }
return (strdup(url));
}


char *determine_type_based_on_filename(char *fname)
{
int i;
int l;
char *extention;

l=strlen(fname);
for (i=l-1;i>=0;i--) {
  if (fname[i] == '.') {break;}
  }
i++;

extention = fname+i;


if (strcmp(extention,"html")==0) {
  return("text/html");
  }
else if (strcmp(extention,"m4v")==0) {
  return("video/x-m4v");
  }
else if (strcmp(extention,"3gp")==0) {
  return("video/3gpp");
  }
return("text/plain");
}




int add_to_poll (int fd, FILE *f,
 int type_of_poll,
 int parent_fd,
 struct sockaddr_in *client_sap,
 struct sockaddr_in *server_sap,
 short eventflags,
 char *buf1,
 int buf1_length,
 char *buf2,
 int buf2_length,
 int accessory_id,
 long position,
 long end_position) 
{
if (!fd) fd = fileno(f);
fds[no_fds].fd = fd;
fds[no_fds].events=eventflags;
fds[no_fds].revents=0;

{
  struct action *a;
  a=actions + fd;
  a->file=f;
  a->filedes=fd;
  a->position=position;
  a->end_position=end_position;
  a->type=type_of_poll;
  a->parentfd=parent_fd;
  a->client_sap = client_sap;
  a->server_sap = server_sap;
  a->poll_position=no_fds;
  a->buf1=buf1;
  a->buf1_length=buf1_length;
  a->buf2=buf2;
  a->buf2_length=buf2_length;
  a->reader_fd=0;
  a->mode=0;
  a->accessory = accessory_id;
  }
no_fds++;
no_actions++;

}








int initial_get_accessory(struct action *a,char *url) {
struct url_access acs;
struct stat filestat;
int test;

acs.url = strdup(url);
acs.filename = convert_URL_to_filename(a,url);
acs.filedes = open(acs.filename,O_RDONLY|O_NONBLOCK);
if (acs.filedes == -1) {
  free(acs.filename);
  acs.filename=NULL;
//  fprintf(stderr,"cannot open %s: %d\n",acs.filename,errno);
//  free(acs.filename);
  free(acs.url);
  return(-1);
  }
acs.type = determine_type_based_on_filename(acs.filename);

test=fstat(acs.filedes,&filestat);
if (test) {
  fprintf(stderr,"error stating file %s: %d\n",acs.filename,errno);
  }
acs.size = filestat.st_size;
acs.stream_only=0;
acs.time_opened=time(NULL);
acs.request_sd=0;
if (no_accessories < MAX_ACCESSORIES) {
  int acc=no_accessories;
  a->accessory=acc;
  actions[acs.filedes].accessory = acc;
  accessories[no_accessories++] = acs;
  return(acc);
  }
else {
  fprintf(stderr,"out of accessories! ???\n");
  exit(-1);
  return(0);
  }
}






void print_action (FILE *fd) 
{
int i;
fprintf(fd,"%d Actions\n",no_actions);
for (i=0;i<no_actions;i++) {
  struct action *a;
  a=actions+i;
  if (a->filedes) {
    fprintf(fd,"  Action %d fd %d ",i,a->filedes);
    fprintf(fd,"des %d Position %ld end %ld type %s parent %d poll %d mode %d reader %d accessory %d\n",
      a->filedes,a->position,a->end_position,event_type_string[a->type],
      a->parentfd,a->poll_position,
         a->mode,a->reader_fd,a->accessory);
    }
  }
fprintf(fd,"end actions\n");
}


void print_accessories (FILE *fd) 
{
int i;
fprintf(fd,"%d Accessories\n",no_actions);
for (i=0;i<no_accessories;i++) {
  struct url_access *u;
  u=accessories+i;
  if (u->filedes) {
    fprintf(fd,"  Acc %d Url %s name %s size %ld type %s request %d  ",i,
          u->url,u->filename,u->size,u->type,u->request_sd);
    }
  }
fprintf(fd,"end accessories\n");
}




void print_polls (FILE *fd) 
{
int i;
fprintf(fd,"%d polls\n",no_fds);
for (i=0;i<no_fds;i++) {
  struct pollfd *f;
  struct action *a;
  f=fds+i;
  a = actions+f->fd;
  fprintf(fd,"  Poll %d (",i);
  {
    short flag;
    flag=f->events;
    if (flag&POLLIN) fprintf(fd,"POLLIN ");
    if (flag&POLLPRI) fprintf(fd,"POLLPRI ");
    if (flag&POLLOUT) fprintf(fd,"POLLOUT ");
    if (flag&POLLERR) fprintf(fd,"POLLERR ");
    if (flag&POLLHUP) fprintf(fd,"POLLHUP ");
    if (flag&POLLNVAL) fprintf(fd,"POLLNVAL ");
  }
  fprintf(fd,") <");
  {
    short flag;
    flag=f->revents;
    if (flag&POLLIN) fprintf(fd,"POLLIN ");
    if (flag&POLLPRI) fprintf(fd,"POLLPRI ");
    if (flag&POLLOUT) fprintf(fd,"POLLOUT ");
    if (flag&POLLERR) fprintf(fd,"POLLERR ");
    if (flag&POLLHUP) fprintf(fd,"POLLHUP ");
    if (flag&POLLNVAL) fprintf(fd,"POLLNVAL ");
  }
  fprintf(fd,">");
  fprintf(fd,"  Action %d fd %d ",f->fd,a->filedes);
  if (a->filedes==0) {fprintf(fd,"CLOSED!!!  ");}
  fprintf(fd,"Position %ld end %ld type %s parent %d poll %d mode %d reader %d accessory %d\n",
      a->position,a->end_position,event_type_string[a->type],a->parentfd,a->poll_position,
         a->mode,a->reader_fd,a->accessory);
  }
fprintf(fd,"end actions\n");
}






int read_me (struct action *read_a) {
  
int cnt;
cnt =READ_COUNT; /* usual amount to read in */
if ((read_a->end_position != -1)&&(read_a->position > read_a->end_position)) {
  read_a->mode=3;
  }  
else {
  if ((read_a->end_position != -1)&&(read_a->position + cnt > 
    read_a->end_position+1)) {
    cnt = read_a->end_position +1 - read_a->position; 
    }

  cnt = read(read_a->filedes,read_a->buf1,cnt); /* send it out */
  if (cnt==-1) {fprintf(stderr,"error reading from media stream: %d\n",errno);}
  else if (cnt==0) {read_a->mode=3;}
  else {
    read_a->mode=1; /* we are reading a buffer in now */
    if ((read_a->end_position != -1l)&&(read_a->position + cnt >
            read_a->end_position+1)) {
      cnt = read_a->end_position + 1 - read_a->position;
      }
    read_a->position += cnt;        
    read_a->buf1_length = cnt;
    }
  fds[read_a->poll_position].events |= (POLLIN||POLLPRI);
  }
return(0);
}



extern int init_external_call();
extern char *make_external_call(char *buf,char *url,int *buflength);

extern int handle_send_out_web_header_01(struct action *a);


void change_state_to_send_web_header(int sd) {
struct action *a;
a=actions+sd;
/* change our state so that we start to write out the web page */
a->type=SEND_OUT_WEB_HEADER;
fds[a->poll_position].events=POLLOUT|POLLERR|POLLHUP|POLLNVAL;

/* ok,  now dump out the response */
handle_send_out_web_header_01(a);
}








void change_state_to_send_web_header_and_data(int sd) {
struct action *a;
a=actions+sd;
/* change our state so that we start to write out the web page */
a->type=SEND_OUT_WEB_HEADER_AND_DATA;
fds[a->poll_position].events=POLLOUT|POLLERR|POLLHUP|POLLNVAL;

}









int add_buffer(char **b, int *pl,char *n, int nl);

void just_prove_byte_range_ok_response(int sd2) {
/* The next test the ipod does is to see if the byte range requests work.  We lie and say,  yeah they do and then send 2 bytes of junk.
Again,  there is no need to open the file yet,  but we probably should set up and call everything properly here
If we make a set of processed that stream the current values - yeah!  Then the next step is to just stream them over.
*/
char obuf[10000];
struct action *a;
  struct url_access *ac;
  a=actions+sd2;
  ac = accessories+a->accessory;
  
/*sprintf(obuf,
"HTTP/1.1 304 Not Modified\n\
Date: Sat, 27 Dec 2008 13:39:18 GMT\n\
Server: Apache/1.3.39 (Unix) mod_perl/1.30\n\
Connection: close\n\n");*/

sprintf(obuf,
"HTTP/1.1 206  Partial Content\n\
Date: Sat, 27 Dec 2008 13:39:18 GMT\n\
Server: Apache/1.3.39 (Unix) mod_perl/1.30\n\
Last-Modified: Mon, 09 Jul 2007 02:31:32 GMT\n\
Accept-Ranges: bytes\n\
Content-Length: 2\n\
Content-Range: bytes 0-1/2000000000\n\
Keep-Alive: timeout=5, max=100\n\
Connection: Keep-Alive\n\
Content-Type: %s\n\n",ac->type);



{  add_buffer(&(a->buf2),&(a->buf2_length),obuf,strlen(obuf));
  }

change_state_to_send_web_header(sd2);


}





char *gettotoken(char *buf,int *bptr,char token) {
char *line;
int s=*bptr;
if (!buf[*bptr]) return(NULL);

while ((buf[s])&&(buf[s] != token)) {
  s++;
  }

line = malloc(sizeof(char)*(s-*bptr+1));
if (!line) return (NULL);
strncpy(line,buf+*bptr,(s-*bptr));
line[(s-*bptr)] = '\0';
if ((buf[s])) s++;
*bptr=s;
return(line);
}



char *getaline(char *buf,int *bptr) {
return(gettotoken(buf,bptr,'\n'));
}





/* here we got a complete request in.  So lets handle it and print out the proper response */
void external_call(struct action *a,int s,int sd2,struct sockaddr_in *server_sap,
struct sockaddr_in *client_sap, char *buf,char *url)
{

char *method;
char *request;

int accessory_id;
{
  struct action *a;

  a=actions+sd2;
  if (a->buf2) free(a->buf2);
  a->buf2=make_external_call(buf,url,&(a->buf2_length));
  change_state_to_send_web_header(sd2);
  }

/*
    response = "HTTP/1.1 200 OK";
      sprintf(obuf,"%s\n\
Date: Mon, 09 Jul 2007 02:40:56 GMT\n\
Server: Apache/2.2.4 (Unix)\n\
Last-Modified: Mon, 09 Jul 2007 02:31:32 GMT\n\
Accept-Ranges: bytes\n\
Content-Length: 2000000000\n\
Keep-Alive: timeout=5, max=100\n\
Connection: Keep-Alive\n\
Content-Type: %s\n\n",response,ac->type);
*/
  /* set up this as the response - one time to be sent out.  After this,  the other part will be sent*/

}








/* here we got a complete request in.  So lets handle it and print out the proper response */
void new_request(struct action *a,int s,int sd2,struct sockaddr_in *server_sap,
struct sockaddr_in *client_sap, char *buf) 
{

int bufptr;
char *line;
char url[10000];
char type[10000];
char *method;
char *request;

int accessory_id;

long range_from;
long range_to;

url[0]='\0';
type[0]='\0';

bufptr=0;

range_from=-1;
range_to=-1;
/* parse the request */
while (line=getaline(buf,&bufptr)) {
  /* see if this is a range request */
  if (strncmp(line,"GET ",4)==0) { /* get */
    sscanf(line+4,"%s %s",url,type);
    }
  else if (strncmp(line,"Range: bytes=",13)==0) { /* range */
    sscanf(line+13,"%ld-%ld\n",&range_from,&range_to);
    }    
  if (line) free(line);
  }


{
    accessory_id= initial_get_accessory(actions+sd2,url);
    if (accessory_id==-1) {
      external_call(a,s,sd2,server_sap,client_sap,buf,url);
      return; 
      }
   }
 

/* Ok - we do different things here.
The Apple iphone will send an initial request - just to know if byte range is accepatable.
Then it will do a range for 0 to 1 just to test that the range does work.
We respond in kind again - just by sending a couple of dummy bytes.  We also send a bogus size of 2 billion
And then we ger a third request - gange bytes 0-1999999999 and we are on!
*/

if (0 &&(range_from == 0)&&(range_to==1)) {
  just_prove_byte_range_ok_response(sd2); /* send 2 bogus bytes just to prove that we can do byte range */
   /* note - close_it will be 1 but we don't care. We open the file eaely */
   return;
  }


{
/* The next test the ipod does is to see if the byte range requests work.  We lie and say,  yeah they do and then send 2 bytes of junk.
  Again,  there is no need to open the file yet,  but we probably should set up and call everything properly here
  If we make a set of processed that stream the current values - yeah!  Then the next step is to just stream them over.
  */
  char obuf[10000];
  long size;
  long current;
  char *response;
  struct url_access *ac = accessories+a->accessory;
  obuf[0]='\0';

  if ((range_from ==-1)&&(range_to==-1)) {
    response = "HTTP/1.1 200 OK";
      sprintf(obuf,"%s\n\
Date: Mon, 09 Jul 2007 02:40:56 GMT\n\
Server: Apache/2.2.4 (Unix)\n\
Last-Modified: Mon, 09 Jul 2007 02:31:32 GMT\n\
Accept-Ranges: bytes\n\
Content-Length: 2000000000\n\
Keep-Alive: timeout=5, max=100\n\
Connection: Keep-Alive\n\
Content-Type: %s\n\n",response,ac->type);
    }
  else {
    response = "HTTP/1.1 206 Partial Content";
    sprintf(obuf,"%s\n\
Date: Mon, 09 Jul 2007 02:40:56 GMT\n\
Server: Apache/2.2.4 (Unix)\n\
Last-Modified: Mon, 09 Jul 2007 02:31:32 GMT\n\
Accept-Ranges: bytes\n\
Content-Length: %ld\n\
Content-Range: bytes %ld-%ld/2000000000\n\
Keep-Alive: timeout=5, max=100\n\
Connection: Keep-Alive\n\
Content-Type: %s\n\n",response,(range_to+1-range_from),range_from,range_to,ac->type);

    }


  





  /* set up this as the response - one time to be sent out.  After this,  the other part will be sent*/
  {struct action *a;
    a=actions+sd2;
    add_buffer(&(a->buf2),&(a->buf2_length),obuf,strlen(obuf));
    }

  {
    int fd;
    struct action *a2;
    fd = accessories[a->accessory].filedes;

  
    /* seek the new position */ 
    {
    off_t pos;
    off_t rf;
    rf = range_from;
    if (rf == (off_t)-1) {
      rf = 0;
      }
    pos = lseek(fd,rf,SEEK_SET);
    if (pos == (off_t)-1) {
      fprintf(stderr,"error seeking file. Oh well\n");
      }
    
    a2=actions+fd;
    if (a2->filedes!=fd) { /* new one */
      add_to_poll(fd, /* file des */
        NULL,  /* File alternate */
	READ_FROM_FILE_OR_STREAM, /* type */
	sd2,  /* parent */
	NULL, /* client socket */
	NULL, /* server socket */
        POLLIN|POLLPRI|POLLERR|POLLHUP, /* event flags */
	NULL,0,NULL,0,
	accessory_id, /* accessory id */
	rf, /* range from */
	(long)(range_to) /* range to */);   
      a2->buf1=malloc(READ_COUNT);
      a2->buf1_length=READ_COUNT;
      a2->buf2=malloc(READ_COUNT);
      a2->buf2_length=READ_COUNT;
      a2->mode = 0;
    
      }
    }
    
    
  {
    read_me(a2);
    actions[sd2].reader_fd=fd;
    
    change_state_to_send_web_header_and_data(sd2);

    }
  }


}


}

int fds_restruct=0;







int close_poll(int fd) {
struct action *a;
int reader_fd;

a=actions+fd;

/* take out of our file descriptor polling list */
if (no_fds==1) {
  no_fds =0;
  }
else {
  fds[a->poll_position]=fds[no_fds-1];
  /* Move the last poll position into the current spot */
  actions[fds[no_fds-1].fd].poll_position=a->poll_position;
  /* That top position poll position now points to the poll position we just had */
  /* so this moves an action */
  no_fds--;
  }
fds_restruct+=1; /* need to restructure the file descriptors -- after we are done going through them all */


/* handle the accessory -- easy for now.  Maybee later we hold
the file open or something */
{
int acc = a->accessory;
if (acc != -1) {
  accessories[acc].filedes=0;
  }
}


/* clean up the action */
{
  reader_fd = a->reader_fd;
  close(fd);
  a->type=0;
/*?  if (a->client_sap) free(a->client_sap);*/
  a->filedes=0;
  a->position=0;
  a->end_position=0;
  a->parentfd=0;
  a->client_sap=NULL;
  a->server_sap=NULL;
/*  if (a->buf1) free (a->buf1);*/
  a->buf1=NULL;
  a->buf1_length=0;
/*  if (a->buf2) free (a->buf2);*/
  a->buf2=NULL;
  a->buf2_length=0;
  a->reader_fd=0;
  a->accessory=0;
  }

if (reader_fd) {
  close_poll(reader_fd);
  }

}






      
	          
int handle_web_event_01_accept(struct action *a) {	          
  socklen_t accept_length;
  struct sockaddr_in *pclient_sa;
  int n;
  char buf[100001];
  int sd2;
  int s;
  
  
  accept_length = sizeof(struct sockaddr_in);
  pclient_sa = malloc(accept_length);
  s = a->filedes;
  sd2 = accept(s,(struct sockaddr *)pclient_sa,&accept_length);
  if (sd2<0) {
    fprintf(stderr,"accept failed %d\n",errno);
    
    exit(-1);
    }
  add_to_poll(sd2,
              NULL,
	      ACCEPT_INCOMING_WEB_REQUEST,
	      a->filedes,
	      pclient_sa,
              a->server_sap, 
	      POLLIN|POLLPRI|POLLERR|POLLHUP|POLLNVAL,
	      NULL,0,NULL,0,
	      -1,
	      0l,
	      -1l);   

}

								  
											  


/* Add to the buffer - if it has anything! 
pointer to buffer,  pointer to length, new buffer, new length 
Assumes that if there is a buffer,  it was malloced so we can free it.
*/
int add_buffer(char **b, int *pl,char *n, int nl) {
if (!(*b)) {
  *b = malloc(sizeof(char)*(nl+1));
  strncpy(*b,n,nl);
  *pl = nl;
  }
else { 
  char *r;
  r = malloc(sizeof(char)*(*pl + nl +2));
  strncpy(r,*b,*pl);
  strncpy(r+*pl,n,nl);
  *pl += nl;
  free(*b);
  *b = r;
  (*b)[*pl]='\0';
  }
}


int is_complete_request (char *r,int l) {
int i;
for (i=l-1;i>1;i--) {
  if (((i>3)&&(r[i]=='\n')&&
             (r[i-1]=='\r')&&
	     (r[i-2]=='\n')&&
	     (r[i-3]=='\r'))
   ||((r[i]=='\n')&&(r[i-1]=='\n'))) return(1);
  }
return(0);
}



/* this is what we get when we have a web request starting */
int handle_web_event_02_receive(struct action *a) {	          
  struct sockaddr_in client_sa;
  int n;
  char buf[100001];
  int sd2;
  sd2=a->filedes;
  n = recv(sd2,buf,100000,0);
  if (n<0) {
    fprintf(stderr,"error receiving\n");
    n=0;
    }
  buf[n]='\0';
  
  if ((a->buf1)&&(a->buf1[0]=='\0')&&buf[0]=='\0') 
      {
      /* this is a bogus action - ignore it */

      close_poll(a->filedes);
      }
      
  add_buffer(&(a->buf1),&(a->buf1_length),buf,n);
  
  /* see if the request is done */
  if (is_complete_request(a->buf1,a->buf1_length)) {
    new_request(a,a->parentfd,sd2,a->server_sap,&client_sa,buf);
    
    }
  
}







int handle_send_out_web_header_01(struct action *a) {	          
  socklen_t accept_length;
  struct sockaddr_in *pclient_sa;
  int n;
  char buf[100001];
  int sd2;
  int check;
  
  
  check=send(a->filedes,a->buf2,((size_t)(((long)a->buf2_length)-a->position)),
    MSG_DONTWAIT|MSG_NOSIGNAL);
  if (check==-1) {
    fprintf(stderr,"error sendingb : %d\n",errno);
    exit(-1);
    }
    
  a->position+= check;
  if ((a->position>=a->buf2_length)
     || ( (errno != EAGAIN)&&(errno != EWOULDBLOCK)) ) {
    close_poll(a->filedes);
    }
}




extern int handle_flow (struct action *write_a,struct action *read_a);







int handle_send_out_web_header_and_data_01(struct action *a) {	          
  socklen_t accept_length;
  struct sockaddr_in *pclient_sa;
  int n;
  char buf[100001];
  int sd2;
  int check;
  
  
  check=send(a->filedes,a->buf2,((size_t)(((long)a->buf2_length)-a->position)),
    MSG_DONTWAIT|MSG_NOSIGNAL);
  if (check==-1) {
    fprintf(stderr,"error sendinga : %d\n",errno);
    exit(-1);
    }
    
  a->position+= check;
  if ((a->position>=a->buf2_length)
     || ( (errno != EAGAIN)&&(errno != EWOULDBLOCK)) ) {
    a->type = WRITE_DATA_FROM_STREAM;
    a->mode=1;


    handle_flow(a,actions+a->reader_fd);
    }
}








/* 2AIcy info:
echo "GET /listen.pls HTTP/1.0am.aol.com 80
Icy-MetaData:1
Accept: * /*
"  netcat 205.188.215.229 8012


HTTP/1.0 200 OK
content-type:audio/x-scpls
Connection: close

[playlist]
NumberOfEntries=1
File1=http://:8012/





echo "GET :8012 HTTP/1.0
Icy-MetaData:1
Accept: * /*
" | netcat 205.188.215.229 8012


HTTP/1.0 302 Found
Content-type:text/html
Location: http://scfire-ntc-aa02.stream.aol.com:80/stream/1050

<HTML><HEAD><TITLE>Redirect</TITLE></HEAD><BODY>Click <a href="http://scfire-ntc-aa02.stream.aol.com:80/stream/1050">HERE</A> for redirect.</BODY></HTML>hib@lov

echo "GET /stream/1050 HTTP/1.0
Icy-MetaData:1
Accept: * /*
" | netcat scfire-ntc-aa03.stream.aol.com 80




ICY 200 OK
icy-notice1: <BR>This stream requires <a href="http://www.winamp.com/">Winamp</a><BR>
icy-notice2: Firehose Ultravox/SHOUTcast Relay Server/Linux v2.6.0<BR>
icy-name: P h i l o s o m a t i k a - 100% Psychedelic Trance - Get your stomp on!
icy-genre: Psytrance Trance Electronic
icy-url: http://www.philosomatika.com
content-type: audio/mpeg
icy-pub: 1
icy-metaint: 16384
icy-br: 128




http://205.188.215.229:8012/listen.pls 
GET $path HTTP/1.0\n"
     append buff "Host: $server\n"
     append buff "Icy-MetaData:1\n"
     append buff "Accept: * /*\n"
     append buff "User-Agent: Tcl/8.4.9\n"
     append buff "\n"
     puts $sock $buff
     flush $sock
     set title "Connected to $server."
*/






int handle_flow (struct action *write_a,struct action *read_a) {
if ((write_a->mode==1)||(write_a->mode==0)) /* ready to write out something */ 
  {
  if (read_a->mode==2) /* read something and full buffer */
    {
    int cnt;
    cnt = send(write_a->filedes,read_a->buf1,read_a->buf1_length,MSG_DONTWAIT|MSG_NOSIGNAL); /* send it out */
    if (cnt==-1) {fprintf(stderr,"error sending out to socket %d: %d\n",write_a->filedes,
    errno);}
    fds[write_a->poll_position].events |= (POLLOUT);
    
    
    { /* swap buf1 and buf2 */
      char *temp;
      int t2;
      temp = read_a->buf2;
     read_a->buf2 = read_a->buf1;
     read_a->buf1=temp;
     t2=read_a->buf2_length;
     read_a->buf2_length=read_a->buf1_length;
     read_a->buf1_length=t2;
     }

    
    write_a->mode=2; /* are writing things out now */
    
    read_me(read_a);
    }
  else if (read_a->mode==1) /* still reading */ {
    /* nothing right now.  Have to wait until the read is done */
    fds[write_a->poll_position].events &= ~(POLLOUT);
    }
  else if (read_a->mode==0) /* haven't started reading */ {
    read_me(read_a);
    } /* if need to read */
  else if (read_a->mode==3) {
    close_poll(write_a->filedes);
    }
  } /* if we are ready to write some more */
else if ((write_a->mode==2)) { /* writing out something */ 
  if (read_a->mode==0) /* havent started reading */ {
    read_me(read_a);
    } /* if need to read */
  else if (read_a->mode==2) /* read is finished,  waiting for write */ {
    /* We can stop looking for read completed information for the read file descriptor,  until we read again */
    fds[read_a->poll_position].events &= ~(POLLIN|POLLPRI);
    /* of coarse, we will have to turn these pollings back on after we write something out */
    }
  } /* if writing out something */
}




void handle_hangup(struct action *a,int is_error_flag) {
if (is_error_flag) {
  /*fprintf(stderr,"ERROR CLOSE\n");*/
  }
  close_poll(a->filedes);
/* should be the sending output that usually is errored with this */


}


								  
/* ACTION_WEB_LKISTENER - 
    ACCEPT_INCOMING_WEB_REQUEST -
    
    */
int handle_event(struct pollfd *e) {
struct action *a;
short revents;
a=actions+e->fd;
revents=e->revents;
/*fprintf(stderr,"r %d type %s\n",revents,event_type_string[a->type]);*/
if (a->type==ACTION_WEB_LISTENER) {
  if (revents&(POLLIN|POLLPRI)) {
    handle_web_event_01_accept(a);
    }
  else {
    fprintf(stderr,"undef ACTION_WEB_LISTENER type %d event %d\n",(int)(a->type),(int)(revents));
    }
  }
else if (a->type==ACCEPT_INCOMING_WEB_REQUEST) {
  if (revents&(POLLIN|POLLPRI)) {
    handle_web_event_02_receive(a);
    }
  else {
    fprintf(stderr,"undef ACCEPT_INCOMING_WEB_REQUEST type %d event %d\n",(int)(a->type),(int)(revents));
    }    
  }
else if (a->type==SEND_OUT_WEB_HEADER) {
  if (revents&(POLLOUT)) {
    handle_send_out_web_header_01(a);
    }
  else {
    fprintf(stderr,"undef SEND_OUT_WEB_HEADER type %d event %d\n",(int)(a->type),(int)(revents));
    }    
  }
else if (a->type==SEND_OUT_WEB_HEADER_AND_DATA) {
  if (revents&(POLLOUT)) {
    handle_send_out_web_header_and_data_01(a); 
    }
  else {
    fprintf(stderr,"undef SEND_HEADER_AND_DATA type %d event %d\n",(int)(a->type),(int)(revents));
    }    
  }
else if (a->type==READ_FROM_FILE_OR_STREAM) {
  if (revents&(POLLIN|POLLPRI)) {
    a->mode=2;
    handle_flow(actions+a->parentfd,a);
    }
  else {
    fprintf(stderr,"undef READ_FROM_FILE_OR_STREAM type %d event %d\n",(int)(a->type),(int)(revents));
    }    
  }
else if (a->type==WRITE_DATA_FROM_STREAM) {
  if (revents&(POLLOUT)) {
    a->mode=1;
    handle_flow(a,actions+a->reader_fd); 
    }
  else if (revents & POLLHUP) {
    /*24 is POLLERR + POLLHUP */
    int err = (revents& POLLERR);
    handle_hangup(a,err);
    }
  else {
    fprintf(stderr,"undef WRITE_DATA_FROM_STREAM type %d event %d\n",(int)(a->type),(int)(revents));
    }    
  }
else {
  fprintf(stderr,"undef type %d event %d\n",(int)(a->type),(int)(revents));
  }
}











int main() {
/* open the socket*/

int s;
int check;
struct sockaddr_in server_sa;

init_external_call();

s = socket(AF_INET,SOCK_STREAM,0);

if (s<0) {
  fprintf(stderr,"socket failed\n");
  }
  
server_sa.sin_family = AF_INET;
server_sa.sin_port = htons(8889);
server_sa.sin_addr.s_addr = INADDR_ANY;
{
  int on=1;
  check= setsockopt(s, SOL_SOCKET,
            SO_REUSEADDR,(char *) &on, sizeof(on));
  if (check) {	    
    fprintf(stderr,"setsockopt failed : %d\n",errno);
    exit(-1);	    
    }
  }
  

{ /* This will make the response faster because of the nagle algorithm */
  int flag, ret;
  /* Disable the Nagle (TCP No Delay) algorithm */

  flag = 1;
  ret = setsockopt( s, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag) );
  if (ret == -1) {
    printf("Couldn't setsockopt(TCP_NODELAY)\n");
    exit( -1 );
    }
}


check=bind(s,(struct sockaddr *)&server_sa,sizeof(server_sa));
if (check== -1) {
  fprintf(stderr,"bind failed ");
  if (errno==EACCES) fprintf(
      stderr,"EACCES - address protected not superuser");
  else if (errno==EBADF) fprintf(stderr,"BADF - invalid descriptor");
  else if (errno==EINVAL) fprintf(stderr,"EINVAL - already bound");
  else if (errno==EADDRINUSE) fprintf(stderr,"EADDRINUSE - address in use");
  else
    fprintf(stderr,"%d",errno);
  fprintf(stderr,"\n");
  exit(-1);
  
  }

if (check=listen(s,10000)) {
  fprintf(stderr,"listen failed\n");
  }
no_fds=0;
add_to_poll(s,
           NULL,
	   ACTION_WEB_LISTENER,
	   0,
	   NULL,
	   &server_sa,
  POLLIN|POLLPRI|POLLOUT|POLLERR|POLLHUP|POLLNVAL,
  NULL,0,NULL,0,
  -1,0l,-1l); /* our main listener
     for dispatching :) */   
     


while (1) {  
  int no_events;
  int i;
  int number_fds_local;
  no_events=poll(fds,no_fds,1222222);
  i=0;

  number_fds_local=no_fds;
  while (no_events) {
    for (;i<number_fds_local;i++) {
      if (fds[i].revents) {
        handle_event(fds+i);
        if (fds_restruct) /* push a forward 1 if we took out some events */ {
          number_fds_local =- fds_restruct; /* less to do */
          fds_restruct=0;
          }
        else
          i++; /* go to the next one */
	break; /* good if we only see one set of events at a time */
	}
      } /* for going through all events */
    no_events--;
    } /* while */
  } /* forever */
exit(0);
}





#ifdef STANDALONEfdsfdsfds
int init_external_call() {}
char  *make_external_call(char *buf,char *url,int *buflength) {
char *x = strdup("HTTP/1.1 200 OK\nContent-Type: text/plain\n\n<b>hello");
*buflength= strlen(x);
return x;
}
#endif
