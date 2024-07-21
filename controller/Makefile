CC = clang

CFLAGS = -Wall -std=c11 -pedantic -Wextra -g

TARGET = IrrigationManager

SRCS = main.c utils.c weather_utils.c

HEADERS = utils.h weather_utils.h

OBJS=$(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean