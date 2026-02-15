# optional arguments:
# 	UNIGEN=NO
# 	BUILD=RELEASE/DEBUG
# 	STATIC=YES

# Default to building without UniGen/Scalmc unless explicitly overridden.
UNIGEN ?= NO
STATIC ?= NO

# Architecture required by ABC
CPP_FLAGS += -DLIN64

BFSS 	= bfss
RCNF  	= readCnf
ORDR  	= genVarOrder
VRFY  	= verify
RSUB    = revsub
UNATE   = unate

ABC_PATH = ./dependencies/abc
SCALMC_PATH = ./dependencies/scalmc

ifndef CXX
CXX = g++
endif

SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

TARGET_RCNF  = $(BINDIR)/$(RCNF)
TARGET_BFSS  = $(BINDIR)/$(BFSS)
TARGET_ORDR  = $(BINDIR)/$(ORDR)
TARGET_VRFY  = $(BINDIR)/$(VRFY)
TARGET_RSUB  = $(BINDIR)/$(RSUB)
TARGET_UNATE = $(BINDIR)/$(UNATE)

ABC_INCLUDES = -I $(ABC_PATH) -I $(ABC_PATH)/src
UGEN_INCLUDES = -I $(SCALMC_PATH)/build/cmsat5-src/ -I $(SCALMC_PATH)/src/
LIB_DIRS = -L $(SCALMC_PATH)/build/lib/ -L $(ABC_PATH)/
DIR_INCLUDES = $(ABC_INCLUDES) $(UGEN_INCLUDES) $(LIB_DIRS)

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
BOOST_PREFIX ?= $(shell brew --prefix 2>/dev/null)
ifeq ($(BOOST_PREFIX),)
BOOST_PREFIX = /opt/homebrew
endif
CPP_FLAGS += -I$(BOOST_PREFIX)/include
CPP_FLAGS += -Wno-c++11-narrowing
LIB_UGEN   = -lcryptominisat5
LIB_ABC    = -labc
LIB_COMMON = -lm -ldl -lreadline -lpthread -lz
else
LIB_UGEN   = -Wl,-Bdynamic -lcryptominisat5
LIB_ABC    = -Wl,-Bstatic  -labc
LIB_COMMON = -Wl,-Bdynamic -lm -ldl -lreadline -ltermcap -lpthread -fopenmp -lrt -Wl,-Bdynamic -lboost_program_options -Wl,-Bdynamic -lz
endif

ifeq ($(STATIC),YES)
ifeq ($(UNAME_S),Darwin)
$(error Static linking is not supported on macOS toolchains here. Use a Linux build environment.)
endif
ifeq ($(UNIGEN),NO)
LDFLAGS += -static -static-libstdc++ -static-libgcc
else
$(error Static build requires UNIGEN=NO to avoid dynamic scalmc dependencies.)
endif
endif

ifeq ($(UNIGEN), NO)
CPP_FLAGS += -std=c++11 -DNO_UNIGEN
LFLAGS    = $(DIR_INCLUDES) $(LIB_ABC) $(LIB_COMMON) $(LDFLAGS)
else
CPP_FLAGS += -std=c++11
LFLAGS    = $(DIR_INCLUDES) $(LIB_ABC) $(LIB_UGEN) $(LIB_COMMON) $(LDFLAGS)
endif

ifeq ($(BUILD),DEBUG)
CPP_FLAGS += -O0 -g
else ifeq ($(BUILD),RELEASE)
CPP_FLAGS += -O3 -s -DNDEBUG
else
CPP_FLAGS += -O3
endif

COMMON_SOURCES  = $(SRCDIR)/nnf.cpp $(SRCDIR)/helper.cpp

BFSS_SOURCES  = $(SRCDIR)/bfss.cpp $(COMMON_SOURCES)
ORDR_SOURCES  = $(SRCDIR)/genVarOrder.cpp $(COMMON_SOURCES)
RCNF_SOURCES  = $(SRCDIR)/readCnf.cpp
VRFY_SOURCES  = $(SRCDIR)/verify.cpp
RSUB_SOURCES  = $(SRCDIR)/revsub.cpp
UNATE_SOURCES = $(SRCDIR)/unateQdimacs.cpp $(COMMON_SOURCES)
READCNF_LIB_OBJ = $(OBJDIR)/readCnf_lib.o
BFSS_OBJECTS  = $(BFSS_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
ORDR_OBJECTS  = $(ORDR_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
UNATE_OBJECTS = $(UNATE_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o) $(READCNF_LIB_OBJ)
ALL_OBJECTS   = $(sort $(BFSS_OBJECTS) $(ORDR_OBJECTS) $(filter-out $(READCNF_LIB_OBJ),$(UNATE_OBJECTS)))

.PHONY: all clean remove bfss readCnf genVarOrder verify revsub unate directories
all: bfss readCnf genVarOrder verify revsub unate
bfss: directories $(TARGET_BFSS)
genVarOrder: directories $(TARGET_ORDR)
readCnf: directories $(TARGET_RCNF)
verify: directories $(TARGET_VRFY)
revsub: directories $(TARGET_RSUB)
unate: directories $(TARGET_UNATE)

directories:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)

$(TARGET_BFSS): $(BFSS_OBJECTS)
	$(CXX) $(CPP_FLAGS) -o $@ $^ $(LFLAGS)
	@echo "Built Target! - bfss"

$(TARGET_ORDR): $(ORDR_OBJECTS)
	$(CXX) $(CPP_FLAGS) -o $@ $^ $(LFLAGS)
	@echo "Built Target! - genVarOrder"

$(TARGET_RCNF): $(RCNF_SOURCES)
	$(CXX) $(CPP_FLAGS) $^ -o $@
	@echo "Compiled "$^" successfully!"
	@echo "Built Target! - readCnf"

$(TARGET_VRFY): $(VRFY_SOURCES)
	$(CXX) $(CPP_FLAGS) $^ -o $@ $(DIR_INCLUDES) $(LIB_ABC) $(LIB_COMMON)
	@echo "Compiled "$^" successfully!"
	@echo "Built Target! - verify"

$(TARGET_RSUB): $(RSUB_SOURCES)
	$(CXX) $(CPP_FLAGS) $^ -o $@
	@echo "Compiled "$^" successfully!"
	@echo "Built Target! - revsub"

$(TARGET_UNATE): $(UNATE_OBJECTS)
	$(CXX) $(CPP_FLAGS) -o $@ $^ $(LFLAGS)
	@echo "Built Target! - unate"

$(ALL_OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	$(CXX) $(CPP_FLAGS) -c $^ -o $@  $(LFLAGS)
	@echo "Compiled "$<" successfully!"

$(READCNF_LIB_OBJ): $(SRCDIR)/readCnf.cpp
	$(CXX) $(CPP_FLAGS) -DREADCNF_LIB -c $^ -o $@
	@echo "Compiled "$<" successfully!"

clean:
	@$(RM) $(BFSS_OBJECTS)
	@echo "Cleanup complete!"

remove: clean
	@$(RM) $(TARGET_BFSS) $(TARGET_ORDR) $(TARGET_RCNF) $(TARGET_VRFY) $(TARGET_RSUB) $(TARGET_UNATE)
	@echo "Executable removed!"
