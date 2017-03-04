
target := ctrl.bin
CC := g++
CPPFLAGS += -std=c++11

srcs := $(shell find -name "*.cpp")
objs := $(srcs:.cpp=.o)
deps := $(srcs:.cpp=.d)

all: $(target)

-include ../objs.mk

test:
	echo $(com_obj)

$(target): $(objs) $(com_obj)
	$(CC) -o $@ $^

-include $(deps)

%.d: %.cpp
	$(CC) -MM $< > $@

clean:
	rm *.d *.o *.bin


