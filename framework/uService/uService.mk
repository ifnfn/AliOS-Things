NAME := uService

$(NAME)_SOURCES += service.c

$(NAME)_INCLUDES := .
	
GLOBAL_INCLUDES += .

$(NAME)_TYPE := framework

GLOBAL_DEFINES += AOS_USERVICE
