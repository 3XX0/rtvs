TARGET_SEED := seed

CC 	= $(CROSS_PREFIX)gcc
CFLAGS  = -std=c99 -W -Wall -Wextra -pipe -O3 -D_POSIX_C_SOURCE=200112L -DUSE_VP9 -DLITTLE_ENDIAN -I ./webm.libvpx $(EXTRA_CFLAGS)
LDFLAGS = -L ./webm.libvpx -lvpx -lswscale $(EXTRA_LDFLAGS)

RM := rm -f

SRC_SEED = seed.c \
	   capture.c \
	   encode.c \
	   mux.c \
	   packetizer.c \
	   bed.c \
	   frame.c \

OBJ_SEED = $(SRC_SEED:.c=.o)

all : $(TARGET_SEED)

$(TARGET_SEED) : $(OBJ_SEED)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET_SEED) $(OBJ_SEED)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(TARGET_SEED) $(OBJ_SEED)
