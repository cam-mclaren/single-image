#My complier 

COMPILER = gcc

# Linker Flags 

LINKER_FLAGS = -lm -lmpfr -lgmp 

#Compilation flags 

COMPILER_FLAGS = 
TARGET_FILES =  my_utils.c image_gen.c main.c


OBJECT_FILES = $(TARGET_FILES:%.c=%.o)


main.out: $(OBJECT_FILES) 
	$(COMPILER) $(OBJECT_FILES) $(LINKER_FLAGS) -o main.out

compile:
	$(COMPILER) $(COMPILER_FLAGS) -c $(TARGET_FILES)

runmain: main.out
	./main.out $$(cat image_parameters.txt | awk '{printf "%50.50s ", $$0}')

clean: 
	rm -rf *.o *.out


######################################## Will remove below later. 
######################################## Only need these file to create the centre_image_params.txt
maketemp: 
	$(COMPILER)  temp_math.c  $(LINKER_FLAGS) -o test.out

runtemp: test.out
	./test.out $$(cat image_parameters.txt | awk '{printf "%50.50s ", $$0}')

######################################## Will remove above later. 
######################################## Only need these file to create the centre_image_params.txt
