#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

#include "sqlite3.h"

#include "statnet.h"

#define BUFF_SIZE 300
#define SUCCESS 7
#define FAILURE 0
#define NAME_LENGTH 31
#define OLD 2
#define NEW 3
#define IN 4
#define OUT 5
#define INSERT_IFACE_SIZE 130
#define CHECK_IFACE_SIZE 142
#define UPDATE_IFACE_SIZE 128
#define OFFSET_QUERY_SIZE 76
#define SAVE_QUERY_SIZE 123
//#define DEBUGMODE
//#define SAVE_BYTES_LOG


//sqlite3 *db;			//Variable that holds our database

//FILE *sqlReport;		//Pointer to where reports pertaining to SQLite are stored

int result_ifcheck;		//result of checkif in a global variable

void InterruptHandler(int signum)
{
        FreeMemory2();
#ifdef DEBUGMODE
	printf("Statnet Stopped");
#endif
	sqlite3_close(db);
	exit(1);
}

void AppendLog(const int i)
{
	/*Declaration of variables*/
	FILE *log_pointer;

	char log_name[18];
	char cur_time[9];

	time_t raw_time;
	struct tm *obtained_time;


	/*Filename creation*/
	time(&raw_time);
	obtained_time = localtime(&raw_time);
	strftime(log_name,sizeof(log_name),"/etc/%Y%m%d.log",obtained_time);



	/*Opening file*/
	if ( (   log_pointer=fopen(   log_name,"a"   )  ) != NULL )
	{
		time(&raw_time);
		obtained_time = localtime(&raw_time);
		strftime(cur_time,sizeof(cur_time),"%X",obtained_time);
#ifdef SAVE_BYTES_LOG
		fprintf(log_pointer,"%s,%s,%.2f,%.2f,%lu,%lu\n",cur_time,interface_names[i],kbps_in,kbps_out,new_bytes_in[i],new_bytes_out[i]);
#else
		fprintf(log_pointer,"%s,%s,%.2f,%.2f\n",cur_time,interface_names[i],kbps_in,kbps_out);
#endif
		fclose(log_pointer);
		log_pointer = NULL;
	}

}

void SaveBandwidthSQLite(const int i)
{
	char *sql;
	char date[11];
	char cur_time[9];
	time_t raw_time;
	struct tm *obtained_time;
	int query_check;

	time(&raw_time);
	obtained_time = localtime(&raw_time);

	strftime(date,sizeof(date),"%Y-%m-%d",obtained_time);//date obtained
	strftime(cur_time,sizeof(cur_time),"%X",obtained_time);//time obtained

	char queryForm[]="INSERT INTO hourly(Date,Time,Name,rx,tx)VALUES(\'%s\',\'%s\',\'%s\',%.2f,%.2f);";

	sql = calloc(SAVE_QUERY_SIZE,sizeof(char));

	query_check = snprintf(sql,SAVE_QUERY_SIZE,queryForm,date,cur_time,interface_names[i],kbps_in,kbps_out);
	if (query_check == 0)
	{
		//SQLReport4,__FUNCTION__,__LINE__);
		free(sql); sql = NULL;
		return;
	}

	sql = realloc(sql,strlen(sql)+1);

	query_check = SQLITE_BUSY;
	while (query_check == SQLITE_BUSY)
		query_check = sqlite3_exec(db,sql,0,0,0);
	if (query_check != SQLITE_OK)
	{
		free(sql);
		sql = NULL;
		return;
	}
	free(sql);
	sql = NULL;
}


