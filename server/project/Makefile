CC=gcc

dweb.exe: dweb.c dwebsvr.c send_gpio.c gpio_status.c gpio_control.c read.c write.c
	$(CC) -o $@  $^ -lpthread
clean:
	rm -f *.o *.exe dweb simple
