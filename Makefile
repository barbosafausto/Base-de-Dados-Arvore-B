# ----- Variáveis de Configuração -----

# Nome do executável
EXECUTAVEL = programaTrab

# Define o compilador
CC = gcc

# Flags
CFLAGS = -fdiagnostics-color=always -g

# .h
HEADERS = fornecidas/fornecidas.h funcionalidades/funcionalidades.h registro/registro.h utils/utils.h

# Para o GCC: onde procurar pelos arquivos .h
INCLUDES = -I./fornecidas -I./funcionalidades -I./registro -I./utils

# .c
SRC = main.c fornecidas/fornecidas.c funcionalidades/funcionalidades.c registro/registro.c utils/utils.c


# ----- Regras de Compilação -----

# Evita conflitos com arquivos que tenham o mesmo nome das regras
.PHONY: all run clean verifica leaks zip

# O comando 'all' é o padrão executado pelo RunCodes ou ao digitar apenas 'make'
all: $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o $(EXECUTAVEL) -lm

# O comando 'run' executa o programa
run: all
	./$(EXECUTAVEL)

# O comando 'clean' remove o executável e o zip
clean:
	rm -f $(EXECUTAVEL) entrega.zip

leaks: all
	valgrind --leak-check=full ./$(EXECUTAVEL)


# ----- Submissão -----

# Nome do arquivo de submissão
ZIP = entrega.zip

# O comando 'zip' empacota os fontes e o Makefile
zip:
	@echo "Empacotando arquivos para submissão..."
	zip -r $(ZIP) . -i \*.c \*.h \*Makefile