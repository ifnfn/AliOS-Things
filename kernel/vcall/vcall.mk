NAME := vcall

$(NAME)_TYPE := kernel
$(NAME)_MBINS_TYPE := share

GLOBAL_INCLUDES += ./mico/include

#default gcc
ifeq ($(COMPILER),)
$(NAME)_CFLAGS      += -Wall -Werror
else ifeq ($(COMPILER),gcc)
$(NAME)_CFLAGS      += -Wall -Werror
endif

ifeq ($(HOST_ARCH),ARM968E-S)
$(NAME)_CFLAGS += -marm
endif

vcall ?= rhino

ifeq ($(vcall),posix)
GLOBAL_DEFINES += VCALL_POSIX

$(NAME)_SOURCES += \
    aos/aos_posix.c
endif

ifeq ($(vcall),rhino)
GLOBAL_DEFINES += VCALL_RHINO
$(NAME)_COMPONENTS += rhino

$(NAME)_SOURCES += \
    aos/aos_rhino.c
endif

