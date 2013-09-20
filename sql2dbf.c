/*	utility to read out an table from mSQL or Postgres95, and store it
    into a DBF-file

	M. Boekhold (M.Boekhold@et.tudelft.nl) April 1996
	oktober 1996 added support for Postgres95
*/

#ifndef POSTGRES95
	#ifndef MSQL
		#error One of POSTGRES95 or MSQL *must* be defined in the Makefile
	#endif
#endif

#ifdef POSTGRES95
	#error Not implemented for Postgres95 yet, sorry, I m working on it
#endif

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <limits.h>
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

int		verbose = 0, upper = 0, lower = 0, create = 0;
long	precision = 6;
char	*host = NULL;
char	*dbase = NULL;
char	*table = NULL;

inline void strtoupper(char *string);
inline void strtolower(char *string);
void usage(void);

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

#ifdef POSTGRES95
void usage(void) {
	printf("pg2dbf %s\n", VERSION);
	printf("usage:\tpg2dbf [-h host] [-u | -l] [-v[v]]\n");
	printf("\t\t\t[-p precision] [-q query] -d dbase -t table dbf-file\n");
}
#endif
#ifdef MSQL
void usage(void) {
	printf("msql2dbf %s\n", VERSION);
	printf("usage:\tmsql2dbf [-h host] [-u | -l] [-v[v]]\n");
	printf("\t\t\t[-p precision] [-q query] -d dbase -t table dbf-file\n");
}
#endif

