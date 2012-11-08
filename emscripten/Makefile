EMSCRIPTEN = ~/Dev/emscripten
EMCC = $(EMSCRIPTEN)/emcc
BINDINGS_GENERATOR = $(EMSCRIPTEN)/tools/bindings_generator.py
NATIVIZER = $(EMSCRIPTEN)/tools/nativize_llvm.py

all: relooper.js
#relooper.exe

combined.bc: ../Relooper.h ../Relooper.cpp
	$(BINDINGS_GENERATOR) bindings ../Relooper.h -- '{ "ignored": "Shape::IdCounter,LabeledShape,Block::IdCounter,Shape::Type" }' > /dev/null
	$(EMCC) -include ../Relooper.h bindings.cpp -o bindings.bc
	$(EMCC) ../Relooper.cpp -I.. -o relooper.bc
	$(EMCC) bindings.bc relooper.bc -o combined.bc

relooper.js: combined.bc
	$(EMCC) -O2 combined.bc --post-js bindings.js -o relooper.raw.js
	echo "var Relooper = (function() {" > relooper.js
	cat relooper.raw.js >> relooper.js
	echo "  return Module;" >> relooper.js
	echo "})();" >> relooper.js

#relooper.exe: combined.bc
#	$(NATIVIZER) combined.bc

clean:
	rm relooper.js bindings.js bindings.cpp combined.bc relooper.bc bindings.bc bindings.all.h

