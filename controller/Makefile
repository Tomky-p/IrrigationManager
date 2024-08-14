CC = clang

CFLAGS = -Wall -std=c11 -pedantic -Wextra -g

LDFLAGS = -lwiringPi

TARGET = IrrigationManager

SRCS = main.c utils.c weather_utils.c gpio_utils.c

HEADERS = utils.h weather_utils.h gpio_utils.h

OBJS=$(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean