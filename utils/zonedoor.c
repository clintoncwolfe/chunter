/*
 * This utility provides a SSH door for SmartOS VM's
 * it is based on MerlinDMC's code for a always open
 * SSH door but instead of always allowing access it
 * prints the information to stdout and then reads
 * the response from STDIN (one line).
 *
 * usage: ./zonedoor <uuid> <service>
 * i.e.: ./zonedoor <uuid> _joyent_sshd_key_is_authorized
 * gets: /zones/<uuid>/root/var/tmp/._joyent_sshd_key_is_authorized
 */

#pragma ident "%Z%%M% %I% %E% SMI"

#include <alloca.h>
#include <zdoor.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HEARTBEAT 7  // number of seconds missing HB before exit


zdoor_result_t *server(zdoor_cookie_t *cookie, char *argp, size_t arpg_sz);
void addVMDoor(char * zoneID, char *doorName);
void rmVMDoor(char * zoneID, char *doorName);

zdoor_handle_t zdid;
int pendingRequest;
char requestResponse;


void addVMDoor(char *zoneID, char *doorName){
  if (zdoor_open(zdid, zoneID, doorName, zoneID, server) < 0){
    fprintf(stderr, "Error [zonedoor] opening door in zone %s.\r\n", zoneID);
    fprintf(stderr, "zdoor_open result: %i", zdoor_open(zdid, zoneID, "_joyent_sshd_key_is_authorized", zoneID, server));
    return;
  }
  //  printf("ok\n");  //no sure if responce is necessary. right now chunter_vm_auth does not handle it.
  //  fflush(stdout);
}

void deleteVMDoor(char *zoneID, char *doorName){
  zdoor_close(zdid, zoneID, doorName);
  // printf("ok\n");
  // fflush(stdout);

}

zdoor_result_t *server(zdoor_cookie_t *cookie, char *argp, size_t arpg_sz)
{
  zdoor_result_t *result;
  fprintf(stdout, "%s %s\n", cookie->zdc_biscuit, argp);
  fflush(stdout);
  pendingRequest = 1;
  char deny[] = "0";

  result = malloc(sizeof(zdoor_result_t));
  result->zdr_data = NULL;
  result->zdr_size = 1;

  int i = 0;
  while(i<20){
    if(pendingRequest == 0){
      result->zdr_data = &requestResponse;
      return result;
    }
    i++;
    nanosleep((struct timespec[]){{0, 100000000}}, NULL);
  }
  pendingRequest = 0;
  result->zdr_data = deny;
  return result;
}


void sigAlrmHandler(int sig) {
  exit(0);
}


int
main(int argc, char *argv[])
{

  signal(SIGALRM, sigAlrmHandler);
  alarm(HEARTBEAT);

  zdid = zdoor_handle_init();

  while(1){

    size_t nbytes = 200;
    char *input = NULL;
    char *arg2 = NULL;
    getline(&input, &nbytes, stdin);
    switch(input[0])
      {
      case 'h':    // heartbeat
        alarm(HEARTBEAT);
        break;
      case 'a':    // add zone door
        input++[strlen(input)-1]=0;
        arg2 = input;
        while (arg2 != ' ' && arg2 - input < nbytes) {
          arg2++;
        };
        if (arg2 == ' ') {
          *arg2 = 0;
          arg2++;
          // "_joyent_sshd_key_is_authorized"
          addVMDoor(input, arg2);
        } else {
          alert("invalid input");
        }
        break;
      case 'd':    // delete zone door
        input++[strlen(input)-1]=0;
        if (arg2 == ' ') {
          *arg2 = 0;
          arg2++;
          // "_joyent_sshd_key_is_authorized"
          deleteVMDoor(input, arg2);
        } else {
          alert("invalid input");
        }
        break;
      case 'r':   // request response
        requestResponse = input[1];
        pendingRequest = 0;
        break;
      default:
        break;
      }

    free(input);

  }
}
