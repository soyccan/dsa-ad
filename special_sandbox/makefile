source = demo
all: $(source)
$(source):
%: %.c
	g++ -O2 -std=c11 $< -o $@
%: %.cpp
	g++ -O2 -std=c++14 $< -o $@
clean:
	rm -rf $(source)
run:
	./demo < input