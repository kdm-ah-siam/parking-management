CC     = gcc
CFLAGS = -Wall -std=c11 -I/opt/homebrew/include
LFLAGS = -L/opt/homebrew/lib
RLIBS  = -lraylib -framework OpenGL -framework Cocoa -framework IOKit \
         -framework CoreAudio -framework CoreVideo
TARGET = parking_gui
SRCS   = main.c ui.c slots.c vehicle.c fileio.c
OBJS   = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^ $(RLIBS)

main.o:    main.c parking.h ui.h
	$(CC) $(CFLAGS) -c main.c

ui.o:      ui.c parking.h ui.h
	$(CC) $(CFLAGS) -c ui.c

slots.o:   slots.c parking.h
	$(CC) $(CFLAGS) -c slots.c

vehicle.o: vehicle.c parking.h
	$(CC) $(CFLAGS) -c vehicle.c

fileio.o:  fileio.c parking.h
	$(CC) $(CFLAGS) -c fileio.c

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean
