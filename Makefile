INC=   -lm -lSDL2 -lSDL2_ttf
FLAGS= -g ${INC}

HDR=input.h beep.h video.h logError.h interpreter.h util.h assembly.h 
SRC=input.c beep.c video.c logError.c interpreter.c util.c assembly.c
OBJ=${SRC:.c=.o}

carl8: ${OBJ} main.c
	gcc -o $@ main.c ${OBJ} ${FLAGS}

.c.o:
	gcc -c ${FLAGS} $<

${OBJ}: ${HDR}

clean:
	rm *.o carl8
