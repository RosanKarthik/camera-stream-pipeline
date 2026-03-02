camera_stream: main.c v4l2.c gstream.c thread.c
	gcc main.c v4l2.c gstream.c thread.c -o main.o -pthread `pkg-config --cflags --libs gstreamer-1.0 gstreamer-app-1.0`
	./main.o
compile:
	gcc main.c v4l2.c gstream.c thread.c -o main.o -pthread `pkg-config --cflags --libs gstreamer-1.0 gstreamer-app-1.0`
clean:
	rm -f camera_stream