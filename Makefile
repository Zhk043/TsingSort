################################################################################
# makefile
################################################################################

ARCH ?= linux_a53
ROOT_PATH ?= ../..
EXPORT_DIR := $(ROOT_PATH)

ifeq ($(strip $(ARCH)),linux_a53)
	CC_PREFIX ?= aarch64-none-linux-gnu-
	CPU_FLAGS ?= -march=armv8-a -mcpu=cortex-a53
	DEF_FLAGS ?= -DLINUX_PAL -DRESULT_HWC
	DEF_FLAGS += -DFirst_Version_Id=3000
	DEF_FLAGS += -DSecond_Version_Id=1010
	# DEF_FLAGS += -DOPEN_LOG
	INC_FLAGS ?=
	BUILD_PATH ?= build
	PLATFORM := linux_a53
	X_LIB_L_FLAGS := -lrne_pal_linux_a53,-lgcc,-lc,-lm,-lpthread,-lopencv_core,-lopencv_imgcodecs,-lopencv_highgui,-lopencv_imgproc,-lopencv_dnn,-lopencv_calib3d,-lopencv_features2d,-lopencv_flann,-lopencv_video,-lopencv_videoio
	LIB_SUB_DIR ?= 64bit
else
	CC_PREFIX ?=
	CPU_FLAGS ?= -m64
	INC_FLAGS ?=
	DEF_FLAGS ?= -DSIMULATOR -DRESULT_HWC
	BUILD_PATH ?= build_sim
	PLATFORM := sim
	X_LIB_L_FLAGS := -lrne_pal_sim,-lrne_vm,-lgcc,-lc,-lm,-lpthread
	LIB_SUB_DIR ?= 64bit
endif

INC_FLAGS += -I$(EXPORT_DIR)/include/  -I$(EXPORT_DIR)/include/opencv2 -I./include/

LIBS := -L$(EXPORT_DIR)/libs/$(LIB_SUB_DIR)  -Wl,--start-group,-lrne_rt_g3,-lrne_common,-lknight1.3,$(X_LIB_L_FLAGS),-end-group,-lgomp
CC_FLAGS := $(CPU_FLAGS)
CC_FLAGS += $(DEF_FLAGS)
CC_FLAGS += $(OPT_LEVEL)

RM =rm -rf

ifeq ($(MAKECMDGOALS),release)
	BUILD_DIR = $(BUILD_PATH)/bin
	OPT_LEVEL ?= -Os
else ifeq ($(MAKECMDGOALS),debug)
	BUILD_DIR = $(BUILD_PATH)/Debug
	OPT_LEVEL ?= -O0
else
	BUILD_DIR = $(BUILD_PATH)/bin
	OPT_LEVEL ?= -Os
endif

ifeq ($(OPT_LEVEL),-O0)
    CC_FLAGS += -g
endif

CC_FLAGS += $(OPT_LEVEL)
#CC_FLAGS += -Werror
CC_FLAGS += -fPIC -ffunction-sections -fdata-sections
CC_FLAGS += $(INC_FLAGS)
CC_FLAGS += -fopenmp

LINK_FLAGS := $(OPT_LEVEL)


SRC_DIR := $(shell find core -type d)
SRC_DIR += $(shell find core -type l)

S_SRCS = $(wildcard $(foreach n,$(SRC_DIR),$(n)/*.s))
S_OBJS = $(addprefix $(BUILD_DIR)/, ${S_SRCS:.s=.o})

CC_SRCS = $(wildcard $(foreach n,$(SRC_DIR),$(n)/*.c))
CC_OBJS = $(addprefix $(BUILD_DIR)/, ${CC_SRCS:.c=.o})

CPP_SRCS = $(wildcard $(foreach n,$(SRC_DIR),$(n)/*.cpp))
CPP_OBJS = $(addprefix $(BUILD_DIR)/, ${CPP_SRCS:.cpp=.o})

OBJS = $(CPP_OBJS) $(CC_OBJS) $(S_OBJS)

SRC_DEPS = $(patsubst %.o, %.d, $(OBJS))

TARGET_NAME = sort
TARGET = $(BUILD_DIR)/$(TARGET_NAME)
# ELFSIZE = $(TARGET).size

ifneq ($(CC_SRCS),)
	LINKER  := $(CC_PREFIX)g++
else ifeq ($(CC_PREFIX),)
	LINKER  := $(CC_PREFIX)g++
else
	LINKER  := $(CC_PREFIX)g++
endif

# All Target
release: all
debug: all
all: pre_build $(TARGET) $(BUILD_DIR)/deploy

# Tool invocations
$(TARGET): $(OBJS) $(LINK_SCRIPT)
	@echo 'Building target: $@'
	$(LINKER) $(CPU_FLAGS) $(LINK_FLAGS) -o $@ $(OBJS) $(LIBS)
	@echo ' '

%.size: %
	@echo 'Building target: $@'
	$(CC_PREFIX)size $< |tee $@
	@echo ' '

$(BUILD_DIR)/%.o: %.c
	@echo 'Building file: $<'
	$(CC_PREFIX)gcc -Wall -std=c99 -c -fmessage-length=0 $(CC_FLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo ' '

$(BUILD_DIR)/%.o: %.cpp
	@echo 'Building file: $<'
	$(CC_PREFIX)g++ -Wall -std=c++17 -c -fmessage-length=0 $(CC_FLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo ' '

$(BUILD_DIR)/%.o: %.s
	@echo 'Building file: $<'
	$(CC_PREFIX)g++ -Wall -std=c++17 -c -fmessage-length=0 $(CC_FLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo ' '

$(BUILD_DIR)/deploy: $(BUILD_DIR)/tool/deploy.o $(BUILD_DIR)/core/Config.o $(BUILD_DIR)/core/cJSON.o
	@echo 'Building target: $@'
	$(LINKER) $(CPU_FLAGS) $(LINK_FLAGS) -o $@ $(BUILD_DIR)/tool/deploy.o $(BUILD_DIR)/core/Config.o $(BUILD_DIR)/core/cJSON.o ${LIBS}
	# -L$(EXPORT_DIR)/libs/$(LIB_SUB_DIR) -Wl,--start-group,-lrne_rt_g3,-lrne_common,$(X_LIB_L_FLAGS),-end-group,-lgomp

$(BUILD_DIR)/tool/deploy.o: tool/deploy.cpp core/Config.cpp core/cJSON.c
	$(CC_PREFIX)g++ -Wall -std=c++17 -c -fmessage-length=0 $(CC_FLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"

-include $(SRC_DEPS)

# Other Targets
clean:
	@-$(RM) $(SRC_DEPS) $(OBJS) $(TARGET)
	@-$(RM) build*

run: all
	$(TARGET)

pre_build: mkdir_build


mkdir_build:
	@if [ ! -d $(BUILD_DIR) ]; then\
		mkdir -p $(addprefix $(BUILD_DIR)/, $(SRC_DIR));\
	fi
	@if [ ! -d "build/bin/tool" ]; then\
		mkdir -p "build/bin/tool";\
	fi

.PHONY: clean all pre_build mkdir_build

