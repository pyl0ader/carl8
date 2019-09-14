HDR = video.h input.h interpreter.h error.h
SRC = main.c video.c input.c error.c interpreter.c
OBJ = ${SRC:.c=.o}

chipper: ${OBJ} 
	cc -g -lSDL2 -o $@ $^
	
.c.o:
	cc -g -c $<

${OBJ}: ${HDR}

clean:
	rm chipper *.o
