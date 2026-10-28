# Rolagem dos wallpapers online

OnlineWallpaperGrid usa um ListModel estável. Páginas que preservam a sequência de chaves acrescentam somente os novos itens, sem trocar o modelo do GridView. Itens existentes só são atualizados quando seus metadados mudam. Nova busca ou coleção incrementa uma revisão para reiniciar a posição. Paginação automática não é solicitada enquanto a grade está sincronizando ou oculta.

Validação executada: teste nativo QML com 48 → 72 → 96 itens, preservando duas posições de rolagem e a identidade do delegado visível. Nova busca, atualização da mesma busca e desativação também passaram. O fixture portátil está em tests/qml/wallpaper-scroll/Main.qml; seus resultados ficam em probe no arquivo de configuração temporário escolhido pelo executor.

A paginação real do Wallhaven não foi percorrida manualmente neste teste; o painel online atualizado foi aberto na instalação atual para verificação de carregamento e registros.

## Indicador e barra lateral

Três bolinhas pulsantes aparecem durante consultas online e carregamento das miniaturas. A animação pausa quando o indicador fica oculto e respeita a preferência global de animações. A grade reserva espaço para uma barra lateral arrastável, com clique na trilha e rolagem sobre ela; também disponível na grade de wallpapers locais. Chegar ao fim pela barra lateral pode solicitar a página seguinte.

Teste nativo ampliado: onze verificações passaram, incluindo posição no meio/fim pelo controlador da barra, indicador visível, variação da fase da pulsação e interrupção da animação ao ocultar. O arraste físico do mouse na barra não foi executado pelo teste automatizado.
