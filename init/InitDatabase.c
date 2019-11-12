//This program will create a table in the database
#include "sqlite3.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
//#define BUFF_SIZE 1000

int main()
{
	sqlite3 *db;

//	char sql[BUFF_SIZE];

	int rc = sqlite3_open("/etc/amt-db.db",&db);

	if ( rc != SQLITE_OK)
	{
		printf("Failed\n");
		sqlite3_close(db);
		return 1;
	}
//		printf("%s\n",sqlite3_libversion());
	char *sql ="BEGIN TRANSACTION;CREATE TABLE IF NOT EXISTS interfaces(IfaceID INTEGER PRIMARY KEY,Name TEXT UNIQUE NOT NULL,rx INTEGER NOT NULL,tx INTEGER NOT NULL,tt INTEGER NOT NULL,QuotaID INTEGER,enabled INTEGER NOT NULL DEFAULT 0,overusage INTEGER NOT NULL DEFAULT 0);CREATE TABLE IF NOT EXISTS quota(QuotaID INTEGER PRIMARY KEY AUTOINCREMENT,rx INTEGER NOT NULL,tx INTEGER NOT NULL,tt INTEGER NOT NULL);INSERT INTO quota(rx,tx,tt)VALUES(0,0,50000),(0,0,500000),(0,0,5000000);CREATE TABLE IF NOT EXISTS bandwidths(Date TEXT NOT NULL,Time TEXT NOT NULL,Name TEXT NOT NULL,rx DOUBLE NOT NULL,tx DOUBLE NOT NULL);CREATE TABLE IF NOT EXISTS hourPeak(Time TEXT NOT NULL,Name TEXT NOT NULL,rx DOUBLE NOT NULL,tx DOUBLE NOT NULL);CREATE INDEX bandwidths_index ON bandwidths(Date,Time,Name);CREATE INDEX hour_index ON hourPeak(Name);COMMIT;";
	rc = sqlite3_exec(db,sql,0,0,NULL);
	if ( rc != SQLITE_OK)
	{
		printf("Failed2");
		sqlite3_close(db);
		return 1;
	}
	sqlite3_close(db);
	return 0;


}
