TARGET := laser
CFLAGS := -DDEBUG -ggdb

all:$(TARGET)

$(TARGET): main.o laser.o serial.o

clean:
	-rm *.o $(TARGET)

all-clean: clean
	-rm *~
