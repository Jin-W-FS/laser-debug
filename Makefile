TARGET := laser
CFLAGS := -DDEBUG -DTEST -ggdb

all:$(TARGET)

$(TARGET): laser.o serial.o

clean:
	-rm *.o $(TARGET)

all-clean: clean
	-rm *~
