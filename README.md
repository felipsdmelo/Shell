# Shell
Implementação de uma shell básica em linguagem C com uma interface simples de interação com o usuário para a disciplina de Computadores e Programação da UFRJ.

# Comandos built-in
    - help: lista os comandos built-in implementados
    - cd: muda o diretório corrente
    - exit: fecha a shell
    - jobs: lista jobs correntes e seus status
    - bg: coloca um job em background
    - fg: coloca um job em foreground

# Compilação e utilização
Conceda permissão para execução ao arquivo Makefile com o comando:
```bash
chmod +x Makefile
```

Após isso, execute o Makefile para compilar o programa com o comando:
```bash
./Makefile
```

Com isso, será gerado um arquivo executável de nome `shell` e você poderá utilizar `./shell` para começar a utilizar.

# Outras funcionalidades
Essa shell é capaz de executar os mesmos comandos não built-in de um terminal Linux, tais como compilar e executar um programa, listar os arquivos de um diretório e copiar ou remover arquivos.
