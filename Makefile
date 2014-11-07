CC = g++
LDFLAGS = -std=c++0x
CFLAGS = -c -O3 -std=c++0x
SOURCES = parser_helper.cpp graph.cpp chronosphere.cpp
OBJECTS = $(SOURCES:.cpp=.o)

ifeq ($(debug),true)
CFLAGS += -DDEBUG
endif

EXECUTABLE = chronosphere

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *o chronosphere