/*void SQLReport(const int i, const char *function_name,const signed int line)
{

	sqlReport = fopen("/tmp/sqlite.log","a");
	if (sqlReport != NULL)
	{

		switch(i){

			case 0:
				fprintf(sqlReport,"The program works till line %d of %s file in %s function\n",line,__FILE__,function_name);
			break;

			case 1://failure to open
				fprintf(sqlReport,"In function %s at line %d, database failed to open.\n",function_name,line);
			break;

			case 2://failure to execute the query
				fprintf(sqlReport,"In function %s at line %d, query failed to execute.\n",function_name,line);
				sqlite3_close(db);
			break;

			case 3://list of successfully obtained offsets
				fprintf(sqlReport,"The IDs of interfaces for whom offsets were obtained:\n");
				if (offsets_count > 0)
				{
					for (int i = 0; i < offsets_count ; i++)
						fprintf(sqlReport,"%i\n",offsets_list[i].if_ID);
				}
				else
					fprintf(sqlReport,"(null)\n");
			break;

			case 4://Failure to successfully generate the query
				fprintf(sqlReport,"In function %s at line %d, program failed to succesfully generate the query.\n",function_name,line);
			break;

			case 5://Successfully checking existence of interface in the database
				fprintf(sqlReport,"For %s, the program found its existence to be : %d\n",function_name,line);//abuse of variables
			break;
			}
		fclose(sqlReport); sqlReport = NULL;

	}

}
*/

void LoadOffsets()
{
	int queryCheck; //Check used for all sqlite functions

	static char queryFormat[] = "SELECT IfaceID,rx,tx,tt FROM interfaces";

	queryCheck = SQLITE_BUSY;
	while (queryCheck == SQLITE_BUSY)
		queryCheck = sqlite3_exec(db,queryFormat,fresult_loadoffsets,0,0);

	if (queryCheck != SQLITE_OK )
	{
//		SQLReport(2,__FUNCTION__,__LINE__);
		return;
	}

/*Informs us as to how many offsets were obtained from the database, in form of a list of IDs*/
/*	if (offsets_count > 0)
		SQLReport(3,"",0);
	return;
*/
}

int fresult_loadoffsets(void *unused, int argc, char **argv, char **unused2)
{
	//SQLReport(0,__FUNCTION__,__LINE__);//tells that program works till here
	unused = 0; unused2 = NULL;

	offsets_list = realloc(offsets_list,(offsets_count+1)*sizeof(offset));

	offsets_list[offsets_count].if_ID 	= atoi(argv[0]);
	//SQLReport(0,argv[0],__LINE__);//tells that program works till here
	offsets_list[offsets_count].in		= atol(argv[1]);
	//SQLReport(0,argv[1],__LINE__);//tells that program works till here
	offsets_list[offsets_count].out		= atol(argv[2]);
	//SQLReport(0,argv[2],__LINE__);//tells that program works till here
//	printf("%lu",offsets_list[offsets_count].total);
	offsets_list[offsets_count].total = atol(argv[3]);
	//SQLReport(0,argv[3],__LINE__);//tells that program works till here

	offsets_count++;
	return 0;
}

int SaveBytes(int i)
{

	/*Checks for success of steps in saving bytes in the database*/
	int ifaceExists; 	//check for if an interface's record exists in the database
	int updateIface; 	//check for successful update of the interface's record
	int insertIface; 	//check for successful insertion of the interface's record
	int result_checkif;	/*contains the result of checking the interface, which is
				  its ID in the database*/

	/*Step 1: Checking if the interface record exists in database*/
	//SQLReport(0,__FUNCTION__,__LINE__);//tells that program works till here
	ifaceExists = ifaceCheck(i,&result_checkif);

	if (ifaceExists == FAILURE)
	{
		//SQLReport function has been already called within ifaceCheck which more clearly presents errors
		//SQLReport(0,__FUNCTION__,__LINE__);//tells that program works till here
		return FAILURE;
	}

	/*Step 2: Updating record if interface exists, else inserting new record*/
	if (result_checkif != 0)
	{	/*Update*/
		int offset_index;
		offset_index = findOffset(result_checkif);
		//SQLReport(0,__FUNCTION__,__LINE__);//tells that program works till here
		if (offset_index == offsets_count)
			return FAILURE;
		updateIface = ifaceUpdate(i,offset_index);
		//SQLReport(0,__FUNCTION__,__LINE__);//tells that program works till here
		if (updateIface == FAILURE)
			return FAILURE;
	}
	else
	{	/*Insertion*/
		insertIface = ifaceInsert(i);
		//SQLReport(0,__FUNCTION__,__LINE__);//tells that program works till here
		if (insertIface == FAILURE)
			return FAILURE;
	}
	return FAILURE;
}

int findOffset(const int ID)
{
	int index; //index of offset list where the offsets lie

	for (index = 0;index < offsets_count;index++)
		if (offsets_list[index].if_ID == ID)
			break;
	return index;
}


