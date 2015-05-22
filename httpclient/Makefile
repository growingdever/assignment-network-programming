PATH_BUILD = ./build
OBJS = $(PATH_BUILD)/main.o

main: main.o
	@- mkdir -p $(PATH_BUILD);
	@- echo "build::"
	gcc -lm -o $(PATH_BUILD)/$@ $(OBJS)
	@- echo ""

%.o: %.c
	@- mkdir -p $(PATH_BUILD);
	gcc -g -c -o $(PATH_BUILD)/$@ $<

clean:
	@- echo "cleaning::"
	rm -rf $(PATH_BUILD)/*
	@- echo ""

run:
	@- echo "result of run::"
	@- $(PATH_BUILD)/main $(ip) $(port)
	@- echo ""