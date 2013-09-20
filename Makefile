# Makefile fopr the dbf2pg-utility
# Maarten Boekhold (M.Boekhold@et.tudelft.nl) 1995

# first and most important, set the target, ie. wether you want to
# build this for Postgres95 or for mSQL

# use this for Postgres95
#DEFINES = -DPOSTGRES95
# use this for mSQL
DEFINES = -DMSQL

# Set this to your C-compiler
CC=gcc

# for BSDI use /usr/bin/ar rc
AR=ar rcs

# Set this to whatever your compiler accepts. Nothing special is needed
#CFLAGS=-O2 -Wall
#CFLAGS=-g3 -Wall
CFLAGS=-O5 -Wall -Zexe

# For Postgres95 usage
# Set this to your postgres95 installation-path
#SQLINC=-I/usr/local/postgres95/include
#SQLLIBDIR=-L/usr/local/postgres95/lib
#SQLLIB=-lpq

# For mSQL usage
# Set this to your mSQL installation-path
SQLINC=-I\public\mSQL2\include
SQLLIBDIR=-L\public\mSQL2\lib
SQLLIB=-llibmsql

# Set this if your system needs extra libraries
# For Solaris use:
#EXTRALIBS=-lsocket -lnsl
EXTRALIBS=

# You should not have to change this unless your system doesn't have gzip
# or doesn't have it in the standard place (/usr/local/bin for ex.).
# Anyways, it is not needed for just a simple compile and install
RM=del
GZIP=/bin/gzip
TAR=/bin/tar

VERSION=2.0

#OBJS=dbf.o endian.o libdbf.a dbf2pg.o pg2dbf.o
OBJS=dbf.o endian.o libdbf.a dbf2sql.o sql2dbf.o conv.o

# for postgres95 use this line and comment out the second
#all: dbf2pg pg2dbf

# for mSQL use this line and comment out the first
all: dbf2msql msql2dbf

libdbf.a: dbf.o endian.o
	$(AR) libdbf.a dbf.o endian.o

dbf2msql: dbf2sql.o libdbf.a conv.o
	$(CC) $(CFLAGS) -s -L. $(SQLLIBDIR) -o $@ dbf2sql.o conv.o libdbf.a $(SQLLIB) $(EXTRALIBS)

msql2dbf: sql2dbf.o libdbf.a
	$(CC) $(CFLAGS) -s -L. $(SQLLIBDIR) -o $@ sql2dbf.o libdbf.a $(SQLLIB) $(EXTRALIBS)

dbf.o: dbf.c dbf.h
	$(CC) $(CFLAGS) -c -o $@ dbf.c

endian.o: endian.c
	$(CC) $(CFLAGS) -c -o $@ endian.c

conv.o: conv.c conv.h
	$(CC) $(CFLAGS) -c -o $@ conv.c

dbf2sql.o: dbf2sql.c dbf.h
	$(CC) $(CFLAGS) $(DEFINES) -DVERSION=\"$(VERSION)\" $(SQLINC) -c -o $@ dbf2sql.c

sql2dbf.o: sql2dbf.c dbf.h
	$(CC) $(CFLAGS) $(DEFINES) -DVERSION=\"$(VERSION)\" $(SQLINC) -c -o $@ sql2dbf.c

clean:
	$(RM) $(OBJS) dbf2pg pg2dbf dbf2msql msql2dbf

# the 'expand' is just for me, I use a tabstop of 4 for my editor, which
# makes lines in the README very long and ugly for people using 8, so
# I just expand them to spaces

dist:
	-expand -4 README.tab > README
	(cd .. ; $(TAR) cf dbf2sql-$(VERSION).tar dbf2sql-$(VERSION)/*.[ch] \
	dbf2sql-$(VERSION)/Makefile dbf2sql-$(VERSION)/README \
	dbf2sql-$(VERSION)/*.1 ; \
	$(GZIP) -9 dbf2sql-$(VERSION).tar)