int ifaceCheck(int i,int *result_checkif)
{

	static char queryForm[] = "SELECT MAX(CASE WHEN (interfaces.Name=\'%s\')THEN interfaces.IfaceID ELSE 0 END)x FROM interfaces";
	char *sql;//the actual query will be formed here
	sql = calloc(CHECK_IFACE_SIZE,sizeof(char));//initialized with size of maximum possible size of this query

	int queryCheck; //checks if the query was successfully made
	queryCheck = snprintf(sql,CHECK_IFACE_SIZE,queryForm,interface_names[i]);
	if (queryCheck == 0)
	{
		//SQLReport4,__FUNCTION__,__LINE__);
		free(sql); sql = NULL;
		return FAILURE;
	}
	sql = realloc(sql,strlen(sql));

	queryCheck = SQLITE_BUSY;
	while (queryCheck == SQLITE_BUSY)
		queryCheck = sqlite3_exec(db,sql,fresult_checkif,result_checkif,NULL);

	free(sql); sql = NULL;

	if (queryCheck != SQLITE_OK)
	{
		//SQLReport2,__FUNCTION__,__LINE__);
		return FAILURE;
	}
	else
	{
		//SQLReport5,interface_names[i],*result_checkif);
		return SUCCESS;
	}
}

int fresult_checkif(void *result_checkif,int argc, char **argv,char **NotUsed2)
{
	////SQLReport0,__FUNCTION__,__LINE__);
	NotUsed2 = NULL;
	char result[7]; //result holder

	snprintf(result,sizeof(result),"%s",argv[0]);
	//SQLReport(0,result,__LINE__);
	if (strcmp(result,"(null)") == 0)
		*(int *)result_checkif = 0;
	else
		*(int *)result_checkif = atoi(argv[0]);
	return 0;
}

int ifaceUpdate(int i, int offset_index)
{

	char *name;//name of the interface
	name = interface_names[i];

	unsigned long bytes_in;//incoming bytes
	unsigned long bytes_out;//outgoing bytes
	unsigned long bytes_total;//total bytes

	/*Setting their values*/
	bytes_in	 = new_bytes_in[i] + offsets_list[offset_index].in;
	bytes_out	 = new_bytes_out[i] + offsets_list[offset_index].out;
	bytes_total	 = new_bytes_in[i] + new_bytes_out[i];
	bytes_total	+= offsets_list[offset_index].total;

	/*Format of the query*/
	static char queryFormat[]="UPDATE interfaces SET RX=%lu,TX=%lu,TT=%lu WHERE Name=\'%s\';";

	char *sql; //where the program will form the actual query
	sql = calloc(UPDATE_IFACE_SIZE,sizeof(char));

	/*Query Formation*/
	int queryCheck;//check for successful formation of query
	queryCheck = snprintf(sql,UPDATE_IFACE_SIZE,queryFormat,bytes_in,bytes_out,bytes_total,name);
	if (queryCheck == 0)
	{
		free(sql); sql = NULL;
		return FAILURE;
	}
	sql = realloc(sql,strlen(sql));


	/*Executing the query*/
	queryCheck = SQLITE_BUSY;
	while (queryCheck == SQLITE_BUSY)
		queryCheck = sqlite3_exec(db,sql,0,0,NULL);
	if (queryCheck != SQLITE_OK)
	{
		free(sql); sql = NULL;
		return FAILURE;
	}

	free(sql); sql = NULL;

	return SUCCESS;
}


