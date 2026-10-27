# Áreas com até duas janelas (NiriWin)

Cada bolinha representa um grupo por monitor: uma janela fica centralizada; duas ficam lado a lado. A terceira inicia outra área, a quarta forma seu par. Ao fechar uma janela, a restante mantém a área e volta ao centro. Clique na bolinha ou use Alt+Esquerda/Direita para trocar de grupo. Isso depende da versão atualizada do NiriWin; estados de versões antigas continuam compatíveis com indicadores individuais. Os grupos são reconstruídos ao reiniciar o gerenciador.

# Personalizações da barra e integração com NiriWin

## Funcionalidades

- Configurações → Barra: arraste widgets entre Disponíveis, Esquerda, Centro e Direita, inclusive para reordenar. Remova devolvendo a Disponíveis.
- Listas antigas salvas como `null` recuperam os padrões. Listas explicitamente vazias continuam vazias. A gravação nativa é atômica; a interface mantém compatibilidade com executáveis anteriores usando `bar.widget_layout_json`.
- O relógio mostra hora, dia da semana, dia e mês: `14:24 seg., 16 mar.`. O formato da hora continua configurável.
- O gato da GPU substitui o botão de papel de parede no layout padrão; ambos ficam disponíveis no editor. Instalações existentes conservam seu layout salvo.
- O gato dorme abaixo de 8% e começa a correr a partir de 12% da carga suavizada. A corrida acelera com o uso. Dados ausentes ou antigos mostram `?`; clicar abre o painel de sistema. Respeita a opção de animações.
- As cores acompanham o papel de parede, inclusive alterações no conteúdo do mesmo arquivo. Há atualização manual, proteção contra resultados assíncronos antigos e preservação da escolha de tema fixo.

## Leitor da GPU

`tools/gpu-agent` contém um auxiliar Rust independente, sem serviço e sem administrador. Consulta os contadores PDH GPU Engine do Windows, soma contextos por mecanismo físico e mostra o mecanismo mais ocupado entre todas as GPUs, limitado a 100%.

Com Rust 1.87+ e linker para Windows disponíveis:

```powershell
cargo test --locked --manifest-path tools/gpu-agent/Cargo.toml
cargo build --release --locked --manifest-path tools/gpu-agent/Cargo.toml
```

Copie `tools/gpu-agent/target/release/noctalia-gpu.exe` para a pasta do `noctalia.exe`. O CMake faz isso automaticamente quando Cargo está disponível (`NOCTALIA_BUILD_GPU_HELPER=ON`). O GitHub Actions instala Rust, testa e inclui o auxiliar no artefato. O novo executável inicia o auxiliar junto com o Noctalia; ele termina quando o processo pai fecha. `NOCTALIA_GPU_STATE` permite fornecer um leitor externo e evita duplicação.

## Integração opcional com NiriWin

Para carregar estes QML com um executável já instalado, use `tools/Start-Noctalia.ps1 -Executable "C:\Noctalia\noctalia.exe"`. Acrescente `-Background` para iniciar sem abrir as configurações, ou `-NiriStatusPath "C:\NiriWin\state\status.json"` para fornecer o estado do gerenciador. O auxiliar da GPU deve estar junto do executável. Os logs deste lançador ficam em `%LOCALAPPDATA%\noctalia-win\dev-launcher`.

O NiriWin é um projeto separado: seu código e executável não fazem parte deste repositório. O Noctalia acha o `state/status.json` dele sozinho: pela variável `NOCTALIA_NIRIWIN_STATUS`, pela pasta indicada em *Configurações → Sistema → NiriWin* (`windows.niriwin_path`), pela pasta do processo do NiriWin em execução ou por pastas comuns (`C:\NiriWin`, `%LOCALAPPDATA%\NiriWin`...). A busca se repete a cada 5 segundos, então o NiriWin pode ser aberto depois. O widget usa o monitor e suas coordenadas para filtrar janelas e destaca a ativa. Sem o NiriWin (ou sem estado recente), o mesmo widget volta a mostrar as áreas de trabalho virtuais do Windows.

Configurações → Aparência oferece arredondamento de janelas de 0–100%, salvo em `windows.corner_rounding_percent`. **A aplicação nas janelas depende do NiriWin ativo e compatível**, que lê as preferências do Noctalia. O máximo corresponde a raio de 32 pixels no NiriWin atual.

