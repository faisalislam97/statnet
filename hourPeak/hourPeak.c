#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#define EXTRACT_QUERY_SIZE 130
#define SAVE_QUERY_SIZE 120
				//				|
struct iface
{
	char *name;
	double rx;
	double tx;
};

struct iface *if_list;		//List of all interfaces
int if_count;		//number of interfaces

int f_Extract(void *,int,char **,char **);

void free_if_list()
{
	if (if_list != NULL)
	{
		for (int i = 0; i < if_count; i++)
		{
			if (if_list[i].name != NULL)
				free(if_list[i].name);
		}
		free(if_list);
	}

}


int main(int argc,char *argv[])
{


//	if (argc < 2)
//	{
//		printf("No time?\n");
//		return 1;
//	}
	int query_check;	//Used for various query-checks
	sqlite3 *db;		//Pointer to the database
	char *sql;		/* It will store the executable
				query */
	FILE *fp;
	time_t raw_time;
	struct tm *obtained_time;

	char date[12];
	char logName[26];

	char hour[3];
	int hour_i; 		//to reduce the hour obtained by one
	/* Obtaining time and applying it to required places */
	time(&raw_time);
	obtained_time = localtime(&raw_time);
	strftime(date,sizeof(date),"%Y-%m-%d",obtained_time);
	strftime(logName,sizeof(logName),"/etc/stats/%Y%m%d_h.log",obtained_time);
	strftime(hour,sizeof(hour),"%H",obtained_time);

	hour_i = atoi(hour);
	hour_i--;
	hour_i = snprintf(hour,sizeof(hour),"%i",hour_i);
	if (hour_i == 0)
	{
		sqlite3_close(db);
		return 1;
	}

	if_count = 0;

	if_list = calloc(1,sizeof(struct iface));

	/* Opening database connection */
	query_check = SQLITE_BUSY;
	while(query_check == SQLITE_BUSY)
		query_check = sqlite3_open("/etc/fuse-db.db",&db);
	if (query_check != SQLITE_OK)
	{
		sqlite3_close(db);
		db = NULL;
		free_if_list();
		return 1;
	}
	/* Forming query for obtaining the peaks */
	char extractQuery[]="SELECT Name,max(rx),max(tx)FROM bandwidths WHERE Date = '%s' AND Time LIKE \'%s%%\' GROUP BY Name HAVING Time LIKE \'%s%%\';";
	sql = calloc(EXTRACT_QUERY_SIZE,sizeof(char));
	query_check = snprintf(sql,EXTRACT_QUERY_SIZE,extractQuery,date,hour,hour);
	if (query_check == 0)
	{
		printf("Query not formed\n");
		free_if_list();
		return 1;
	}
		printf("%s\n",sql);

	/* Obtaining the peaks via query */
	query_check = SQLITE_BUSY;
	while(query_check == SQLITE_BUSY)
		query_check = sqlite3_exec(db,sql,f_Extract,&if_count,0);
	if (query_check != SQLITE_OK)
	{
		printf("%s\n",sql);
		free(sql);
		printf("%s\n",sqlite3_errmsg(db));
		sqlite3_close(db);
		free_if_list();
		return 1;
	}
	free(sql);

	/* Opening the log file for saving peaks */

	fp =fopen(logName,"a");
	if (fp == NULL)
	{
		printf("not opening file\n");
		sqlite3_close(db);
		free_if_list();
		return 1;
	}

	/* Saving the peaks in log file */
	for (int i = 0; i < if_count; i++)
	{
		fprintf(fp,"%s,%s,%.2f,%.2f\n",hour,if_list[i].name,if_list[i].rx,if_list[i].tx);
	}
	fclose(fp);

	for (int i = 0; i < if_count; i++)
	{
		/* Creating query for saving peaks in the database */
		char saveQuery[] = "INSERT INTO hourPeak(Date,Time,Name,rx,tx)VALUES(\'%s\',\'%s\',\'%s\',%.2f,%.2f);";
		sql = calloc(SAVE_QUERY_SIZE,sizeof(char));
		query_check = snprintf(sql,SAVE_QUERY_SIZE,saveQuery,hour,if_list[i].name,if_list[i].rx,if_list[i].tx);
		if (query_check == 0)
		{
			printf("Not forming save query\n");
			sqlite3_close(db);
			free_if_list();
			return 1;
		}
		printf("%s\n",sql);

		/* Saving the peaks into database via query*/
		query_check = SQLITE_BUSY;
		while (query_check == SQLITE_BUSY)
			query_check = sqlite3_exec(db,sql,0,0,0);
		if (query_check != SQLITE_OK)
		{
			printf("%s\n",sql);
			free(sql);
			printf("%s\n",sqlite3_errmsg(db));
			sqlite3_close(db);
			free_if_list();
			return 1;
		}

		free(sql);
	}

/* End of program */
	free_if_list();

	sqlite3_close(db);
	return 0;
}

int f_Extract(void *i_p,int argc,char **argv, char **unused)
{
	int i =*(int *)i_p;

	if_list = realloc(if_list,(i+1)*sizeof(struct iface));

	if_list[i].name = calloc(31,sizeof(char));
	if_list[i].name = strdup(argv[0]);

	if_list[i].rx   = atof(argv[1]);

	if_list[i].tx   = atof(argv[2]);

	if_count++;
	return 0;
}
