/* This program reads in an xbase-dbf file and sends 'inserts' to an
   mSQL- or Postgres95-server with the records in the xbase-file
   (depending on how it was compiled)

   M. Boekhold (boekhold@cindy.et.tudelft.nl)  okt. 1995
   oktober 1996: merged sources of dbf2msql.c and dbf2pg.c
*/

#ifndef POSTGRES95
	#ifndef MSQL
		#error One of POSTGRES95 or MSQL *must* be defined in the Makefile
	#endif
#endif

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#ifdef POSTGRES95
	#include <libpq-fe.h>
#endif
#ifdef MSQL
	#include <msql.h>
#endif
#include "dbf.h"

int	verbose = 0, upper = 0, lower = 0, create = 0, fieldlow = 0;
int del = 0;
unsigned int begin = 0, end = 0;
#ifdef MSQL
/* Postgres95 doesn't support primary keys */
	char	primary[11];
#endif
char	*host = NULL;
char 	*dbase = "test";
char 	*table = "test";
char  *subarg = NULL;

void do_substitute(char *subarg, dbhead *dbh);
inline void strtoupper(char *string);
#ifdef POSTGRES95
inline void strtolower(char *string);
void do_create(PGconn *, char*, dbhead*);
void do_inserts(PGconn *, char*, dbhead*);
int check_table(PGconn *, char*);
#endif
#ifdef MSQL
void do_create(int, char*, dbhead*);
void do_inserts(int, char*, dbhead*);
int check_table(int, char*);
#endif
char *Escape(char*);
void usage(void);
extern void ConvertAscii(char *);

inline void strtoupper(char *string) {
	while(*string != '\0') {
		*string = toupper(*string);
		*string++;
	}
}

inline void strtolower(char *string) {
	while(*string != '\0') {
		*string = tolower(*string);
		*string++;
	}
}

char *Escape(char *string) {
    char    *escaped, *foo, *bar;
    u_long  count = 0;

    foo = string;
    while(*foo != '\0') {
        if (*foo++ == '\'') count++;
    }

    if (!(escaped = (char *)malloc(strlen(string) + count + 1))) {
        return (char *)-1;
    }

    foo = escaped;
    bar = string;
    while (*bar != '\0') {
        if (*bar == '\'') {
            *foo++ = '\\';
        }
        *foo++ = *bar++;
    }
    *foo = '\0';

    return escaped;
}

#ifdef POSTGRES95
int check_table(PGconn *conn, char *table) {
	char		*q = "select relname from pg_class where "
					"relkind='r' and relname !~* '^pg'";
	PGresult	*res;
	int 		i = 0;

	if (!(res = PQexec(conn, q))) {
		printf("%s\n", PQerrorMessage(conn));
		return 0;
	}

	for (i = 0; i < PQntuples(res); i++) {
		if (!strcmp(table, PQgetvalue(res, i, PQfnumber(res, "relname")))) {
			return 1;
		}
	}

	return 0;
}
#endif

#ifdef MSQL
int check_table(int sock, char *table) {
	m_result	*result;
	m_row		row;

	if ((result = msqlListTables(sock))) {
		while ((row = msqlFetchRow(result))) {
			if (strcmp((char *) row[0], table)) {
				msqlFreeResult(result);
				return 1;
			}
		}
	}

	return 0;
}
#endif

#ifdef POSTGRES95
void usage(void){
		printf("dbf2pg %s\n", VERSION);
		printf("usage:\tdbf2pg\t[-u | -l] [-h hostname]\n");
		printf("\t\t\t[-s oldname=newname[,oldname=newname]]\n");
		printf("\t\t\t-d dbase -t table [-c | -D] [-f] [-v[v]]\n");
		printf("\t\t\tdbf-file\n");
}
#endif
#ifdef MSQL
void usage(void){
		printf("dbf2msql %s\n", VERSION);
		printf("usage:\tdbf2msql\t[-u | -l] [-h hostname]\n");
		printf("\t\t\t[-s oldname=newname[,oldname=newname]]\n");
		printf("\t\t\t-d dbase -t table [-c] [-f] [-v[v]]\n");
		printf("\t\t\t[-p primary key]dbf-file\n");
}
#endif

