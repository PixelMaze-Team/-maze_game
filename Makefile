# Makefile
CC = gcc
CFLAGS = -Wall
TARGET = maze_game
OBJS = main.o maze_map.o dot_control.o switch_input.o fnd_timer.o buzzer_alert.o lcd_display.o led_notify.o
all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
%.o: %.c
	$(CC) $(CFLAGS) -c $<
clean:
	rm -f *.o $(TARGET)
