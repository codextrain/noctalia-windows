# Atualizações locais de 27/10/2026

- Balões da barra limitados à tela do widget, incluindo monitores com coordenadas negativas; textos longos quebram a linha.
- Fontes do NiriWin em tools/niriwin, com a configuração usada: janela individual centralizada em 97% da largura disponível; pares continuam lado a lado.
- Minimizar mantém a janela na área e nos ícones. Janelas já minimizadas ao iniciar também são reconhecidas e continuam minimizadas. Janelas escondidas na bandeja permanecem excluídas.
- Sobre este PC e nomes das categorias publicados no commit anterior e instalados via compilação GitHub Actions 40577071130.

Validação executada: 23 testes JavaScript; 24 testes Rust, clippy sem avisos e build release. Teste nativo com duas janelas próprias confirmou minimizar/restaurar e descoberta de uma janela inicialmente minimizada. Tela QML de hardware carregou oito componentes reais, sem erro. Hover visual dos balões ainda não foi verificado por captura de tela.

A compilação automática atual gera Noctalia. Para compilar o NiriWin separadamente, use tools/niriwin/build.ps1 com Rust e um linker Windows disponíveis. Binários, estado de recuperação, configurações pessoais e logs não estão incluídos.
