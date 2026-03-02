PROJECT_NAME := OWR #target file name

CC := gcc #compiler
CFLAGS := -g -Wall

FILES := $(wildcard src/*.c)
HEADERS := $(wildcard headers/*.h)
OBJECTS := $(addprefix bin/,$(notdir $(FILES:.c=.o)))

VPATH := src

all: bin $(PROJECT_NAME)

$(PROJECT_NAME): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(PROJECT_NAME) $(OBJECTS)

bin/%.o: %.c | $(HEADERS)
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

bin:
	mkdir -p bin

clean:
	rm -rf bin/* $(PROJECT_NAME)