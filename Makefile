CPP	= @echo " g++ $@"; g++
CC	= @echo " gcc $@"; gcc
LD	= @echo " ld  $@"; ld
AR	= @echo " ar  $@"; ar
RM	= @echo " RM	$@"; rm -f
#STRIP	= @echo " strip  $@"; $(CROSS)strip

# 编译参数
#CFLAGS	+= -I./src
CFLAGS += -Wall -std=c++11
#-O2 -Os
#CFLAGS += -D_REENTRANT -msoft-float
CFLAGS += -g

# 链接参数
#LDFLAGS += "-Wl" -lpthread -lc
LDFLAGS += -lcurl
LDFLAGS += -L.  ##-lenc -lsyslink  -lbinder -llog    -losa -lpdi -lsdk  -lz -lrt -lsplice -lmem -ldec
AFLAGS += -r

# 目录分布
#LIBDIR = ./Libs
BINDIR = .
SRCDIR = .#/src ./src/io



# .cpp源文件目录
SRCS_PATH = $(SRCDIR) 

# 抽取源文件
LIB_SRCS += $(foreach dir,$(SRCS_PATH),$(wildcard $(dir)/*.cpp))

# 获得目标文件名
LIB_OBJS += $(patsubst %.cpp,%.o,$(LIB_SRCS))

# 生成Demo库文件
LIB_APP = curltool.a


# 可执行文件名
DEMO_NAME	= curltool
TARGET_DEMO = $(BINDIR)/$(DEMO_NAME)

# 开始执行
all: $(TARGET_DEMO)

# 生成可执行程序
$(TARGET_DEMO) : $(LIB_APP)
	$(CPP) -o $@ $(LDFLAGS) $^
	#$(CROSS)g++ -o $@ $(LDFLAGS) $^  
	#$(STRIP) $(TARGET)

# 生成Demo库
$(LIB_APP): $(LIB_OBJS)
	$(RM) $@;
#	mkdir -p $(LIBDIR)
	$(AR) $(AFLAGS) $@ $^

# 隐式生成
.c.o:
	$(CC) -c $(CFLAGS) $^ -o $@

.cpp.o:
	$(CPP) -c $(CFLAGS) $^ -o $@

# 清除可执行程序, 中间文件, Demo库文件
clean:
	$(RM) $(LIB_OBJS) $(LIB_APP) $(TARGET_DEMO)

# 清除中间文件
cleanobj:
	$(RM) $(LIB_OBJS)