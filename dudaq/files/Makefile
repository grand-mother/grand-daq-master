APP = dudaq

# Add any other object files to this list below
APP_OBJS = dudaq.o scope.o ad_shm.o

#CFLAGS = -W -Wunused-variable
all: build

build: $(APP)

clean:
	rm -f *.o
	rm -f dudaq

$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) -O3 -DSCOPE_V4   -o $@ $(APP_OBJS) $(LDLIBS)

