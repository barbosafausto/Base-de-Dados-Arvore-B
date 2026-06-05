# Define o breakpoint na função que está dando dor de cabeça
b createIndex

# Inicia a execução injetando o arquivo de teste
run < teste.in

# (Opcional) Já dá o primeiro 'next' e mostra as variáveis locais
n
info locals