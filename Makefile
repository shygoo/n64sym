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

$(TARGET): $(OFILES) | $(BDIR)
	$(LD) $(LDFLAGS) $(OFILES) -o $(TARGET)

$(ODIR)/%.o: $(SDIR)/%.cpp | $(ODIR)
	$(CC) $(CFLAGS) -c $^ -o $@

$(ODIR):
	mkdir $(ODIR)

$(BDIR):
	mkdir $(BDIR)

clean:
	rm -f n64sym
	rm -rf $(ODIR)
	rm -rf $(BDIR)

.PHONY: n64sym clean