/* patch submitted by Jeffrey Y. Sue <jysue@aloha.net> */
/* Provides functionallity for substituting dBase-fieldnames for others */
/* Mainly for avoiding conflicts between fieldnames and mSQL-reserved */
/* keywords (this stems from the time there was only an mSQL-version :) */

void do_substitute(char *subarg, dbhead *dbh)
{
      /* NOTE: subarg is modified in this function */
      int i,bad;
      char *p,*oldname,*newname;
      if (!subarg) {
              return;
      }
      if (verbose>1) {
              printf("Substituting new field names\n");
      }
      /* use strstr instead of strtok because of possible empty tokens */
      oldname = subarg;
      while (oldname && strlen(oldname) && (p=strstr(oldname,"=")) ) {
              *p = '\0';      /* mark end of oldname */
              newname = ++p;  /* point past \0 of oldname */
              if (strlen(newname)) {  /* if not an empty string */
                      p = strstr(newname,",");
                      if (p) {
                              *p = '\0';      /* mark end of newname */
                              p++;    /* point past where the comma was */
                      }
              }
              if (strlen(newname)>=DBF_NAMELEN) {
                      printf("Truncating new field name %s to %d chars\n",
                              newname,DBF_NAMELEN-1);
                      newname[DBF_NAMELEN-1] = '\0';
              }
              bad = 1;
              for (i=0;i<dbh->db_nfields;i++) {
                      if (strcmp(dbh->db_fields[i].db_name,oldname)==0) {
                              bad = 0;
                              strcpy(dbh->db_fields[i].db_name,newname);
                              if (verbose>1) {
                                      printf("Substitute old:%s new:%s\n",
                                              oldname,newname);
                              }
                              break;
                      }
              }
              if (bad) {
                      printf("Warning: old field name %s not found\n",
                              oldname);
              }
              oldname = p;
      }
} /* do_substitute */

