TARGET := rtvs

CC 	= $(CROSS_PREFIX)gcc
RM := rm -f

PREPROC = HAS_VP8_HW_SUPPORT \
		  LITTLE_ENDIAN \
		  WITH_MMAL_CAPTURE \
		  #BIG_ENDIAN \
		  #WITH_V4L2_CAPTURE \
		  #HAS_VP9_SUPPORT \

INC = /opt/userland/ \
	  /opt/vc/include/interface/vcos/pthreads/ \
	  /opt/vc/include/ \
	  /opt/vc/include/interface/vmcs_host/linux/ \

LIB = /opt/vc/lib/ \

CFLAGS  = -std=c99 -W -Wall -Wextra -pipe -O3 -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=700 $(EXTRA_CFLAGS)
LDFLAGS = -lvpx $(EXTRA_LDFLAGS)

# Ubuntu ldflag specific
UNAME := $(shell uname -v)
ifneq (,$(findstring Ubuntu, $(UNAME)))
	LDFLAGS := -Wl,--no-as-needed -lrt $(LDFLAGS)
endif

SRC =  main.c \
	   encode.c \
	   mux.c \
	   packetizer.c \
	   bed.c \
	   frame.c \
	   rtp.c \

ifneq (,$(findstring WITH_V4L2_CAPTURE, $(PREPROC)))
	SRC += capture_v4l2.c
	LDFLAGS += -lswscale
endif
ifneq (,$(findstring WITH_MMAL_CAPTURE, $(PREPROC)))
	SRC += capture_mmal.c
	CFLAGS += -fgnu89-inline $(addprefix -I , $(INC))
	LDFLAGS += $(addprefix -L , $(LIB)) -lpthread -lmmal -lbcm_host
endif

OBJ = $(SRC:.c=.o)

all : $(TARGET)

$(TARGET) : $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJ)

.c.o:
	$(CC) $(CFLAGS) $(addprefix -D, $(PREPROC)) -c -o $@ $<

clean:
	$(RM) $(TARGET) $(OBJ)
