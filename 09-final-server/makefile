CC	:= g++
CXXFLAGS:= -w -std=c++11 -c
LFLAGS	:= -lpthread
BINS 	:= web_final
SRCS	:= $(wildcard *.cpp) # 当前目录下的所有的.cc文件 
OBJS	:= $(SRCS:.cpp=.o) # 将所有的.cc文件名替换为.o

.PHONY: all clean

all:$(BINS)

BINOS	= $(addsuffix .o, $(BINS))
TEMP_OBJ= $(filter-out $(BINOS), $^)

$(BINS):$(OBJS) 
	@echo "正在链接程序......"; \
	$(foreach BIN, $@, $(CC) $(TEMP_OBJ) $(BIN).o $(LFLAGS) -o $(BIN));   

%.d:%.cpp
	@echo "正在生成依赖中......"; \
	rm -f $@; \
	$(CC) -MM $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(SRCS:.cpp=.d)

clean:
	rm -f *.o *.d
	rm -f $(BINS)

# makefile说白了就是拼凑字符串
