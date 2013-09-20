dbf2sql v2.0

OVERVIEW:

From this source you can build programs to convert xBase-style .dbf-files
to an mSQL or an Postgres95 table. Which one is the target is selected
on compile-time. The resulting binaries will have different names (dbf2msql
and dbf2pg resp.). Note that there are some limitations to these programs:
they do not support memo-fields, logical fields (type 'L' in dBase) are
converted to 'char(1)' (for mSQL, for Postgres95 this is 'char'), date-fields
are written as char(8) (or whatever length dBase gives these files).

There is also a program which can read out an mSQL-table and write a
.dbf-file for this. This is not (yet) supported for Postgres95. This program
is called msql2dbf. Note that this program makes an assumption on how long
a decimal field is in the resulting .dbf-file (10). It is also still
considered 'alpha-stage code', so use it at yur own risk (however, it seems
to perform pretty well).

dbf2msql | dbf2pg:

This program takes an xBase-file and sends queries to an mSQL-server or
Postgres95-server to insert it into a table. It takes a number of
arguments to set its behaviour:

-v  verbose:
    Produce some status-output

-vv more verbose:
    Also produce progress-report.

-f  field-lowercase:
    translate all field-names in the xbase-file to lowercase

-u  uppercase:
    Translate all text in the xbase-file to uppercase

-l  lowercase:
    Translate all text in the xbase-file to lowercase

-s  substitute:
    Takes a list of fieldname/newfieldname pairs. Primary use is to avoid
    conflicts between fieldnames and SQL reserved keywords. When the new
    fieldname is empty, the field is skipped in both the CREATE-clause and
    the INSERT-clauses, in common words: it will not be present in the
    SQL-table

        example:
                -s ORDER=HORDER,REMARKS=,STAT1=STATUS1

-d  database:
    Select the database to insert into. Default is 'test'

-t  table:
    Select the table to insert into. Default is 'test'

-c  create:
    If the table already exists, drop it and build a new one.
    The default is to insert all data into the named table.

-D  delete:
    Delete the contents of the table before inserting the new data.

-p  primary:
    Select the primary key. You have to give the exact
    field-name. (Only for dbf2msql, Postgres95 doesn't have a primary key)

-h  host:
    Select the host where to insert into. Defaults to localhost.

-s  start:
-e  end:
    You can insert a range of records from the .dbf-file by using
    -s and/or -e to specify a start- and end-record to insert.
    The default start is 0 and the default end is the last record in the file.


msql2dbf:

This takes basically the same arguments as dbf2msql, only [-p] has changed.

-v  verbose
-vv more verbose
-u  translate all field-contents to uppercase
-l  translate all field-contents to lowercase
-d  select database from where to read
-t  select table from where to read
-q  specifies custom-query to use
-p  specify precision to use in case of REAL-fields

THIS IS ALPHA-CODE!!!!!!!!!!

INSTALL:

To build it, take a look at the Makefile. You might have to change a few
things.

First of all you have to tell wether you want to build this stuff for
mSQL or for Postgres95 (ie. dbf2msql/msql2dbf or dbf2pg). Because of the
fact that you can build for mSQL *AND* for Postgres95, you'll have to
change stuff at several places, so look carefully in the Makefile (it's
all documented there).

Then there's the compiler you use. Also tell what compiler-options you
want (probably optimizing :). The next step is important: tell where
your mSQL/Postgres95 is installed, since this needs include-files and
libraries from there.

Some operating-systems require extra libraries to be linked against,
Solaris for example needs -lsocket and -lnsl to compile this cleanly.
Specify this in EXTRALIBS.

Then you're probably set to go. There are 3 more options you can change,
but you only need them if you want to do a 'make dist'. These are 'RM',
'GZIP' and 'TAR'. Of these, probably the only one that _could_ give
trouble is gzip, but you probably already have this, else you wouldn't
have succeeded in reading this document :).

So if you think you've set everything right, just do a 'make' and you'll
end up with either dbf2pg or dbf2msql and msql2dbf, depending on what you've
configured in the Makefile. The install-target has been removed, cos it would
have complicated things. So you'll have to copy these programs to the
location you want them yourself. Also don't forget to copy the manpages
to the right location and give them the correct name, ie. dbf2msql.1 and
msql2dbf.1 for mSQL-usage or dbf2pg.1 for usage with Postgres95).

COPYRIGHT:

Use this piece of software as you want, modify it to suit your needs,
but please leave my name in place ok? :)

DISCLAIMER:

