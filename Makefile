INC = -lSDL2
FLAGS = -g ${INC}

HDR = video.h input.h interpreter.h logError.h
SRC = main.c video.c input.c logError.c interpreter.c
OBJ = ${SRC:.c=.o}

carl8: ${OBJ} 
	cc ${FLAGS} -o $@ $^
	
.c.o:
	cc ${FLAGS} -c $<

${OBJ}: ${HDR}

clean:
	rm carl8 *.o
