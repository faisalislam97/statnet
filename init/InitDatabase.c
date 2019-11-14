//This program will create a table in the database
#include "sqlite3.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
//#define BUFF_SIZE 1000
//										|
int main()
{
	sqlite3 *db;

//	char sql[BUFF_SIZE];

	int rc = sqlite3_open("/etc/fuse-db.db",&db);

	if ( rc != SQLITE_OK)
	{
		printf("Failed\n");
		sqlite3_close(db);
		return 1;
	}
//		printf("%s\n",sqlite3_libversion());
	char sql[] ="BEGIN TRANSACTION;"
	"CREATE TABLE IF NOT EXISTS interfaces(IfaceID INTEGER PRIMARY KEY,"
	"Name TEXT UNIQUE NOT NULL,rx INTEGER NOT NULL,tx INTEGER NOT NULL,"
	"tt INTEGER NOT NULL,QuotaID INTEGER,enabled INTEGER NOT NULL DEFAULT 0,"
	"overusage INTEGER NOT NULL DEFAULT 0);"
	"CREATE TABLE IF NOT EXISTS quota(QuotaID INTEGER PRIMARY KEY AUTOINCREMENT,"
	"rx INTEGER NOT NULL,tx INTEGER NOT NULL,tt INTEGER NOT NULL);"
	"INSERT INTO quota(rx,tx,tt)VALUES(0,0,50000),(0,0,500000),(0,0,5000000);"
	"CREATE TABLE IF NOT EXISTS bandwidths(Date TEXT NOT NULL,Time TEXT NOT NULL,"
	"Name TEXT NOT NULL,rx DOUBLE NOT NULL,tx DOUBLE NOT NULL);"
	"CREATE TABLE IF NOT EXISTS hourPeak(Date TEXT NOT NULL,Time TEXT NOT NULL,"
	"Name TEXT NOT NULL, rx DOUBLE NOT NULL,tx DOUBLE NOT NULL);"
	"CREATE INDEX IF NOT EXISTS bandwidths_index ON bandwidths(Date,Time,Name);"
	"CREATE INDEX IF NOT EXISTS hour_index ON hourPeak(Name);"
//	"CREATE TABLE IF NOT EXISTS dayPeak(Date TEXT NOT NULL,Name TEXT NOT NULL,"
//	"rx DOUBLE NOT NULL, tx DOUBLE NOT NULL);"
//	"CREATE INDEX IF NOT EXISTS day_index ON dayPeak(Date,Name);"
	"COMMIT;";
	rc = SQLITE_BUSY;
	while (rc == SQLITE_BUSY)
		rc = sqlite3_exec(db,sql,0,0,NULL);
	if ( rc != SQLITE_OK)
	{
		printf("Failed: %s\n",sqlite3_errmsg(db));
		sqlite3_close(db);
		return 1;
	}
	sqlite3_close(db);
	return 0;


}
