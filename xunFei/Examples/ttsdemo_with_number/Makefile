#common makefile header

export BASEDIR=$(PWD)/../../prj
include $(BASEDIR)/scripts/configure.mk

MODULE_EXEC	= ttsdemo_with_number
MODULE_TARGET	= $(MODULE_EXEC)
LOCAL_INCLUDE	= -I$(BASEDIR)/../include
LOCAL_FLAGS	= $(LOCAL_INCLUDE) $(PIC_FLAG)
LIBS	:= -L$(BASEDIR-BIN) -lmsc -ldl -lpthread

MODULE_OBJ   = \
	$(MODULE_OUTPUT)/ttsdemo_with_number.o

#common makefile foot
include $(BASEDIR)/scripts/rules.mk
