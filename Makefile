PROGRAM=cjdc
C_SRCS=cjdc.c
H_SRCS=cjdc.h

include unistring.mk

$(PROGRAM): $(C_SRCS)
	$(CC) -o $(PROGRAM) -I$(UNISTRING_INC) $(C_SRCS) $(UNISTRING_LIB)/libunistring.a

$(C_SRCS): $(H_SRCS)

run: $(PROGRAM)
	$(PROGRAM)

clean:
	$(RM) *.o *~ $(PROGRAM)
