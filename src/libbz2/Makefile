
SHELL=/bin/sh

# To assist in cross-compiling
#CC=gcc
#AR=ar
#RANLIB=ranlib

CFLAGS	 = -Wall -Winline -O3 -D_FILE_OFFSET_BITS=64
#CFLAGS += -fno-stack-protector
CFLAGS	+= -fomit-frame-pointer -fno-strict-aliasing -funroll-loops
CFLAGS	+=  $(XFLAGS)

OBJS= blocksort.o  \
      huffman.o    \
      crctable.o   \
      randtable.o  \
      compress.o   \
      decompress.o \
      bzlib.o

BZ2LIB=libbz2.a

# format for logging messages, $1=job, $2=object, $3=more info
CLOGFORMAT		:= *** %7s %-17s %s\n
LOGFORMAT		:= ***** %7s %-17s %s\n

#-------------------------------------------------------------------------------

$(BZ2LIB): $(OBJS)
	@printf "$(LOGFORMAT)" ar "$(BZ2LIB)" ""
	@rm -f $(BZ2LIB)
	@$(AR) cq $(BZ2LIB) $(OBJS)
	@if ( test -f $(RANLIB) -o -f /usr/bin/ranlib -o \
		-f /bin/ranlib -o -f /usr/ccs/bin/ranlib ) ; then \
		printf "$(LOGFORMAT)" "ranlib" "$(BZ2LIB)" "" ; \
		$(RANLIB) $(BZ2LIB) ; \
	fi

clean: 
	@printf "$(CLOGFORMAT)" clean "libbz2" ""
	@rm -f *.o $(BZ2LIB)

$(OBJS): %.o: %.c Makefile
	@printf "$(LOGFORMAT)" object "$@" ""
	$(CC) $(CFLAGS) -c $< -o $@

