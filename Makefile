CC     = gcc
CFLAGS = -Wall -std=c11 -I/opt/homebrew/include
LFLAGS = -L/opt/homebrew/lib
RLIBS  = -lraylib -framework OpenGL -framework Cocoa -framework IOKit \
         -framework CoreAudio -framework CoreVideo
TARGET = parking_gui
SRCS   = main.c ui.c slot-management.c system-logic.c file-saving.c
OBJS   = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^ $(RLIBS)

main.o:    main.c function-define.h ui.h
	$(CC) $(CFLAGS) -c main.c

ui.o:      ui.c function-define.h ui.h
	$(CC) $(CFLAGS) -c ui.c

slot-management.o: slot-management.c function-define.h
	$(CC) $(CFLAGS) -c slot-management.c

system-logic.o: system-logic.c function-define.h
	$(CC) $(CFLAGS) -c system-logic.c

file-saving.o:  file-saving.c function-define.h
	$(CC) $(CFLAGS) -c file-saving.c

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean
