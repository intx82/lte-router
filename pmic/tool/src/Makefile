#=============================================================================#
# ARM makefile
#
# author: Freddie Chopin, http://www.freddiechopin.info/
# last change: 2012-01-07
#
# this makefile is based strongly on many examples found in the network
#=============================================================================#

#=============================================================================#
# toolchain configuration
#=============================================================================#

#define Build/Compile
#	$(MAKE) -C $(PKG_BUILD_DIR) \
#		CC="$(TARGET_CC)" \
#		CFLAGS="$(TARGET_CFLAGS) -Wall"
#endef

TOOLCHAIN ?= 

CXX = $(TOOLCHAIN)g++
CC = $(TOOLCHAIN)gcc
AS = $(TOOLCHAIN)gcc
OBJCOPY = $(TOOLCHAIN)objcopy
OBJDUMP = $(TOOLCHAIN)objdump
SIZE = $(TOOLCHAIN)size

#=============================================================================#
# project configuration
#=============================================================================#

# project name
PROJECT = pmicctrl

# C++ definitions (e.g. "-Dsymbol_with_value=0xDEAD -Dsymbol_without_value")
CXX_DEFS = 

# C definitions
C_DEFS = 


# output folder (absolute or relative path, leave empty for in-tree compilation)
OUT_DIR = out

# ASM definitions
AS_DEFS =

# include directories (absolute or relative paths to additional folders with
# headers, current folder is always included)
INC_DIRS = 

# library directories (absolute or relative paths to additional folders with
# libraries)
LIB_DIRS =

# libraries (additional libraries for linking, e.g. "-lm -lsome_name" to link
# math library libm.a and libsome_name.a)
LIBS = -lubox -lubus -ljson-c -lblobmsg_json

# additional directories with source files (absolute or relative paths to
# folders with source files, current folder is always included)
SRCS_DIRS = 

# extension of C++ files
CXX_EXT = cpp

