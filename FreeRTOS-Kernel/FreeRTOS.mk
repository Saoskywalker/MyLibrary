#this Lib path
FreeRTOS_path ?= lib/

#define
DEFINES	+=  

#include path
INCDIRS		+= 	-I$(FreeRTOS_path)FreeRTOS-Kernel/portable/GCC/ARM926ejs \
				-I$(FreeRTOS_path)FreeRTOS-Kernel/include \
				-I$(FreeRTOS_path)FreeRTOS-Kernel/user_define \
				# -I$(FreeRTOS_path)FreeRTOS-Kernel/user_define/Supporting_Functions \

#library path
LIBDIRS	+=

#library
LIBS += 

#c source path
SRCDIRS_C += 	$(FreeRTOS_path)FreeRTOS-Kernel \
				$(FreeRTOS_path)FreeRTOS-Kernel/portable/GCC/ARM926ejs \
				$(FreeRTOS_path)FreeRTOS-Kernel/portable/MemMang \
				# $(FreeRTOS_path)FreeRTOS-Kernel/user_define/Supporting_Functions \

#c source files
# usr library src
SRC_C += 

#c++ source path
SRCDIRS_CXX += 

#c++ source files
SRC_CXX += 

#asm source files
SRC_ASM += 
