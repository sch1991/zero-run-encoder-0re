CC = gcc
CFLAGS = -IZeroRunEncoder.0re/include -pthread

SRCDIR = ZeroRunEncoder.0re/src
OBJDIR = ZeroRunEncoder.0re/build
TARGET = ZeroRunEncoder.0re/ZeroRunEncoder

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)