int ifaceInsert(int i)
{

	char *name;//name of the interface
	name = interface_names[i];

	unsigned long bytes_in;//incoming bytes
	unsigned long bytes_out;//outgoing bytes
	unsigned long bytes_total;//total bytes

	/*Setting their values*/
	bytes_in	 = new_bytes_in[i];
	bytes_out	 = new_bytes_out[i];
	bytes_total	 = bytes_in+bytes_out;
	/*Format of the query*/
	static char queryFormat[]="INSERT INTO INTERFACES(Name,RX,TX,TT)VALUES(\"%s\",%lu,%lu,%lu)";

	char *sql; //where the program will form the actual query
	sql = calloc(INSERT_IFACE_SIZE,sizeof(char));

	/*Query Formation*/
	int queryCheck;//check for successful formation of query
	queryCheck = snprintf(sql,INSERT_IFACE_SIZE,queryFormat,name,bytes_in,bytes_out,bytes_total);
	if (queryCheck == 0)
	{
		free(sql); sql = NULL;
		return FAILURE;
	}
	sql = realloc(sql,strlen(sql));

	/*Executing the query*/
	queryCheck = SQLITE_BUSY;
	while (queryCheck == SQLITE_BUSY)
		queryCheck = sqlite3_exec(db,sql,0,0,NULL);
	if (queryCheck != SQLITE_OK)
	{
		free(sql); sql = NULL;
		return FAILURE;
	}

	/*Extracting the interface's ID for use as offset's ID*/
	free(sql); sql = NULL;
	//SQLReport0,__FUNCTION__,__LINE__);
	char offset_query[] = "SELECT IfaceID FROM interfaces WHERE Name=\'%s\'";
	sql = calloc(OFFSET_QUERY_SIZE,sizeof(char));


	queryCheck = snprintf(sql,sizeof(offset_query)+29,offset_query,name);
	if (queryCheck == 0)
	{
		free(sql); sql = NULL;
		return FAILURE;
	}
	sql = realloc(sql,strlen(sql));
	//SQLReport0,__FUNCTION__,__LINE__);

	int offset_ID;
	queryCheck = SQLITE_BUSY;
	while (queryCheck == SQLITE_BUSY)
		queryCheck = sqlite3_exec(db,sql,fresult_offsetID,&offset_ID,0);
	if (queryCheck != SQLITE_OK)
	{
		free(sql); sql = NULL;
		return FAILURE;
	}
	AddOffset(offset_ID,&bytes_in,&bytes_out,&bytes_total);

	free(sql); sql =NULL;
	return SUCCESS;

}

int fresult_offsetID(void * ID, int argc, char ** argv, char ** unused2)
{
	//SQLReport0,__FUNCTION__,__LINE__);
	*(int *)ID = atoi(argv[0]);
	//SQLReport0,__FUNCTION__,__LINE__);
	return 0;
}

void AddOffset(int ID,ulong *in,ulong *out,ulong *total)
{
	offsets_list = realloc(offsets_list,(offsets_count+1)*sizeof(offset));
	offsets_list[offsets_count].if_ID = ID;
	offsets_list[offsets_count].in = *in;
	offsets_list[offsets_count].out = *out;
	offsets_list[offsets_count].total = *total;
	offsets_count++;
}

void SaveBandwidth(const int i,const int type) {
	int success_store;
	char *filename;
	double *kbps;
#ifdef DEBUGMODE
	char inout[9];
#endif
	if (type == OUT)
	{
#ifdef DEBUGMODE
		strcpy(inout,"outgoing");
#endif
		kbps_out = Bandwidth(old_bytes_out[i],new_bytes_out[i],&beginning,&ending,OUT);
		kbps=&kbps_out;
	}
	else
	{
#ifdef DEBUGMODE
		strcpy(inout,"incoming");
#endif
		kbps_in = Bandwidth(old_bytes_in[i],new_bytes_in[i],&beginning,&ending,IN);
		kbps=&kbps_in;
	}


	FILE *file;


	filename =SetFileName(interface_names[i],type);
	if(filename == NULL)
	{
#ifdef DEBUGMODE
		printf("Error in creating %s bandwidth file\n",inout);
#endif
		free(filename);
		return;
	}

	file = fopen(filename,"w");
	if(file == NULL)
	{
#ifdef DEBUGMODE
		printf("Error in opening %s bandwidth file\n",inout);
#endif

		fclose(file);
		free(filename);
		return;
	}


	success_store = fprintf(file,"%.2f",*kbps);
	if (success_store == 0)
	{
#ifdef DEBUGMODE
		printf("Error in storing data within %s bandwidth file\n",inout);
#endif
		fclose(file);
		free(filename);
		return;

	}

	free(filename);
	fclose(file);
}