Clique com o botão esquerdo em uma bolinha numerada para trazer a janela correspondente do NiriWin. A mesma ação de foco usada pelo dock restaura janelas minimizadas; o NiriWin acompanha a janela em foco e ajusta a posição. Os atalhos de teclado continuam disponíveis. No modo de áreas de trabalho virtuais do Windows, o clique continua ativando a área correspondente.

O blur permanece experimental e desligado por padrão. Aplicativos com fundo opaco podem não exibir efeito. O Noctalia está excluído da aplicação do blur e seu controle de intensidade está desativado porque o teste produziu margens brancas. A alteração nas janelas também depende do NiriWin; estes controles não implementam blur universal no Noctalia.

## Validação e limites

Os testes JavaScript passaram localmente em 27/10/2026:

```powershell
node --test tests/*.test.cjs
```

O auxiliar Rust foi compilado e executado no Windows durante o desenvolvimento, com amostras reais de GPU. Foram executados dois testes unitários do leitor e testes QML próprios de repouso/corrida. Não houve teste de estresse da GPU nem validação visual completa da barra por captura.

As alterações C++/Qt não foram recompiladas nesta máquina, por falta do SDK de desenvolvimento Qt. A execução local usou o Noctalia instalado com os QML modificados por `NOCTALIA_QML_DIR`. O resultado da compilação completa deve ser conferido no GitHub Actions; enviar os fontes não confirma que o build passou.

Preferências pessoais, caminhos do computador, backups, logs, executáveis locais e telemetria não são versionados.

## Camada dos widgets da area de trabalho
Os widgets de desktop mantem o indicativo de camada inferior, sem janela transiente, e reafirmam sua posicao inferior a cada 500ms enquanto visiveis. Isso evita que o relogio reapareca acima dos aplicativos apos recriacoes da janela Qt. Validacao local via ordem Z do Windows: relogio abaixo de Chrome/ChatGPT e acima do Program Manager; nenhuma captura visual foi executada.

## Bordas do tema e animacoes
Em Configuracoes > Aparencia, Bordas com a cor do tema liga/desliga a cor de destaque na janela ativa e outlineVariant nas demais. O Noctalia envia a paleta atual por um JSON escalar, atualizado apenas quando as cores mudam, para o NiriWin. Isso acompanha o wallpaper quando o tema esta em modo Papel de parede. Requer o NiriWin atualizado: o Noctalia sozinho nao modifica janelas de outros aplicativos. A API de borda e especifica do Windows 11 e o resultado visual depende do aplicativo. Ao desativar ou recuperar, a cor volta ao gerenciamento do sistema quando a API nao fornece leitura da cor anterior.
Movimento suave das janelas suaviza encaixe e redimensionamento pelo NiriWin ao abrir/fechar janelas. Nao captura nem atrasa o fechamento de aplicativos; o efeito de entrada/saida de cada janela continua com Windows/aplicativo. Respeita a opcao geral Animacoes.
Transicao do papel de parede usa uma camada temporaria de desktop, sem foco e transparente a cliques, para desvanecer a imagem anterior em 350ms, atras dos aplicativos. Tem opcao propria e respeita Animacoes. E suportada nos modos crop, fit e stretch; center/repeat/span ficam com a troca nativa. O servico atual acompanha o wallpaper do primeiro monitor, portanto papeis diferentes por monitor nao foram validados. Desligar libera a imagem guardada para a transicao.
Validacao: teste QML com sinal de troca controlado confirmou imagem carregada, opacidade intermediaria, fim da animacao, flag transparente a entrada, desativacao e publicacao da paleta. Nao foi feita captura visual nem troca do wallpaper pessoal durante esse teste. C++/Qt nao foi recompilado localmente; os novos QML foram carregados pelo executavel instalado.

## Espaco reservado apos reiniciar o Explorador
A barra refaz a reserva uma vez apos inicializar, liberando o registro e retomando-o 120ms depois. Nos fontes nativos, o evento TaskbarCreated invalida o registro antigo e agenda uma nova reserva quando o Explorador reinicia; essa parte C++ requer nova compilacao e ainda nao foi compilada localmente. O NiriWin atualizado detecta tambem barras com sombra transparente alem dos limites da tela. Para uma reparacao explicita de janelas maximizadas no monitor ativo, existe niriwin.exe --retile; ele nao e aplicado automaticamente.

