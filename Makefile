OBJECTS = ssu_monitor.o
TARGET = ssu_monitor
CC = gcc -g

$(TARGET) : $(OBJECTS)
			  $(CC) -o $@ $^

ssu_monitor.o : ssu_monitor.c
	$(CC) -c $^

clean:
	rm *.o
