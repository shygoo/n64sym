CC=g++
CFLAGS=-static -I./include -O3 -s -ffunction-sections -fdata-sections -fno-ident

LD=g++
LDFLAGS=-s -Wl,--gc-sections,-lm

SDIR=src
ODIR=obj
BDIR=bin

SFILES=$(wildcard $(SDIR)/*.cpp)
OFILES=$(patsubst $(SDIR)/%.cpp,$(ODIR)/%.o,$(SFILES))

TARGET=$(BDIR)/n64sym

.PHONY: all n64sym elf2pj64 elftest clean

all: n64sym elf2pj64 elftest

##############

n64sym: $(TARGET)

$(TARGET): $(OFILES) | $(BDIR)
	$(LD) $(LDFLAGS) $(OFILES) -o $(TARGET)

$(ODIR)/%.o: $(SDIR)/%.cpp | $(ODIR)
	$(CC) $(CFLAGS) -c $^ -o $@

$(ODIR):
	mkdir $(ODIR)

$(BDIR):
	mkdir $(BDIR)

##############

elf2pj64: $(BDIR)/elf2pj64

$(ODIR)/elf2pj64.o: $(SDIR)/elf2pj64/elf2pj64.cpp | $(ODIR)
	$(CC) $(CFLAGS) -c $(SDIR)/elf2pj64/elf2pj64.cpp -o $(ODIR)/elf2pj64.o

$(BDIR)/elf2pj64: $(ODIR)/elf2pj64.o $(ODIR)/elfutil.o | $(BDIR)
	$(LD) $(LDFLAGS) $(ODIR)/elfutil.o $(ODIR)/elf2pj64.o -o $(BDIR)/elf2pj64

##############

elftest: $(BDIR)/elftest

$(ODIR)/elftest.o: $(SDIR)/elftest/elftest.cpp | $(ODIR)
	$(CC) $(CFLAGS) -c $(SDIR)/elftest/elftest.cpp -o $(ODIR)/elftest.o

$(BDIR)/elftest: $(ODIR)/elftest.o $(ODIR)/elfutil.o $(ODIR)/arutil.o | $(BDIR)
	$(LD) $(LDFLAGS) $(ODIR)/elfutil.o $(ODIR)/arutil.o $(ODIR)/elftest.o -o $(BDIR)/elftest

##############

clean:
	rm -f n64sym
	rm -rf $(ODIR)
	rm -rf $(BDIR)