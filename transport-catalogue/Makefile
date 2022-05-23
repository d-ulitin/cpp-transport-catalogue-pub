CPP := g++
CPPFLAGS := -c -std=c++17 -Wall -Wextra -Wpedantic -Og -g
LD := $(CPP)
CPPSOURCE := $(wildcard *.cpp)
CPPHEADERS := $(wildcard *.h)
OBJECTS := $(CPPSOURCE:.cpp=.o)
DEPS := $(CPPSOURCE:.cpp=.dep)
TARGET := transport.out

all: deps $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) $(OBJECTS) -o $(TARGET)

%.o: %.cpp
	$(CPP) $(CPPFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: cleanall
cleanall: clean cleandeps

.PHONY: deps
deps: $(DEPS)

%.dep: %.cpp
# 	replace 'main.o: ...' to 'main.o main.dep: ...'
	$(CPP) -MM $< | sed -r 's/^(.*)[.]o:/\1.o \1.dep:/' > $@ 

include $(DEPS)

.PHONY: cleandeps
cleandeps:
	rm -f $(DEPS)

