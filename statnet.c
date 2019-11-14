//This will store bandwidths in separate files
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include "statnet.h"
#include "sqlite3.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFF_SIZE 600
#define FAILURE 0
#define NAME_LENGTH 31
#define OLD 2
#define NEW 3
#define IN 4
#define OUT 5

#define NO_STATLESS_IFS
//$define DEBUGMODE




struct timeval beginning,ending;//for updating times
struct timeval delay;//this stores the delay put by user. By default, it is 1 second

//struct timeval start,end;


int interfaces;//keeps track of interfaces
char **interface_names;//dynamically stores names of interfaces


unsigned long *old_bytes_in; //storing bytes in for first time
unsigned long *old_bytes_out; //storing bytes out for first time


unsigned long *new_bytes_in;//storing the bytes in for second time
unsigned long *new_bytes_out;//storing the bytes out for second time

int offsets_count;

offset *offsets_list;        //Storing the list of all available offsets for this instance of statnet


int first_delay;//this will keep file parsing different for the first time

double kbps_in;//just declaring here
double kbps_out;//just declaring here

sqlite3 *db; //the SQLite Database


int main(int argc,char *argv[])
{

//	Demonize();

	int db_open = sqlite3_open("/etc/fuse-db.db",&db);
	if (db_open != SQLITE_OK)
	{
		sqlite3_close(db);
		exit(EXIT_FAILURE);
	}
/*setting up interrupt signal's mask. This will prevent the signal from stopping the program until the loop
is completely executed*/
	sigset_t intmask;//interrupt's mask
	sigemptyset(&intmask);//setting up the mask
	sigaddset(&intmask, SIGINT);//adding interrupt signal to its mask


/*variables declaration*/
	char datafilename[]="/proc/net/dev";//filename of the file that contains latest data of interfaces

	int success_old;//number of interfaces processed. Used as a flag of success too
	int success_new;//number of interfaces processed. Used as a flag of success too
	int delay_arg;//counts arguments
	delay_arg = 2;/*first argument of a program is its filename, so we are reading the second argument
			which is command line's first argument*/

/*sets delay to either user input or to custom*/
	if (argc == delay_arg)
	{
		delay.tv_sec = (int)atof(argv[1]);
		delay.tv_usec=(int) (( atof(argv[1])-(double)delay.tv_sec  )*( 1000000 ));

	}
	else
	{
		delay.tv_sec = 1;
		delay.tv_usec = 0;
	}
first_delay = 1;
signal(SIGINT,InterruptHandler);//to make sure the memory is freed before the program exits
offsets_count = 0;
/*Loading available offsets for the interfaces from the datatbase*/
LoadOffsets();

do
{
//gettimeofday(&start,NULL);
sigprocmask(SIG_BLOCK,&intmask,NULL);//this blocks the signal, so that the interruption does not mess with the program

/*file parsing to obtain initial bytes*/
	success_old = FileParser(datafilename,OLD);

/*waiting for the amount of delay specified in the beginning*/
	select(0,NULL,NULL,NULL,&delay);
	if (argc == delay_arg)//required to reset the delay, as select function modifies its values to zero
	{
		delay.tv_sec = (int)atof(argv[1]);
		delay.tv_usec=(int) (( atof(argv[1])-(double)delay.tv_sec  )*( 1000000 ));
	}
	else
	{
		delay.tv_sec = 1;
		delay.tv_usec = 0;
	}
/*file parsing to obtain final bytes*/
	success_new = FileParser(datafilename,NEW);

/*checks whether the first time's delay flag is up*/
	if (first_delay == 1)
	{
		first_delay = 0;//setting it down for subsequent delays
	}
/*error reporting if failure in obtaining initial bytes*/
	if (success_old == FAILURE)
	{
#ifdef DEBUGMODE
		printf("Error in extracting old dataset\n");
#endif
		continue;
	}
/*error reporting if failure in obtaining final bytes*/
	if (success_new == FAILURE)
	{
#ifdef DEBUGMODE
		printf("Error in extracting new dataset\n");
#endif
		FreeMemory2(interface_names,old_bytes_in,old_bytes_out,new_bytes_in,new_bytes_out);
		continue;
	}

//Ensuring that least of the processed interfaces are shown. This is safer than modifying the dynamically allocated arrays
if(success_old<=success_new)
	interfaces = success_old;
else
	interfaces = success_new;
/*file manipulation for each interface recorded*/
	for (int i = 0; i < interfaces; i++)
	{
#ifdef NO_STATLESS_IFS
		if (new_bytes_in[i] == 0 && new_bytes_out[i] == 0)//prevents statless interfaces from appearning
			continue;
#endif
		SaveBandwidth(i,IN);
		SaveBandwidth(i,OUT);
		AppendLog(i);
		SaveBytes(i);
		SaveBandwidthSQLite(i);
	}
//	SaveBytes();
sigprocmask(SIG_UNBLOCK,&intmask,NULL);
/*gettimeofday(&end,NULL);
{
	int seconds = end.tv_sec - start.tv_sec;
	double exe_time = (double)(seconds) + (double)(end.tv_usec - start.tv_usec)/(double)(1e6);

	printf("-----Time elapsed in function: %.3f seconds-----",exe_time);
}*/
}
while(1);
	FreeMemory2();
	return 0;
}