# wildcard for C++ source files (all files with CXX_EXT extension found in
# current folder and SRCS_DIRS folders will be compiled and linked)
CXX_SRCS = $(wildcard $(patsubst %, %/*.$(CXX_EXT), . $(SRCS_DIRS)))

# extension of C files
C_EXT = c

# wildcard for C source files (all files with C_EXT extension found in current
# folder and SRCS_DIRS folders will be compiled and linked)
C_SRCS = $(wildcard $(patsubst %, %/*.$(C_EXT), . $(SRCS_DIRS)))

# extension of ASM files
AS_EXT = s

# wildcard for ASM source files (all files with AS_EXT extension found in
# current folder and SRCS_DIRS folders will be compiled and linked)
AS_SRCS = $(wildcard $(patsubst %, %/*.$(AS_EXT), . $(SRCS_DIRS)))

# optimization flags ("-O0" - no optimization, "-O1" - optimize, "-O2" -
# optimize even more, "-Os" - optimize for size or "-O3" - optimize yet more) 
OPTIMIZATION = -O1

# define warning options here
CXX_WARNINGS = -Wall -Wextra -pedantic
C_WARNINGS = -Wall -Wstrict-prototypes -Wextra -pedantic

# C++ language standard ("c++98", "gnu++98" - default, "c++0x", "gnu++0x")
CXX_STD = c++17

# C language standard ("c89" / "iso9899:1990", "iso9899:199409",
# "c99" / "iso9899:1999", "gnu89" - default, "gnu99")
C_STD = gnu17

#=============================================================================#
# set the VPATH according to SRCS_DIRS
#=============================================================================#

VPATH = $(SRCS_DIRS)

#=============================================================================#
# when using output folder, append trailing slash to its name
#=============================================================================#

ifeq ($(strip $(OUT_DIR)), )
	OUT_DIR_F =
else
	OUT_DIR_F = $(strip $(OUT_DIR))/
endif

#=============================================================================#
# various compilation flags
#=============================================================================#

# flags for C++ compiler
CXX_FLAGS = -std=$(CXX_STD) -g -c -fmessage-length=0 -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -fverbose-asm  -funsigned-char -funsigned-bitfields -Wa,-ahlms=$(OUT_DIR_F)$(notdir $(<:.$(CXX_EXT)=.lst))

# flags for C compiler
C_FLAGS = -std=$(C_STD) -g -c -fmessage-length=0 -ffunction-sections -fdata-sections  -fshort-enums -fverbose-asm  -funsigned-char -funsigned-bitfields -Wa,-ahlms=$(OUT_DIR_F)$(notdir $(<:.$(C_EXT)=.lst))

# flags for assembler
AS_FLAGS = -g -mapcs -Wa,-amhls=$(OUT_DIR_F)$(notdir $(<:.$(AS_EXT)=.lst))

# flags for linker
LD_FLAGS = -g -ffunction-sections -fdata-sections -Wl,-gc-sections -Wl,-Map=$(OUT_DIR_F)$(PROJECT).map,--cref,--no-warn-mismatch

#=============================================================================#
# do some formatting
#=============================================================================#

CXX_OBJS = $(addprefix $(OUT_DIR_F), $(notdir $(CXX_SRCS:.$(CXX_EXT)=.o)))
C_OBJS = $(addprefix $(OUT_DIR_F), $(notdir $(C_SRCS:.$(C_EXT)=.o)))
AS_OBJS = $(addprefix $(OUT_DIR_F), $(notdir $(AS_SRCS:.$(AS_EXT)=.o)))
OBJS = $(AS_OBJS) $(C_OBJS) $(CXX_OBJS) $(USER_OBJS)
DEPS = $(OBJS:.o=.d)
INC_DIRS_F = -I. $(patsubst %, -I%, $(INC_DIRS))
LIB_DIRS_F = $(patsubst %, -L%, $(LIB_DIRS))

ELF = $(OUT_DIR_F)$(PROJECT).elf
#BIN = $(OUT_DIR_F)$(PROJECT).bin
#LSS = $(OUT_DIR_F)$(PROJECT).lss
#DMP = $(OUT_DIR_F)$(PROJECT).dmp

# format final flags for tools, request dependancies for C++, C and asm
CXX_FLAGS_F = $(OPTIMIZATION) $(CXX_WARNINGS) $(CXX_FLAGS)  $(CXX_DEFS) -MD -MP -MF $(OUT_DIR_F)$(@F:.o=.d) $(INC_DIRS_F)
C_FLAGS_F = $(OPTIMIZATION) $(C_WARNINGS) $(C_FLAGS) $(C_DEFS) -MD -MP -MF $(OUT_DIR_F)$(@F:.o=.d) $(INC_DIRS_F)
AS_FLAGS_F = $(AS_FLAGS) $(AS_DEFS) -MD -MP -MF $(OUT_DIR_F)$(@F:.o=.d) $(INC_DIRS_F)
LD_FLAGS_F = $(LD_FLAGS) $(LIB_DIRS_F)

#contents of output directory
GENERATED = $(wildcard $(patsubst %, $(OUT_DIR_F)*.%, bin d dmp elf lss lst map o))


#=============================================================================#
# make all
#=============================================================================#

#all : | make_version make_output_dir $(ELF) $(LSS) $(DMP) $(BIN) print_size copy_out
all : | make_output_dir $(ELF) print_size copy_out

# make object files dependent on Makefile
$(OBJS) : Makefile
# make .elf file dependent on linker script
$(ELF) : $(LD_SCRIPT)

#-----------------------------------------------------------------------------#
# linking - objects -> elf
#-----------------------------------------------------------------------------#

$(ELF) : $(OBJS)	
	$(CXX) $(LD_FLAGS_F) $(OBJS) $(LIBS) -o $@	

#-----------------------------------------------------------------------------#
# compiling - C++ source -> objects
#-----------------------------------------------------------------------------#

$(OUT_DIR_F)%.o : %.$(CXX_EXT)	
	$(CXX) -c $(CXX_FLAGS_F) $< -o $@
	

#-----------------------------------------------------------------------------#
# compiling - C source -> objects
#-----------------------------------------------------------------------------#

$(OUT_DIR_F)%.o : %.$(C_EXT)	
	$(CC) -c $(C_FLAGS_F) $< -o $@

#-----------------------------------------------------------------------------#
# assembling - ASM source -> objects
#-----------------------------------------------------------------------------#

$(OUT_DIR_F)%.o : %.$(AS_EXT)	
	$(AS) -c $(AS_FLAGS_F) $< -o $@	

#-----------------------------------------------------------------------------#
# memory images - elf -> bin
#-----------------------------------------------------------------------------#
# $(BIN) : $(ELF)	
# 	$(OBJCOPY) -O binary $< $@	

#-----------------------------------------------------------------------------#
# memory dump - elf -> dmp
#-----------------------------------------------------------------------------#

# $(DMP) : $(ELF)	
# 	$(OBJDUMP) -x --syms $< > $@	

#-----------------------------------------------------------------------------#
# extended listing - elf -> lss
#-----------------------------------------------------------------------------#

# $(LSS) : $(ELF)	
# 	$(OBJDUMP) -S $< > $@	

#-----------------------------------------------------------------------------#
# extended listing - elf -> hex
#-----------------------------------------------------------------------------#

# $(HEX) : $(ELF)
# 	$(OBJCOPY) -O ihex -R .eeprom -R .nwram  $< $@

#-----------------------------------------------------------------------------#
# print the size of the objects and the .elf file
#-----------------------------------------------------------------------------#

print_size :	
	$(SIZE) -B -t --common $(OBJS) $(USER_OBJS)
	$(SIZE) -B $(ELF)	


copy_out:	
	cp $(OUT_DIR)/$(PROJECT).elf ../$(PROJECT)



#-----------------------------------------------------------------------------#
# create the desired output directory
#-----------------------------------------------------------------------------#

make_output_dir :
	mkdir -p $(OUT_DIR)

make_version:
	echo "#define VERSION \"" > tmp.hpp
	date +R%y%m%d- >> tmp.hpp
	git rev-parse --abbrev-ref HEAD >> tmp.hpp
	echo "-" >> tmp.hpp
	git rev-parse HEAD >> tmp.hpp
	echo "\"" >> tmp.hpp
	tr -d '\r\n' < tmp.hpp > version.hpp
	rm -rf tmp.hpp

#=============================================================================#
# make clean
#=============================================================================#

clean:
	rm -rf $(OUT_DIR)


#=============================================================================#
# global exports
#=============================================================================#

.PHONY: all clean dependents

.SECONDARY:

# include dependancy files
-include $(DEPS)
