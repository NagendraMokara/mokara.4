1. To compile use make

$ make
	gcc oss.c -o oss
	gcc user.c -o user
  
2. To execute:

$ ./oss -n 20 osslog.txt

For clean:
	/bin/rm -f *.o oss user osslog.txt 

