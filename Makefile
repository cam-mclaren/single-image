#My complier 

COMPILER = gcc

# Linker Flags 

LINKER_FLAGS = -lm -lpthread -lmpfr -lgmp -lmicrohttpd

#Compilation flags 

COMPILER_FLAGS = 
TARGET_FILES =  my_utils.c log.c image_gen.c server.c main.c


OBJECT_FILES = $(TARGET_FILES:%.c=%.o)


main.out: $(OBJECT_FILES) 
	$(COMPILER) $(OBJECT_FILES) $(LINKER_FLAGS) -o main.out

compile:
	$(COMPILER) $(COMPILER_FLAGS) -c $(TARGET_FILES)

runmain: main.out
	JSON_ARGS=$$(cat centre_image_params.json); ./main.out \
						-x $$(printf "%50.50s " $$(echo $$JSON_ARGS | jq .x | sed -e 's/"//g' )) \
						-y $$(printf "%50.50s " $$(echo $$JSON_ARGS | jq .y | sed -e 's/"//g' ))\
						-w $$(printf "%50.50s " $$(echo $$JSON_ARGS | jq .width | sed -e 's/"//g' ))

runserver: main.out
	./main.out -s

testserver:
	JSON_ARGS=$$(cat centre_image_params.json); \
						URL='localhost:8888/image?x='$$(printf "%50.50s" $$(echo $$JSON_ARGS | jq .x | sed -e 's/"//g' ))'&y='$$(printf "%50.50s" $$(echo $$JSON_ARGS | jq .y | sed -e 's/"//g' ))'&width='$$(printf "%50.50s" $$(echo $$JSON_ARGS | jq .width | sed -e 's/"//g' )) ; \
	curl $$URL;

clean: 
	rm -rf *.o *.out


######################################## Will remove below later. 
######################################## Only need these file to create the centre_image_params.txt
#maketemp: 
#	$(COMPILER)  temp_math.c  $(LINKER_FLAGS) -o test.out
#
#runtemp: test.out
#	JSON_ARGS=$$(cat image_parameters.json); \
#						./test.out \
#						$$(printf "%50.50s " $$(echo $$JSON_ARGS | jq .x | sed -e 's/"//g')) \
#						$$(printf "%50.50s " $$(echo $$JSON_ARGS | jq .y | sed -e 's/"//g')) \
#						$$(printf "%50.50s " $$(echo $$JSON_ARGS | jq .width | sed -e 's/"//g')) 
#
######################################## Will remove above later. 
######################################## Only need these file to create the centre_image_params.txt