double Bandwidth(const unsigned long old_bytes,const unsigned long new_bytes,struct timeval *beginning,struct timeval*ending,const int type)
{
	unsigned long bytes_difference = new_bytes - old_bytes;
	int seconds = ending->tv_sec - beginning->tv_sec;
	double delay_time = (double)(seconds) + (ending->tv_usec - beginning->tv_usec)/((double)1e6);
//	printf("\t%.2f\n",time);

	double rate = (double)bytes_difference/delay_time;
	rate /= 1024; //conversion to kilobytes/second
//	printf("%.2f",rate);
	return rate; //returning the success

}





char *SetFileName(char *interface_name,const int type)
{

	char directory[] ="/tmp/";
        char appended[5];
        if(type == IN)
                strcpy(appended,"_in");
        else
                strcpy(appended,"_out");

        char *filename;//points to the created filename

        int  filesize;//stores calculated filesize
        filesize =strlen(directory)+strlen(interface_name)+strlen(appended)+1;

        filename=calloc(filesize,sizeof(char));//<directory><interface name><appended>\0
	filename=strdup(directory);//adding directory to filename
        filename=strcat(filename,interface_name);//appending interface's name
        filename=strcat(filename,appended);//appending the in/out type

        return filename;

}





void FreeMemory(unsigned long *in,unsigned long *out,FILE *fp)
{
	if (fp != NULL)
	{
		free(fp);
	}
	free(in);
	free(out);
}



void FreeMemory2()
{
	for (int i = 0; i < interfaces; i++)
		free(interface_names[i]);
	free(interface_names);
	free(old_bytes_in);
	free(old_bytes_out);
	free(new_bytes_in);
	free(new_bytes_out);
	free(offsets_list);

}






