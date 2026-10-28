# Categorias visíveis e ícones nítidos

Central de controle: lateral de 178 px com nome permanente das 11 categorias, botão atual com destaque tonal, marcador e ícone vetorial alinhado. Painel ampliado de 700 para 832 px para preservar o espaço do conteúdo. Dicas de nome da lateral removidas, pois o texto já é visível. Novos símbolos CategoryIcon acompanham a paleta do tema.

Barra: mantém somente ícones, preservando seus textos funcionais existentes (hora, clima etc.). Os ícones esculpidos usam Canvas interno de maior resolução com escala visual compensada; seus tamanhos e áreas de clique permanecem iguais. O gato mantém as animações e recebe o mesmo tratamento de resolução. O lobo conserva a arte e passa a carregar imagem com resolução proporcional à densidade da tela.

Validação: fixture Qt da central exibiu 11 categorias, capturou a prévia de 832 px e foi inspecionada visualmente. Fixture da barra capturou os ícones, gato e lobo sem nomes adicionais e foi inspecionada visualmente. Sem erros QML nos testes. As capturas são de janelas próprias das fixtures. Aplicado no runtime local; não houve compilação nativa nesta alteração QML.

Ajuste de contraste: as categorias mantêm fundo transparente e texto na cor normal do tema. Seleção e hover mudam apenas a borda, sem preenchimento colorido ou mudança da cor do texto. Prévia Qt renderizada e inspecionada, sem erros QML; aplicada no runtime local.
