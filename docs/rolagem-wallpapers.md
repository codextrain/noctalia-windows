# Rolagem dos wallpapers online

OnlineWallpaperGrid usa um ListModel estável. Páginas que preservam a sequência de chaves acrescentam somente os novos itens, sem trocar o modelo do GridView. Itens existentes só são atualizados quando seus metadados mudam. Nova busca ou coleção incrementa uma revisão para reiniciar a posição. Paginação automática não é solicitada enquanto a grade está sincronizando ou oculta.

Validação executada: teste nativo QML com 48 → 72 → 96 itens, preservando duas posições de rolagem e a identidade do delegado visível. Nova busca, atualização da mesma busca e desativação também passaram. O fixture portátil está em tests/qml/wallpaper-scroll/Main.qml; seus resultados ficam em probe no arquivo de configuração temporário escolhido pelo executor.

A paginação real do Wallhaven não foi percorrida manualmente neste teste; o painel online atualizado foi aberto na instalação atual para verificação de carregamento e registros.

## Indicador e barra lateral

Três bolinhas pulsantes aparecem durante consultas online e carregamento das miniaturas. A animação pausa quando o indicador fica oculto e respeita a preferência global de animações. A grade reserva espaço para uma barra lateral arrastável, com clique na trilha e rolagem sobre ela; também disponível na grade de wallpapers locais. Chegar ao fim pela barra lateral pode solicitar a página seguinte.

Teste nativo ampliado: onze verificações passaram, incluindo posição no meio/fim pelo controlador da barra, indicador visível, variação da fase da pulsação e interrupção da animação ao ocultar. O teste original exercitava o controlador, sem eventos de mouse. A correção seguinte acrescentou o teste de arraste nativo descrito abaixo.

## Correção de captura do mouse

A MouseArea impede a grade de roubar o gesto. Durante o arraste, a interação por arraste do Flickable é suspensa e seu estado anterior é restaurado ao soltar, cancelar, ocultar ou destruir a barra. A paginação por fim de rolagem é adiada até soltar a barra.

Validação: reproduzido o comportamento antigo usando eventos Win32 SendInput em uma janela QML própria: não completava o arraste e não emitia liberação. Com a correção, dois arrastes (baixo/cima) passaram, alcançando mais de 60% da lista e retornando ao início, com duas liberações e interação restaurada. Ocultar durante o gesto restaura a grade; uma grade previamente não interativa conserva esse estado. A regressão de acréscimo de páginas/posição e o indicador de carregamento também foram exercitados novamente.

Fixture: tests/qml/scrollbar-drag/Main.qml, com send.ps1 enviando os eventos apenas se o alvo for o processo Noctalia do teste. Usar configuração e diretório temporário próprios, sem outra instância do shell durante o ensaio. Esses testes nativos foram executados localmente, não pela rotina JavaScript do CI.