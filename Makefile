CC=gcc
SRCS=main.c v4l2.c gstream.c thread.c
OBJS=$(SRCS:.c=.o)
CFLAGS += `pkg-config --cflags gstreamer-1.0 gstreamer-app-1.0`
LDFLAGS += -pthread `pkg-config --libs gstreamer-1.0 gstreamer-app-1.0`
TARGET=stream_app
$(TARGET): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $(TARGET)

%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)