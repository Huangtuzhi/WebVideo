all: webcam_shower
CXX=g++
CXXFLAGS= -c -g -O0 -fPIC -I./ffmpeg/include -L./ffmpeg/lib 

OBJS_SHOWER=  vshow.o shower.o
LIBS_SHOWER= -lavcodec -lswscale -lavutil -lx264 -lz -lX11 -lXext

.cpp.o:
	$(CXX) $(CXXFLAGS) $<

webcam_shower: $(OBJS_SHOWER)
	$(CXX) -o $@ $^ -L./ffmpeg/lib $(LIBS_SHOWER)

clean:
	rm -f *.o
	rm -f webcam_shower
