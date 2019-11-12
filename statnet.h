//Header file for statnet
#ifndef NETSTAT_H
#define NETSTAT_H
#include "sqlite3.h"

void Demonize();

extern struct timeval beginning,ending;//for updating times

extern int first_delay;//this will allow us to keep first delay's behavior different from the rest

extern int interfaces;
extern char **interface_names;//dynamically stores name of interface

extern int offsets_count; //stores the amount of offsets stored so far

extern int INTERRUPT_DETECTED;//this will become 1 on detection of interrupt from the user

typedef unsigned long ulong;

typedef struct offset_struct {
        int if_ID;
        ulong in;
        ulong out;
        ulong total;
        } offset;

extern offset *offsets_list;	/*this list will store all available offsets for the current instannce
				of statnet*/

extern unsigned long *old_bytes_in; //storing bytes in for first time
extern unsigned long *old_bytes_out; //storing bytes out for first time


extern unsigned long *new_bytes_in;//storing the bytes in for second time
extern unsigned long *new_bytes_out;//storing the bytes out for second time


extern double kbps_in;		//declared as such in order to append this data to the log file
extern double kbps_out;		//declared as such in order to append this data to the log file

extern sqlite3 *db;			//variable that holds the location of the database


//functions:

/*calculates bandwidth in kbps*/
double Bandwidth(const unsigned long old_bytes,const unsigned long new_bytes,struct timeval *beginning,struct timeval*ending,const int type);

/*Sets filename for storing bandwidth*/
char *SetFileName(char *interface_name,const int type);

/*frees memory inside fileParser function*/
void FreeMemory(unsigned long *in,unsigned long *out,FILE *fp);

/*frees memory in the main program*/
void FreeMemory2();

/*parses file to fetch bytes in/out*/
int FileParser(const char *file,const int set);

/*saves bandwidths in files at /tmp/ */
void SaveBandwidth(const int i,const int type);

/*saves bandwidths permanently in a log file*/
void AppendLog(const int i);

/*saves bandwidths pseudo-permanently in the database, for peak calculations*/
void SaveBandwidthSQLite(const int i);

/*handles interrupt given by the user. Ensures that memory is freeed before program exits*/
void InterruptHandler(int signum);

/*Reporter Function of SQLite. Not endorsed nor used by SQLite*/
void SQLReport(const int,const char *,const signed int);
/*saves tx, rx and total bytes of currently available interfaces in an SQLite database*/
int SaveBytes(int i);
	/*Helper Functions for SaveBytes()*/
		int findOffset(const int);
		int ifaceCheck(int,int *);//checks for existence of interface in the database
			int fresult_checkif(void *,int,char**,char**);
		int ifaceUpdate(int,int);//updates record of interface
		int ifaceInsert(int);//inserts record of interace
			int fresult_offsetID(void *,int,char**,char**);
			void AddOffset(int,ulong *,ulong *,ulong *);
/*Loads offsets from the database*/
void LoadOffsets(void);
	/*Helper function for LoadOffsets*/
		int fresult_loadoffsets(void *, int,char **,char **);//transcribes results of query to offsets_list
#endif
