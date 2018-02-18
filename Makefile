PROGRAM=cjdc
C_SRCS=cjdc.c
H_SRCS=cjdc.h

$(PROGRAM): $(C_SRCS)
	$(CC) -o $(PROGRAM) $(C_SRCS)

$(C_SRCS): $(H_SRCS)

clean:
	$(RM) *.o *~ $(PROGRAM)