int FileParser(const char *file,const int set)
{
	FILE *filepointer;

	unsigned long *bytes_in;
	unsigned long *bytes_out;

	char *stats;//used for traversing given line for statistics of an interface
	char *name; //used for pointing to interface name

	char buffer[BUFF_SIZE];


	filepointer = fopen(file,"r");
	if (set == OLD)
	{
//		if (first_delay == 1)
			gettimeofday(&beginning,NULL);
//		else
//		{
//			beginning.tv_sec=ending.tv_sec;
//			beginning.tv_usec=ending.tv_usec;
//		}
	}
	else if (set == NEW)
		gettimeofday(&ending,NULL);
	else
		{
#ifdef DEBUGMODE
			printf("Invalid set\n");
#endif
			return FAILURE;
		}


	//to reallcc these variables later on
	interface_names = calloc(1,sizeof(char *));//to make use of realloc in the loop
	bytes_in = calloc(1,sizeof(unsigned long ));
	bytes_out = calloc(1,sizeof(unsigned long ));



	if (filepointer != NULL)
	{
		int lines = 0; //for keeping check of lines traversed
		interfaces = 0;//for keeping track of interfaces

		int internal_buff = setvbuf(filepointer,NULL,_IONBF,0);//disabling internal buffer that messes up the program
		if (internal_buff != 0)//if disabling failed
		{
#ifdef DEBUGMODE
			printf("Error: Internal buffer not deactivated, which prevents filestream to buffer properly.\n");
#endif
			FreeMemory(bytes_in,bytes_out,NULL);
			return FAILURE;
		}


		while (fgets(buffer,BUFF_SIZE,filepointer) != NULL)
		{
			printf("\r");
			if (lines < 2)//to ignore first two lines of the file
			{
				lines++;
				continue;
			}
//----------------------------obtaining name----------------------------------//
			name = strtok(buffer,":");
//			printf("%s",name);
			if (name != NULL)
			{
				interface_names = realloc(interface_names,sizeof(char *)*interfaces+1);//to add interface
				if (interface_names == NULL)
				{
#ifdef DEBUGMODE
					printf("Error in adding interface name: not allocating space for name\n");
#endif
					FreeMemory(bytes_in,bytes_out,filepointer);
					return FAILURE;

				}

				interface_names[interfaces]=calloc(NAME_LENGTH,sizeof(char));
				if (interface_names[interfaces] == NULL)
				{
#ifdef DEBUGMODE
					printf("Error in adding interface name: not allocating space for name characters\n");
#endif
					FreeMemory(bytes_in,bytes_out,filepointer);
					return FAILURE;

				}

				while(strchr(name,' ') != NULL)//to make sure no space is added before interface
				{
					name++;
				}


				if (strcmp(name,"lo")==0)//no need of recording lo
				{
					continue;
				}

				interface_names[interfaces]=strdup(name);
				if (interface_names[interfaces] == NULL)
				{
#ifdef DEBUGMODE
					printf("Error in adding interface name: not saved\n");
#endif
					FreeMemory(bytes_in,bytes_out,filepointer);
					return FAILURE;
				}

				interface_names[interfaces]=realloc(interface_names[interfaces],strlen(interface_names[interfaces])+1);//memory optimization
				if (interface_names[interfaces] == NULL)
				{
#ifdef DEBUGMODE
					printf("Error in adding interface name: memory not optimized\n");
#endif
					FreeMemory(bytes_in,bytes_out,filepointer);
					return FAILURE;

				}
			}
			else
			{
#ifdef DEBUGMODE
				printf("Error in obtaining interface name\n");
#endif
				FreeMemory(bytes_in,bytes_out,filepointer);
				return FAILURE;
			}
//---------------------------------------name obtained------------------------------------//



//---------------------------------------obtaining data----------------------------------//
			stats = strtok(NULL," ");//to go to the bytes

			bytes_in = realloc(bytes_in,sizeof(unsigned long )*(interfaces+1));
			if (bytes_in == NULL)
			{
#ifdef DEBUGMODE
				printf("Error in adding bytes in: not allocating space for bytes\n");
#endif
				FreeMemory(bytes_in,bytes_out,filepointer);
				return FAILURE;

			}


			bytes_out = realloc(bytes_out,sizeof(unsigned long )*(interfaces+1));
			if (bytes_out == NULL)
			{
#ifdef DEBUGMODE
				printf("Error in adding bytes out: not allocating space for bytes\n");
#endif
				FreeMemory(bytes_in,bytes_out,filepointer);
				return FAILURE;

			}



			int success_bytes_in = sscanf(stats,"%lu",(bytes_in+interfaces));
			if (success_bytes_in == 0)
			{
#ifdef DEBUGMODE
				printf("Error in obtaining in_bytes");
#endif
				FreeMemory(bytes_in,bytes_out,filepointer);
				return FAILURE;

			}


			for (int i = 1; i <= 8; i++)
			{
				stats=strtok(NULL," ");
			}

			int success_bytes_out = sscanf(stats,"%lu",(bytes_out+interfaces));
			if (success_bytes_out == 0)
			{
#ifdef DEBUGMODE
				printf("Error in obtaining out_bytes");
#endif
				FreeMemory(bytes_in,bytes_out,filepointer);
				return FAILURE;

			}

				interfaces++;
		}



		fclose(filepointer);



		if(set == OLD)
		{
			old_bytes_in = realloc(old_bytes_in,(sizeof(unsigned long))*(interfaces));
			old_bytes_out = realloc(old_bytes_out,(sizeof(unsigned long))*(interfaces));
			for(int i = 0;i < interfaces;i++)
			{
		//static int nuo = 0;
		//nuo++;

				old_bytes_in[i] = bytes_in[i];
		//printf("Old: %s\t%lu\t",interface_names[i],old_bytes_in[i]);
				old_bytes_out[i]= bytes_out[i];
		//printf("%lu\t%i\n",old_bytes_out[i],nuo);

			}
		}
		if(set == NEW)
		{

			new_bytes_in = realloc(new_bytes_in,(sizeof(unsigned long))*(interfaces));
			new_bytes_out = realloc(new_bytes_out,(sizeof(unsigned long))*(interfaces));
			for (int i = 0; i < interfaces;i++)
			{
		//static int nu = 0;
	//	nu++;
				new_bytes_in[i] = bytes_in[i];
	//	printf("New: %s\t%lu\t",interface_names[i],new_bytes_in[i]);

				new_bytes_out[i] = bytes_out[i];
	//	printf("%lu\t%i\n",new_bytes_out[i],nu);

			}

		}
//-------------------------------------------data obtained--------------------//


		FreeMemory(bytes_in,bytes_out,NULL);

		return interfaces;
	}
	else
	{
#ifdef DEBUGMODE
		printf("File failed to open\n");
#endif
		return FAILURE;
	}


}
