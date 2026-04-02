# ----- Windows

# Nome do executável
EXECUTAVEL = programaTrab

# O comando 'all' compila todos os arquivos .c da pasta atual
all:
	gcc -o $(EXECUTAVEL) *.c -lm

# O comando 'run' executa o programa
run:
	./$(EXECUTAVEL)

# O comando 'clean' remove o executável e arquivos binários de teste para limpar a pasta
clean:
	rm -f $(EXECUTAVEL) 



# ----- Linux

# ----- O comando 'verifica' compila todos os arquivos .c via terminal

# Define o compilador
CC = gcc

# Flags
CFLAGS = -fdiagnostics-color=always -g

# Todos os arquivos .c 
SRC = main.c fornecidas.c funcionalidades.c registro.c utils.c

# Evita conflitos caso exista um arquivo chamado "verifica" na pasta
.PHONY: verifica

# A regra que será executada ao digitar "make verifica"
verifica:
	$(CC) $(CFLAGS) $(SRC) -o $(EXECUTAVEL)

leaks:
	valgrind --leak-checks=full ./EXECUTAVEL

