CUSTOMER := customer.c
BARBER := barber.c
LAUNCHER := launcher.c
CC	:= gcc
CUSTOMER_TARGET	:= customer
BARBER_TARGET	:= barber
LAUNCHER_TARGET	:= launcher
LINK	:= -std=c99 -Wall -lpthread -lm

all: $(CUSTOMER_TARGET) $(BARBER_TARGET) $(LAUNCHER_TARGET)

$(CUSTOMER_TARGET): $(CUSTOMER)
	$(CC) -o $(CUSTOMER_TARGET) $(CUSTOMER) $(LINK)

$(BARBER_TARGET): $(BARBER)
	$(CC) -o $(BARBER_TARGET) $(BARBER) $(LINK)

$(LAUNCHER_TARGET): $(LAUNCHER)
	$(CC) -o $(LAUNCHER_TARGET) $(LAUNCHER) $(LINK)

clean:
	rm -f /dev/shm/sem.hdp38_njs76_* $(CUSTOMER_TARGET) $(BARBER_TARGET) $(LAUNCHER_TARGET)