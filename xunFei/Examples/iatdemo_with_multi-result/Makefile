#common makefile header

export BASEDIR=$(PWD)/../../prj
include $(BASEDIR)/scripts/configure.mk

MODULE_EXEC	= iatdemo_with_multi-result
MODULE_TARGET	= $(MODULE_EXEC)
LOCAL_INCLUDE	= -I$(BASEDIR)/../include
LOCAL_FLAGS	= $(LOCAL_INCLUDE) $(PIC_FLAG)
LIBS	:= -L$(BASEDIR-BIN) -lmsc -ldl -lpthread

MODULE_OBJ   = \
	$(MODULE_OUTPUT)/iatdemo_with_multi-result.o

#common makefile foot
include $(BASEDIR)/scripts/rules.mk
