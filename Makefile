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
	rm -f $(EXECUTAVEL) *.bin