## Destaque na mesma area
Com o NiriWin atualizado, Alt + clique esquerdo centraliza a janela e recolhe sua parceira, conservando as duas na mesma bolinha. Repetir na janela destacada restaura o par. Ao sair e voltar a area, o destaque permanece. A bolinha informa 1 de 2 janelas no tooltip e aponta para a janela escolhida ao voltar. Pausar/reiniciar o gerenciador limpa esse estado temporario.
O recurso pode ser desativado em Aparencia > Alt + clique esquerdo para destacar. O NiriWin intercepta apenas esse gesto em suas janelas gerenciadas; cliques normais, janelas nao gerenciadas, tela cheia e maximizadas ficam fora do gesto. Ctrl/Shift/Win adicionais deixam o clique livre. O Noctalia apenas mostra o estado e salva a preferencia; requer o NiriWin atualizado.

## Monitor sem janelas gerenciadas
O modo da barra agora depende de um estado recente do NiriWin, nao da existencia de janelas naquele monitor. Com NiriWin ativo, um monitor sem janelas mostra somente a area 1 vazia e numerada; nao troca para a lista global de areas virtuais do Windows. A area vazia nao tenta focar HWND 0. Sem NiriWin ativo/recente, as areas reais do Windows continuam disponiveis e tambem mostram numeros.
Validacao local: 16 testes JavaScript passaram. Um teste QML com os dois monitores reais (Hailstorm em 0,0 e HORIZON Z27 em -1920,188) confirmou modo Niri nos dois: uma area 1 com duas janelas no primeiro e uma area 1 vazia no segundo. Nao foi feita captura visual da barra neste teste.

## Condicao do tempo na barra
O widget de clima mostra temperatura e Weather.description lado a lado, atualizadas pelo servico existente. O icone, tooltip de localizacao e clique para abrir o painel de clima continuam disponiveis. Descricoes longas ficam limitadas a 180px de largura (ajustados pela escala), com texto completo no tooltip. O QML foi aplicado ao executavel instalado e reiniciado sem erros de carregamento registrados; nao foi feita captura visual nesta alteracao.

## Ocultacao completa da barra do Windows
Sistema > Esconder a barra de tarefas do Windows agora controla hide_taskbar_permanently, com um botao Mostrar a barra do Windows. Exige NiriWin atualizado: o Noctalia publica a preferencia; o NiriWin oculta as barras principal/secundaria e desliga auto-hide para impedir a revelacao pelo mouse. A antiga preferencia autohide_taskbar e desligada quando a nova opcao e ativada.
O NiriWin amplia as janelas gerenciadas ate a margem inferior configurada, sem alterar a area util global de aplicativos externos. Pausar/parar o gerenciador ou fechar o Noctalia restaura a barra. A recuperacao apos crash usa os comandos de recuperacao do NiriWin e o novo journal taskbar-recovery.json. Ocultacao, hover nas duas bordas, reexibicao e recuperacao apos interrupcao do gerenciador de teste foram verificados localmente.
A descricao do clima fica a direita da temperatura, na mesma linha, usando o texto atual do servico Weather.

## Icones dos aplicativos da area atual
O widget Aplicativos da area atual (running-apps) mostra somente icones, por janela do grupo NiriWin selecionado no monitor daquela barra. Clicar foca a janela; o tooltip mostra seu titulo. A janela ativa tem um pequeno indicador. No modo de destaque as duas janelas da area continuam acessiveis pelos icones; monitores vazios nao mostram aplicativos fixados ou de outros monitores.
Os icones reais e os handles vem do DockService e do provedor de icones existente, com uma atualizacao compartilhada por segundo quando o dock esta desligado. Nao exige ativar o dock flutuante. O novo widget fica a direita no layout padrao e pode ser movido/removido em Barra. O widget Midia permanece disponivel no editor, mas saiu do layout aplicado para retirar o titulo da reproducao.
Requer estado recente do NiriWin para determinar a area e o monitor. Sem esse estado, o widget fica oculto. Nao e uma lista global de todos os aplicativos do Windows.
Validacao: 20 testes JavaScript passaram, cobrindo filtro por area, monitor, destaque e estado antigo/pausado. Um teste QML carregou os icones reais do Chrome e ChatGPT pelo executavel instalado e confirmou Image.Ready nos dois. Nao foi feita captura visual da barra nesta etapa. Os novos padroes C++ ficaram nos fontes, sem recompilacao local do Qt.
