all:
	gcc oss.c -o oss
	gcc user.c -o user
clean:
	/bin/rm -f *.o oss user osslog.txt 
