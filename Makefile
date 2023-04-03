#-------------------------------------------------------------------------------
# Arquivo      : Makefile
# Conteudo     : Implementação de thread
# Autor        : Vinícius Braga Freire (vinicius.braga@dcc.ufmg.br)
#				 Júnio Veras de Jesus Lima (// TODO: email do junio)
# Historico    : 2023-04-03 - arquivo criado
#-------------------------------------------------------------------------------
# Opções	: make all - compila tudo
#			: make clean - remove objetos e executável
#-------------------------------------------------------------------------------
#-pg for gprof
CPP := gcc -g
TARGET := tp01

# Diretórios
BIN := ./bin/
INC := ./include/
OBJ := ./obj/
SRC := ./

LIST_SRC_C := $(wildcard $(SRC)*.c)
LIST_OBJ := $(patsubst $(SRC)%.c, $(OBJ)%.o, $(LIST_SRC_C)

$(OBJ)%.o: $(SRC)%.c
	$(CPP) -c $< -o $@ -I $(INC)
	
all: $(LIST_OBJ)
	$(CPP) -o $(TARGET) $(LIST_OBJ)

clean:
	rm $(TARGET) $(LIST_OBJ) 

proof:
	gprof $(BIN)$(TARGET) ./bin/gmon.out > ./tmp/analise.txt

rod:	
	rm ./rodadas/*.txt