cc = clang++
cflags += -shared -fPIC

src = http_client.cpp
lib = libhttp_client.so
# lib = lib$(src:.cpp=.so)
BUILDDIR = build

all: $(BUILDDIR)/$(lib) example

$(BUILDDIR)/%.so: $(src)
		mkdir -p $(BUILDDIR)
		$(cc) $(cflags) $(ldflags) $< -o $@ 

example: $(BUILDDIR)/$(lib)
		$(cc) example.cpp -o $(BUILDDIR)/example -L$(BUILDDIR) -lhttp_client

run:
		LD_LIBRARY_PATH=$(BUILDDIR) $(BUILDDIR)/example

# clangd_flags:
# 	@echo $(cflags) $(ldflags) | tr [:space:] '\n' > compile_flags.txt

clean:
		rm -f $(BUILDDIR)/$(lib)
		rm -f $(BUILDDIR)/example

.PHONY: all test run