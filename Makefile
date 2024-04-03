#My complier 

COMPILER = gcc

# Linker Flags 

LINKER_FLAGS = -lm -lpthread -lmpfr -lgmp 

#Compilation flags 

COMPILER_FLAGS = 
TARGET_FILES =  my_utils.c image_gen.c main.c


OBJECT_FILES = $(TARGET_FILES:%.c=%.o)


main.out: $(OBJECT_FILES) 
	$(COMPILER) $(OBJECT_FILES) $(LINKER_FLAGS) -o main.out

compile:
	$(COMPILER) $(COMPILER_FLAGS) -c $(TARGET_FILES)

runmain: main.out
	JSON_ARGS=$$(cat centre_image_params.json); ./main.out \
						$$(echo $$JSON_ARGS | jq .x | sed -e 's/"//g' ) \
						$$(echo $$JSON_ARGS | jq .y | sed -e 's/"//g' ) \
						$$(echo $$JSON_ARGS | jq .width | sed -e 's/"//g' )

clean: 
	rm -rf *.o *.out


######################################## Will remove below later. 
######################################## Only need these file to create the centre_image_params.txt
maketemp: 
	$(COMPILER)  temp_math.c  $(LINKER_FLAGS) -o test.out

runtemp: test.out
	JSON_ARGS=$$(cat image_parameters.json); \
						./test.out \
						$$(echo $$JSON_ARGS | jq .x | sed -e 's/"//g') \
						$$(echo $$JSON_ARGS | jq .y | sed -e 's/"//g') \
						$$(echo $$JSON_ARGS | jq .width | sed -e 's/"//g') 

######################################## Will remove above later. 
######################################## Only need these file to create the centre_image_params.txt