#ifdef POSTGRES95
void do_create(PGconn *conn, char *table, dbhead *dbh) {
#endif
#ifdef MSQL
void do_create(int conn, char *table, dbhead *dbh) {
#endif
	char		*query;
	char 		t[20];
	int			i, length;
#ifdef POSTGRES95
	PGresult	*res;
#endif

	if (verbose > 1) {
		printf("Building CREATE-clause\n");
	}

	if (!(query = (char *)malloc(
			(dbh->db_nfields * 40) + 29 + strlen(table)))) {
		fprintf(stderr, "Memory allocation error in function do_create\n");
#ifdef POSTGRES95
		PQfinish(conn);
#endif
#ifdef MSQL
		msqlClose(conn);
#endif
		close(dbh->db_fd);
		free(dbh);
		exit(1);
	}

	sprintf(query, "CREATE TABLE %s (", table);
	length = strlen(query);
	for ( i = 0; i < dbh->db_nfields; i++) {
              if (!strlen(dbh->db_fields[i].db_name)) {
                      continue;
                      /* skip field if length of name == 0 */
              }
		if ((strlen(query) != length)) {
                        strcat(query, ",");
                }

				if (fieldlow)
					strtolower(dbh->db_fields[i].db_name);

				ConvertAscii(dbh->db_fields[i].db_name);

                strcat(query, dbh->db_fields[i].db_name);
                switch(dbh->db_fields[i].db_type) {
						case 'D':
                        case 'C':
#ifdef POSTGRES95
                                strcat(query, " char");
								if (dbh->db_fields[i].db_flen > 1) {
	                                sprintf(t,"(%d)",
											dbh->db_fields[i].db_flen);
									strcat(query, t);
								}
#endif
#ifdef MSQL
								strcat(query, " char");
								sprintf(t, "(%d)",
											dbh->db_fields[i].db_flen);
								strcat(query, t);
#endif
                                break;
                        case 'N':
                                if (dbh->db_fields[i].db_dec != 0) {
                                        strcat(query, " real");
                                } else {
                                        strcat(query, " int");
                                }
                                break;
                        case 'L':
#ifdef POSTGRES95
                                strcat(query, " char");
#endif
#ifdef MSQL
								strcat(query, " char(1)");
#endif
                                break;
                }
#ifdef MSQL
                if (strcmp(dbh->db_fields[i].db_name, primary) == 0) {
					strcat(query, " primary key");
				}
#endif
	}

	strcat(query, ")");

	if (verbose > 1) {
		printf("Sending create-clause\n");
		printf("%s\n", query);
	}

#ifdef POSTGRES95
	if ((res = PQexec(conn, query)) == NULL) {
		fprintf(stderr, "Error creating table!\n");
		fprintf(stderr, "Detailed report: %s\n", PQerrorMessage(conn));
                close(dbh->db_fd);
                free(dbh);
				free(query);
                PQfinish(conn);
                exit(1);
	}

	PQclear(res);
#endif
#ifdef MSQL
	if (msqlQuery(conn, query) == -1) {
		fprintf(stderr, "Error creating table!\n");
		fprintf(stderr, "Detailed report: %s\n", msqlErrMsg);
                close(dbh->db_fd);
                free(dbh);
				free(query);
                msqlClose(conn);
                exit(1);
	}

#endif
	free(query);
}

#ifdef POSTGRES95
void do_inserts(PGconn *conn, char *table, dbhead *dbh) {
#endif
#ifdef MSQL
void do_inserts(int conn, char *table, dbhead *dbh) {
#endif
	int		result, i, h;
	field	*fields;
	char	*query, *keys, *vals, *foo;
	u_long	key_len = 0;
	u_long	val_len = 0;
#ifdef POSTGRES95
	PGresult	*res;
#endif

	if (verbose > 1) {
		printf("Inserting records\n");
	}

	for ( i = 0 ; i < dbh->db_nfields ; i++ ) {
/* This (strtolower) could be done twice (before in do_create()), but
	I needed *some* place where I could do this for do_inserts() :)
*/
		if (fieldlow)
			strtolower(dbh->db_fields[i].db_name);
/* The +1 is for the separating comma */
		key_len += strlen(dbh->db_fields[i].db_name) + 1;
/*  take into account the possibility that there are NULL's in the query
*/
		val_len += ((dbh->db_fields[i].db_flen) > 4 ?
						dbh->db_fields[i].db_flen + 3 : 7);
	}

	if (!(query = (char *)malloc(26 + strlen(table) + key_len + val_len))) {
		fprintf(stderr,
			"Memory allocation error in function do_inserts (query)\n");
#ifdef POSTGRES95
		PQfinish(conn);
#endif
#ifdef MSQL
		msqlClose(conn);
#endif
		close(dbh->db_fd);
		free(dbh);
		exit(1);
	}

	if (!(keys = (char *)malloc(key_len))) {
		fprintf(stderr,
			"Memory allocation error in function do_inserts (keys)\n");
#ifdef POSTGRES95
		PQfinish(conn);
#endif
#ifdef MSQL
		msqlClose(conn);
#endif
		close(dbh->db_fd);
		free(query);
		free(dbh);
		exit(1);
	}

	if (!(vals = (char *)malloc(val_len))) {
		fprintf(stderr,
			"Memory allocation error in function do_inserts (vals)\n");
#ifdef POSTGRES95
		PQfinish(conn);
#endif
#ifdef MSQL
		msqlClose(conn);
#endif
		close(dbh->db_fd);
		free(keys);
		free(dbh);
		exit(1);
	}

	if ((fields = dbf_build_record(dbh)) == (field *)DBF_ERROR) {
        fprintf(stderr,
            "Couldn't allocate memory for record in do_insert\n");
#ifdef POSTGRES95
		PQfinish(conn);
#endif
#ifdef MSQL
		msqlClose(conn);
#endif
        dbf_close(dbh);
        free(keys);
        exit(1);
    }
		
	i = begin;
	if (end == 0)
		end = dbh->db_records;

	for ( i; i < end; i++) {
		result = dbf_get_record(dbh, fields,  i);
		if (result == DBF_VALID) {
			keys[0]='\0';
			vals[0]='\0';

			for (h = 0; h < dbh->db_nfields; h++) {
   				/* if length of fieldname==0, skip it */
   				if (!strlen(fields[h].db_name)) {
   					continue;
   				}
				/* no comma before the first value */
				if (strlen(keys) != 0) {
					strcat(keys, ",");
				}
				if (strlen(vals) != 0) {
					strcat(vals, ",");
				}
				/* no apostrophes around numbers and NULL's */
				strcat(keys, fields[h].db_name);
				if ((fields[h].db_type != 'N') &&
					(strlen(fields[h].db_contents))) { /* we may have to
														insert a NULL */
					strcat(vals, "'");
				}

				/* convert to corresponding 7-bit ascii chars */
				ConvertAscii(fields[h].db_contents);

				if (upper) {
				   strtoupper(fields[h].db_contents);
				}
				if (lower) {
					strtolower(fields[h].db_contents);
				}

				/* escape all special chars ('\) */
				foo=Escape(fields[h].db_contents);
				strcat(vals, foo);
				/* if field is empty insert a NULL */
				if (*foo == '\0') {
					strcat(vals, "NULL");
				}
				free(foo);
				/* numbers and NULL's don't need apostrophes */
				if ((fields[h].db_type != 'N') &&
					(strlen(fields[h].db_contents))) {
					strcat(vals, "'");
				}
			}

			sprintf(query,
				"INSERT INTO %s ( %s ) VALUES ( %s )\n",
			    	table, keys, vals);

			if ((verbose > 1) && ((i % 100) == 0)) {
				printf("Inserting record %d\n", i);
			}
			if (verbose > 2) {
				printf("Query: %s\n", query);
			}

#ifdef POSTGRES95
			if ((res = PQexec(conn, query)) == NULL) {
				fprintf(stderr,
					"Error sending INSERT in record %d\n", i);
				fprintf(stderr,
					"Detailed report: %s\n",
					PQerrorMessage(conn));
				if (verbose > 1) {
					fprintf(stderr, "%s", query);
				}
			}
			if (res != NULL) PQclear(res);
#endif
#ifdef MSQL
			if (msqlQuery(conn, query) == -1) {
				fprintf(stderr,
					"Error sending INSERT in record %04d\n", i);
				fprintf(stderr,
					"Detailed report: %s\n",
					msqlErrMsg);
				if (verbose > 1) {
					fprintf(stderr, "%s", query);
				}
			}
#endif
		}
	}

	dbf_free_record(dbh, fields);

	free(query);
	free(keys);
	free(vals);
}

int main(int argc, char **argv)
{
#ifdef POSTGRES95
	PGconn 		*conn;
#endif
#ifdef MSQL
	int			conn;
#endif
	int			i;
	extern int 	optind;
	extern char	*optarg;
	char		*query;
	dbhead		*dbh;

	while ((i = getopt(argc, argv, "Dflucvh:b:e:d:t:s:")) != EOF) {
		switch (i) {
			case 'D':
				if (create) {
					usage();
					printf("Can't use -c and -D at the same time!\n");
					exit(1);
				}
				del = 1;
				break;
			case 'f':
				fieldlow=1;
				break;
			case 'v':
				verbose++;
				break;
			case 'c':
				if (del) {
					usage();
					printf("Can't use -c and -D at the same time!\n");
					exit(1);
				}
				create=1;
				break;
			case 'l':
				lower=1;
				break;
			case 'u':
				if (lower) {
					usage();
					printf("Can't use -u and -l at the same time!\n");
					exit(1);
				}
				upper=1;
				break;
			case 'b':
				begin = atoi(optarg);
				break;
			case 'e':
				end = atoi(optarg);
				break;
			case 'h':
				host = (char *)strdup(optarg);
				break;
			case 'd':
				dbase = (char *)strdup(optarg);
				break;
			case 't':
				table = (char *)strdup(optarg);
                break;
			case 's':
				subarg = (char *)strdup(optarg);
				break;
			case ':':
				usage();
				printf("missing argument!\n");
				exit(1);
			case '?':
				usage();
				printf("unknown argument: %s\n", argv[0]);
				exit(1);
			default:
				break;
		}
	}

	argc -= optind;
	argv = &argv[optind];

	if (argc != 1) {
		usage();
		exit(1);
	}

	if (verbose > 1) {
		printf("Opening dbf-file\n");
	}

	if ((dbh = dbf_open(argv[0], O_RDONLY)) == (dbhead *)-1) {
		fprintf(stderr, "Couldn't open xbase-file %s\n", argv[0]);
		exit(1);
	}

	if (verbose) {
#ifdef POSTGRES95
		printf("dbf-file: %s, PG-dbase: %s, PG-table: %s\n", argv[0],
																 dbase,
																 table);
#endif
#ifdef MSQL
		printf("dbf-file: %s, PG-dbase: %s, PG-table: %s\n", argv[0],
																 dbase,
																 table);
#endif
		printf("Number of records: %ld\n", dbh->db_records);
		printf("NAME:\t\tLENGTH:\t\tTYPE:\n");
		printf("-------------------------------------\n");
		for (i = 0; i < dbh->db_nfields ; i++) {
			printf("%-12s\t%7d\t\t%5c\n",dbh->db_fields[i].db_name,
									 dbh->db_fields[i].db_flen,
									 dbh->db_fields[i].db_type);
		}
	}

	if (verbose > 1) {
#ifdef POSTGRES95
		printf("Making connection to PG-server\n");
#endif
#ifdef MSQL
		printf("Making connection to mSQL-server\n");
#endif
	}

#ifdef POSTGRES95
	conn = PQsetdb(host,NULL,NULL,NULL, dbase);
	if (PQstatus(conn) != CONNECTION_OK) {
		fprintf(stderr, "Couldn't get a connection with the ");
		fprintf(stderr, "designated host!\n");
		fprintf(stderr, "Detailed report: %s\n", PQerrorMessage(conn));
		close(dbh->db_fd);
		free(dbh);
		exit(1);
	}
#endif
#ifdef MSQL
	if ((conn = msqlConnect(host)) == -1) {
		fprintf(stderr, "Couldn't get a connection with the ");
		fprintf(stderr, "designated host!\n");
		fprintf(stderr, "Detailed report: %s\n", msqlErrMsg);
		close(dbh->db_fd);
		free(dbh);
		exit(1);
	}

	if (verbose > 1) {
		printf("Selecting database\n");
	}

	if ((msqlSelectDB(conn, dbase)) == -1) {
		fprintf(stderr, "Couldn't select database %s.\n", dbase);
		fprintf(stderr, "Detailed report: %s\n", msqlErrMsg);
		close(dbh->db_fd);
		free(dbh);
		msqlClose(conn);
		exit(1);
	}
#endif

/* Substitute field names */
	do_substitute(subarg, dbh);

	if (!create) {
		if (!check_table(conn, table)) {
			printf("Table does not exist!\n");
			exit(1);
		}
		if (del) {
			if (!(query = (char *)malloc(13 + strlen(table)))) {
				printf("Memory-allocation error in main (delete)!\n");
				close(dbh->db_fd);
				free(dbh);
#ifdef POSTGRES95
				PQfinish(conn);
#endif
#ifdef MSQL
				msqlClose(conn);
#endif
				exit(1);
			}
			if (verbose > 1) {
				printf("Deleting from original table\n");
			}
			sprintf(query, "DELETE FROM %s", table);
#ifdef POSTGRES95
			PQexec(conn, query);
#endif
#ifdef MSQL
			msqlQuery(conn, query);
#endif
			free(query);
		}
	} else {
		if (!(query = (char *)malloc(12 + strlen(table)))) {
			printf("Memory-allocation error in main (drop)!\n");
			close(dbh->db_fd);
			free(dbh);
#ifdef POSTGRES95
			PQfinish(conn);
#endif
#ifdef MSQL
			msqlClose(conn);
#endif
			exit(1);
		}
		if (verbose > 1) {
			printf("Dropping original table (if one exists)\n");
		}
		sprintf(query, "DROP TABLE %s", table);
#ifdef POSTGRES95
		PQexec(conn, query);
#endif
#ifdef MSQL
		msqlQuery(conn, query);
#endif
		free(query);

/* Build a CREATE-clause
*/
		do_create(conn, table, dbh);
	}

/* Build an INSERT-clause
*/
	do_inserts(conn, table, dbh);

	if (verbose > 1) {
		printf("Closing up....\n");
	}

    close(dbh->db_fd);
    free(dbh);
#ifdef POSTGRES95
    PQfinish(conn);
#endif
#ifdef MSQL
	msqlClose(conn);
#endif
	exit(0);
}
