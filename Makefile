#-------------------------------------------------------------------------------
# Arquivo      : Makefile
# Conteudo     : Implementação de thread
# Autor        : Vinícius Braga Freire (vinicius.braga@dcc.ufmg.br)
#				 Júnio Veras de Jesus Lima (junio.veras@dcc.ufmg.br)
# Historico    : 2023-04-03 - arquivo criado
#-------------------------------------------------------------------------------
# Opções	: make all - compila tudo
#			: make clean - remove objetos e executável
#-------------------------------------------------------------------------------
#-pg for gprof
CPP := gcc -g
TARGET := tp01

# Diretórios
BIN := ./
INC := ./
OBJ := ./
SRC := ./

LIST_SRC_C := $(wildcard $(SRC)*.c)
LIST_OBJ := $(patsubst $(SRC)%.c, $(OBJ)%.o, $(LIST_SRC_C)) $(TEST).o
LIST_TEST_OBJ := $(wildcard ./tests/*.o)
LIST_ERR_OUT := $(wildcard ./*.out ./*.err)

$(OBJ)%.o: $(SRC)%.c
	$(CPP) -c $< -o $@ -I $(INC)
	
all: $(LIST_OBJ)
	@echo $(LIST_OBJ)
	$(CPP) -o $(TARGET) $(LIST_OBJ) 

clean:
	rm $(TARGET) $(LIST_OBJ) ./gcc.log $(LIST_TEST_OBJ) $(LIST_ERR_OUT)

proof:
	gprof $(BIN)$(TARGET) ./bin/gmon.out > ./tmp/analise.txt

rod:	
	rm ./rodadas/*.txt