I do not accept any responsibility for possible damage you get as result
of using this program.

KNOWN BUGS:

-   It doesn't handle memo-files. If somebody has specifications of these,
    please let me know and I'll see if I can add support for them.
-   There's no pg2dbf. This looks tricky to do, but I'm looking at it.

OTHER BUGS:

Possibly incorrect field-lengths for REAL-numbers.

CHANGES

dbf2sql-2.0:

-   Name-change to reflect the fact that there's also support for
    Postgres95.
-   Merged my own source for dbf2pg with the upstream source (used this
    with dbf-files with 147,000 records several times, so I guess it works :)
-   Added man-pages

dbf2msql-1.04:

-   Fixed bug introduced in version 1.03 that calculated the header-length
    (it was only correct by incident, when I added a pointer to a struct
    this broke).
-   Added a check when reading in the field-descriptions for end-of-header.
    Could fix some problems. Patch from Aaron Kardell <akardell@aksoft.com>.
-   Made numeric fields 10 long (4^32 has 10 chars....)

dbf2msql-1.03 (never publically released):

-   Changed dbf.c to use a standard buffer to read the record in,
    as opposed to allocating one everytime we call dbf_get_record().
    This will save time in reading and writing records. With dbf2msql and
    msql2dbf you won't notice much difference, cos the most time-consuming
    action is the communication with msqld, however, when you use these
    routines for something else it should make a difference.

dbf2msql-1.02b:

-   Fixed a typo in msql2dbf.c
-   Forgot to mention in the README of 1.02 that I also fixed a memory-
    leak, and I mean a major one..... (in dbf_get_record())
-   set *query = NULL in main() in msql2dbf.c. OSF on Alpha's chokes if you
    don't do this
-   changed strtoupper() and strtolower(), explicitly make it clear to
    the compiler *when* to increase the pointer.

dbf2msql-1.02:

-   Added a patch from Jeffrey Y. Sue <jysue@aloha.net> to 'rename'
    fieldnames. This also makes it possible to skip fields altogether
-   Added some patches from Frank Koormann <fkoorman@usf.Uni-Osanbrueck.DE>
    to initialize the different data-area's to their correct values, to set
    the dbf-time correctly and to use the correct format for real-values in
    dbf_put_record()

dbf2msql-1.01:

-   Changed every occurence of FILE to file_no. FILE reportedly caused
    trouble on some systems.
-   Changed dbf2msql.c so that when inserting an empty INT-field, an NULL
    text is inserted in the INSERT-clause instead of nothing. Suggestion
    by Jeffrey Y. Sue (jysue@aloha.net).
-   Same for REAL-fields (comes automagically with the change above).
-   When an error occurs during an INSERT-query, the values of that query
    are displayed when -vv is set.
    Suggestion by Jeffrey Y. Sue (jysue@aloha.net).
-   Added alpha-support for writing dbf-files from msql-tables by means
    of the program msql2dbf

dbf2msql-0.5:

-   Added the -f flag to translate fieldnames to lowercase.
    Suggestion by David Perry (deperry@nerosworld.com).

dbf2msql-0.4:

-   fixed a little offset-bug. I came across a file that had an extra char
    inserted after the header. However, the headerlength-field was correct,
    so I changed the program to use this instead of calculating the
    headerlength ourself. Should have done this in the first place.
    Bugreport from David Perry (deperry@nerosworld.com).

dbf2msql-0.3:

-   moved call to do_create() to inside the check if to create the table
    or import it into an existing one

dbf2msql-0.2:

-   Reorganized the code, cleaned up main(), moved building and sending
    of 'create-clause' to do_create() and moved building and sending
    of 'insert-clauses' to do_inserts.
-   Changed allocation of (char *)query in do_create() from static
    to dynamic, thus avoiding overruns.
-   Changed allocation of (char *)query, (char *)keys and (char *)vals
    in do_inserts() from static to dynamic, thus preventing overruns.
-   Fixed a nasty little bug in freeing allocated memory in
    dbf_free_record(). This was stupid, and only showed up *after*
    the code-reorganization. Don't know why I didn't notice it before.
    If you had *very* large dbases (with large fields), this could fill
    up your memory completely.
-   Added the -l option on suggestion from David Perry
    (deperry@zeus.nerosworld.com)
-   Added the -c option (dito)
-   Added the -vv option
-   Enhanced documentation (this little doc)

dbf2msql-0.1:

-   Initial release

Maarten Boekhold (M.Boekhold@et.tudelft.nl)
