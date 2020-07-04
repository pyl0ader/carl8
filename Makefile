INC=   -lm -lSDL2
FLAGS= -g ${INC}

HDR=input.h video.h logError.h interpreter.h util.h assembly.h 
SRC=input.c video.c logError.c interpreter.c util.c assembly.c
OBJ=${SRC:.c=.o}

carl8: ${OBJ} main.c
	gcc -o $@ main.c ${OBJ} ${FLAGS}

.c.o:
	gcc -c ${FLAGS} $<

${OBJ}: ${HDR}

clean:
	rm *.o carl8