int main(int argc, char **argv) {
#ifdef POSTGRES95
	PGconn			*conn;
	PGresult		*qres;
	double			dvalue;
	unsigned int	ivalue;
#endif
#ifdef MSQL
	int             conn;
	m_result		*qres;
	m_row			qrow;
	m_field			*qfield;
#endif
	int				i;
	extern int      optind;
	extern char     *optarg;
	char            *query = NULL;
	dbhead          *dbh;
	field			*rec;
	u_long			numfields, numrows, t;

	while ((i = getopt(argc, argv, "lucvq:h:d:t:p:")) != EOF) {
		switch(i) {
			case 'q':
				query = strdup(optarg);
				break;
			case 'v':
				verbose++;
				break;
			case 'c':
				create = 1;
				break;
			case 'l':
				if (upper) {
					usage();
					printf("Can't use -u and -l at the same time!\n");
					exit(1);
				}
				lower = 1;
				break;
			case 'u':
				if (lower) {
					usage();
					printf("Can't use -u and -l at the same time!\n");
					exit(1);
				}
				upper = 1;
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
			case 'p':
				precision = strtol(optarg, (char **)NULL, 10);
				if ((precision == LONG_MIN) || (precision == LONG_MAX)) {
					usage();
					printf("Invalid precision specified!\n");
					exit(1);
				}
				break;
			case ':':
				usage();
				printf("Missing argument!\n");
				exit(1);
			case '?':
				usage();
				printf("Unknown argument: %s\n", argv[0]);
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

	if ((dbase == NULL) || ((table == NULL) && (query == NULL))) {
		usage();
		printf("Dbase and table *must* be specified!\n");
		exit(1);
	}

	if (query == NULL) {
		query = (char *)malloc(14+strlen(table));
		sprintf(query, "SELECT * FROM %s", table);
	}

	if (verbose > 1) {
		printf("Opening %s for writing (previous contents will be lost)\n",
																	argv[0]);
	}

	if ((dbh = dbf_open_new(argv[0], O_WRONLY | O_CREAT | O_TRUNC)) ==
												(dbhead *)DBF_ERROR) {
		fprintf(stderr, "Couldn't open xbase-file for writing: %s\n", argv[0]);
		exit(1);
	}

	if (verbose > 1) {
		printf("Making connection with SQL-server\n");
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

	if (verbose > 1) {
		printf("Sending query\n");
	}

#ifdef POSTGRES95
	qres = PQexec(conn, query);
	if ((!res) || (PGRES_TUPLES_OK != PQresultStatus(qres))) {
		fprintf(stderr, "Error sending query.\nDetailed report: %s\n",
				PQerrorMessage(conn));
		if (verbose > 1) {
			fprintf(stderr, "%s\n", query);
		}
		PQfinish(conn);
		close(dbh->db_fd);
		free(dbh);
		exit(1);
	}

	numfields		= PQnfields(qres);
	numrows			= PQntuples(qres);
#endif
#ifdef MSQL
	if (msqlQuery(conn, query) == -1) {
		fprintf(stderr, "Error sending query.\nDetailed report: %s\n",
				msqlErrMsg);
		if (verbose > 1) {
			fprintf(stderr, "%s\n", query);
		}
		msqlClose(conn);
		close(dbh->db_fd);
		free(dbh);
		exit(1);
	}

    qres            = msqlStoreResult();
    numfields       = msqlNumFields(qres);
	numrows			= msqlNumRows(qres);
#endif

/* this part needs to be heavely rewritten for Postgres95
	Biggest problem is the many many types that Postgres95 supports, and
	the fact that the types are encoded in some internal format.
	These are available from POSTGRESDIR/src/backend/catalog/pg_type.h, but
	it's very cryptic.
	They are also obtainable from the server in text-format, see
	POSTGRESDIR/src/bin/psql/psql.c for an example on how to do this.
*/
#ifdef POSTGRES95
	for (i = 0; i < numfields; i++) {
		switch (PQftype(qres, i)) {
/* assume INT's have a max. of 10 digits (4^32 has 10 digits) */
			case INT_TYPE:
				dbf_add_field(dbh, qfield->name, 'N', 10, 0);
				if (verbose > 1) {
					printf("Adding field: %s, INT_TYPE, %d\n", qfield->name,
															   qfield->length);
				}
				break;
/* do we have to make the same assumption here? */
			case REAL_TYPE:
				dbf_add_field(dbh, qfield->name, 'N', qfield->length,
								(int) precision);
				if (verbose > 1) {
					printf("Adding field: %s, INT_REAL, %d\n", qfield->name,
															   qfield->length);
				}
				break;
			case CHAR_TYPE:
				dbf_add_field(dbh, qfield->name, 'C', qfield->length, 0);
				if (verbose > 1) {
					printf("Adding field: %s, INT_CHAR, %d\n", qfield->name,
															   qfield->length);
				}
				break;
			default:
				break;
		}
	}
#endif
/* mSQL makes this easy, but is limited in datatypes */
#ifdef MSQL
	while ((qfield = msqlFetchField(qres))) {
		strtoupper(qfield->name);
		switch (qfield->type) {
/* assume INT's have a max. of 10 digits (4^32 has 10 digits) */
			case INT_TYPE:
				dbf_add_field(dbh, qfield->name, 'N', 10, 0);
				if (verbose > 1) {
					printf("Adding field: %s, INT_TYPE, %d\n", qfield->name,
															   qfield->length);
				}
				break;
/* do we have to make the same assumption here? */
			case REAL_TYPE:
				dbf_add_field(dbh, qfield->name, 'N', qfield->length,
								(int) precision);
				if (verbose > 1) {
					printf("Adding field: %s, INT_REAL, %d\n", qfield->name,
															   qfield->length);
				}
				break;
			case CHAR_TYPE:
				dbf_add_field(dbh, qfield->name, 'C', qfield->length, 0);
				if (verbose > 1) {
					printf("Adding field: %s, INT_CHAR, %d\n", qfield->name,
															   qfield->length);
				}
				break;
			default:
				break;
		}
	}
#endif

	if (verbose > 1) {
		printf("Writing out header and field-descriptions\n");
	}

	dbf_write_head(dbh);
	dbf_put_fields(dbh);

/* Also needs to be adjusted for use with Postgres95, however, this will
	be easier then the previous part */
	if ((rec = dbf_build_record(dbh)) == (field *)DBF_ERROR) {
		fprintf(stderr, "Error allocating memory for record-contents!\n");
	} else {
		if (numrows) {
#ifdef MSQL
			while ((qrow = msqlFetchRow(qres)) != NULL) {
				for (t = 0; t < numfields; t++) {
					if (qrow[t] != 0) {
						strcpy(rec[t].db_contents, qrow[t]);
					} else {
						rec[t].db_contents[0] = '\0';
					}
					if (upper) {
						strtoupper(rec[t].db_contents);
					}
					if (lower) {
						strtolower(rec[t].db_contents);
					}
				}
				dbf_put_record(dbh, rec, dbh->db_records + 1);
			}
#endif
		}
		dbf_free_record(dbh, rec);
	}

	if (verbose > 1) {
		printf("Writing out header\n");
	}

	dbf_write_head(dbh);

	if (verbose > 1) {
		printf("Closing up\n");
	}
				
#ifdef MSQL
	msqlFreeResult(qres);
	msqlClose(conn);
#endif
	close(dbh->db_fd);
	free(dbh);
	exit(0);
}			
