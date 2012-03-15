EMSCRIPTEN = ~/Dev/emscripten
EMCC = $(EMSCRIPTEN)/emcc
BINDINGS_GENERATOR = $(EMSCRIPTEN)/tools/bindings_generator.py
NATIVIZER = $(EMSCRIPTEN)/tools/nativize_llvm.py

all: relooper.js
#relooper.exe

combined.bc: ../Relooper.h ../Relooper.cpp
	$(BINDINGS_GENERATOR) bindings ../Relooper.h > /dev/null
	$(EMCC) -include ../Relooper.h bindings.cpp -o bindings.bc
	$(EMCC) ../Relooper.cpp -I.. -o relooper.bc
	$(EMCC) bindings.bc relooper.bc -o combined.bc

relooper.js: combined.bc
	$(EMCC) -O2 --closure 0 combined.bc --post-js bindings.js -o relooper.js

#relooper.exe: combined.bc
#	$(NATIVIZER) combined.bc

clean:
	rm relooper.js bindings.js bindings.cpp combined.bc relooper.bc bindings.bc bindings.all.h

