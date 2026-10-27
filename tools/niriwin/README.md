# NiriWin 0.1 — MVP para Windows 11

Gerenciador de janelas em Rust inspirado nas colunas horizontais do Niri. Implementação própria sobre Win32; não é um port do compositor Wayland. O Explorer, a barra de tarefas e as decorações das aplicações continuam funcionando.

## Experimentar

Extraia **todo** o pacote portátil para uma pasta gravável. O executável não precisa de Rust instalado. Não execute dentro do ZIP ou em `Program Files`.

1. Para inspecionar sem mover janelas, execute `niriwin.exe --dry-run` no terminal.
2. Para ativar, abra `Start-NiriWin.cmd` ou execute `niriwin.exe --run`.
3. O painel mostra ATIVO/PARADO/PAUSADO e oferece Iniciar, Pausar, Retomar e Parar. Fechar o painel deixa o gerenciador ativo em segundo plano. Para desligar e restaurar as janelas, use o botão Parar, Ctrl+Alt+Shift+Q ou `Stop-NiriWin.cmd`. Abra `Start-NiriWin.cmd` para mostrar o painel novamente. Não há instalação automática na inicialização.
4. Depois de um travamento ou encerramento forçado, abra `Recover-NiriWin.cmd`. Ele solicita encerramento, aguarda dois segundos e tenta recuperar. Se o processo ainda estiver ativo, a recuperação é bloqueada: finalize-o e repita.

Abrir somente `niriwin.exe` mostra a ajuda; não reorganiza o desktop. Use `Start-NiriWin.cmd` para abrir o painel. Ele recupera um journal pendente antes de iniciar e grava mensagens em `state/manager.log` e `state/manager-error.log`. Não é necessário executar como administrador. O painel usa Windows PowerShell e Windows Forms, incluídos no Windows; mantenha `Painel-NiriWin.ps1` junto do executável.

## Atalhos

Navegação: **Alt+←/→**. Os demais comandos usam **Ctrl+Alt+Shift** mais a tecla indicada. Enquanto o gerenciador estiver ativo, Alt+setas fica reservado para as colunas e substitui voltar/avançar do navegador e do Explorador.

| Tecla | Ação |
| --- | --- |
| Alt+← / Alt+→ | Coluna anterior / seguinte no monitor da janela em primeiro plano |
| P | Pausar e restaurar; pressionar novamente retoma |
| Q | Restaurar e encerrar |
| R | Recarregar configuração; um erro mantém a configuração anterior |

Atalhos ocupados fazem o programa abortar antes de mover qualquer janela. O registro usa `RegisterHotKey`, sem interceptar a digitação. O modificador pode ser alterado para Ctrl+Alt, mas esse conjunto já apresentou conflito nesta máquina.

## Configuração

`niriwin.conf` fica ao lado do executável. Também é possível usar `--config CAMINHO`.

| Campo | Intervalo / significado |
| --- | --- |
| `gap` | 0–100 pixels físicos; margem externa e entre colunas |
| `width_percent` | 20–100; largura de cada coluna como percentual da área útil |
| `poll_ms` | 100–5000; intervalo de detecção |
| `reserve_top` | 0–500 pixels adicionais para uma futura barra |
| `hotkey_modifiers` | 7 = Ctrl+Alt+Shift; 3 = Ctrl+Alt; exige reiniciar |
| `navigation_modifiers` | Só para as setas: 1 = Alt (padrão), 3 = Ctrl+Alt, 5 = Alt+Shift, 7 = Ctrl+Alt+Shift; exige reiniciar |
| `align_to_bar` | `true` acompanha a barra detectada; `false` usa toda a área útil |
| `bar_class` | Classe Win32 da barra; nesta máquina, `Qt683QWindowToolSaveBits` |
| `round_corners` | `true` solicita cantos discretamente arredondados pelo Windows 11 |
| `exclude_classes` | Classes exatas separadas por ponto e vírgula |

A área útil respeita a barra de tarefas/AppBars. A largura tem mínimo de 160 pixels. Uma área pequena demais não é reorganizada. Exclusões do shell são permanentes. A classe configurada em `bar_class` também fica fora do gerenciamento. A detecção aceita somente uma barra larga e baixa junto ao topo do respectivo monitor, evitando confundir os outros widgets do shell.

## Funcionamento e recuperação

- Uma faixa ordenada de colunas por monitor, com uma janela por coluna. Novas janelas entram no final; a ordem permanece durante a sessão.
- Com barra detectada, o viewport acompanha as bordas laterais da barra e começa abaixo dela, com o gap configurado. Se apenas uma coluna cabe no viewport, ela ocupa a largura da barra; as outras continuam acessíveis por Alt+setas. Quando várias colunas cabem, a faixa começa na margem esquerda da barra. Sem barra detectada, uma janela única fica centralizada com a largura configurada, como antes.
- Os cantos usam `DWMWCP_ROUNDSMALL`. A preferência anterior é registrada no journal e restaurada na pausa/saída/recuperação. É uma solicitação nativa ao DWM: aplicações com regiões próprias ou transparência personalizada podem não apresentar arredondamento. O controle por porcentagem usa recorte Win32, conforme descrito em ARREDONDAMENTO.md.
- A janela em primeiro plano determina o foco. Navegar desloca a faixa visível. As colunas que não cabem são minimizadas, em vez de serem colocadas sobre outro monitor. Ao navegar de volta, são restauradas sem ativação até a solicitação de foco.
- Detecta monitores e janelas periodicamente. Uma janela movida manualmente para outro monitor passa para esse monitor. Remoção de monitor é tratada de forma básica; precisa de validação física adicional.
- Janelas inicialmente minimizadas, maximizadas, ferramentas, diálogos com proprietário e janelas ocultas/cloaked ficam fora do gerenciamento.
- Uma janela ocupando os limites físicos do monitor é tratada como tela cheia. O monitor fica congelado enquanto ela estiver visível. Uma janela maximizada em primeiro plano também suspende o rearranjo nesse monitor. A restauração não modifica janelas que ficaram maximizadas ou em tela cheia durante a sessão.
- Antes de alterar posições/minimização, grava `state/recovery.tsv`. O identificador da janela, PID e classe são conferidos antes da restauração. Uma segunda instância é bloqueada por mutex, inclusive a recuperação.
- Pausa, saída normal e Ctrl+C tentam restaurar as posições originais. Encerramento forçado exige `--recover`; o programa não instala serviço/watchdog.
- O arquivo de recuperação deve permanecer junto do executável. Não o apague após uma falha. A restauração usa solicitações Win32 assíncronas: uma aplicação travada pode não processá-las mesmo que a chamada seja aceita.

## Integração com Noctalia e próximos passos

O NiriWin não implementa o shell. Ele detecta as janelas das barras do Noctalia já instalado e acompanha suas dimensões por monitor; isso foi validado nesta máquina. Não há integração com protocolo interno do shell. O MVP oferece uma interface local inicial para um adaptador futuro:

- Executar `niriwin.exe --pause`, `--resume`, `--stop`, `--focus-left` ou `--focus-right`.
- Ler `state/status.json` com `schema_version`, estado de pausa, número de janelas gerenciadas/visíveis e monitores; após a saída publica `running:false`.
- O estado ativo inclui `running:true`, `updated_at_ms` e `windows` com identificador de janela, monitor Win32, origem do monitor, número sequencial por monitor e destaque `active`. O widget Janelas da versão local do Noctalia consome esses dados para suas bolinhas numeradas. Não são publicados títulos das janelas.
- Reservar área superior com `reserve_top` e excluir a classe real da janela do shell.

Esses comandos gravam uma solicitação em arquivo e retornam antes de ela ser processada. O arquivo comporta uma solicitação por vez: comandos simultâneos podem se sobrescrever. Não é uma API IPC definitiva. Próxima etapa: named pipe por usuário com ACL, respostas, identificadores estáveis de monitor, eventos e negociação de versão.

## Compilar e testar

Windows x64, Rust estável e linker compatível. Usa serde_json para ler as preferências de aparência do Noctalia. A primeira compilação baixa as dependências Cargo. O `Cargo.lock` está incluído. Com Rust MSVC, instale também o workload C++ e Windows SDK; com Rust GNU, disponibilize MinGW-w64 no PATH.

```powershell
# PowerShell · terminal · NiriWin · Windows 11 · Rust no PATH
.\build.ps1
cargo clippy --all-targets -- -D warnings
cargo build --release --example smoke
.\target\release\examples\smoke.exe "$PWD\niriwin.exe"
```

O último comando abre três janelas temporárias em cada monitor, move/minimiza somente a classe `NiriWinFixture` e testa restauração. Executar esse teste altera o foco temporariamente. Para `cargo run`, passe `--config` apontando para a configuração do projeto, pois a busca padrão ocorre ao lado do executável em `target`.

## Limitações e próximos passos

- Scrolling discreto por minimização; não há animações, recorte de superfícies ou faixa infinita parcialmente visível como em um compositor.
- Há um painel básico em Windows Forms; ainda não há bandeja, instalador, início automático, workspaces virtuais, atalhos para mover colunas entre monitores, drag-and-drop de colunas ou larguras individuais.
- Mínimos de tamanho impostos pela aplicação e bordas invisíveis do DWM podem alterar o gap visual. Aplicações elevadas, UWP, jogos exclusivos, HDR e diferentes escalas DPI não foram validados.
- Não há integração com desktops virtuais do Windows. Cloaking é filtrado, mas restauração entre desktops precisa de implementação própria.
- Detecção de tela cheia é geométrica, não uma garantia de reconhecer todos os modos de jogo. Restauração de janela em tela cheia/maximizada é intencionalmente pulada para preservar esse estado.
- Identidade de recuperação usa HWND/PID/classe, sem horário de criação do processo; reutilização desses identificadores após muito tempo é uma limitação. Use a recuperação na mesma sessão e não transporte o journal para outro computador.
- Estado e journal ficam na pasta do programa, sem isolamento entre usuários. Use uma pasta privada do usuário. Não há durabilidade garantida contra falta de energia.

Prioridades seguintes: journal com identidade forte e confirmação de restauração; políticas por aplicativo; WinEvent hooks com polling de segurança; bandeja com botão de emergência; DPI misto/hotplug/desktops virtuais; IPC versionado e adaptador Noctalia; instalador assinado.

## Referências técnicas

- [Niri](https://github.com/niri-wm/niri) — referência conceitual; nenhum código incorporado.
- [RegisterHotKey](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerhotkey) — registro global e conflitos de atalhos.
- [SetWindowPos](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos) — reposicionamento com preservação de ativação e ordem Z.
- [Cantos arredondados no Windows 11](https://learn.microsoft.com/en-us/windows/apps/desktop/modernize/ui/apply-rounded-corners) — preferência nativa e limitações por aplicação.

Veja `VALIDATION.md` para os testes realmente executados.

## Blur / acrílico

O Noctalia pode solicitar acrílico nas janelas compatíveis. Preferências: windows.blur_enabled e windows.blur_intensity_percent. A intensidade ajusta os fundos do Noctalia; a API nativa dos outros aplicativos usa o material do Windows. Originais ficam em state/blur-recovery.json e são restaurados na pausa/saída/recuperação. Consulte BLUR.md.

Janelas auxiliares de transferencia do Explorador (OperationStatusWindow) sao excluidas via exclude_classes: nao viram colunas nem indicadores. Essa exclusao nao cancela operacoes de arquivo.

# Comportamento atual: áreas com até duas janelas

Cada bolinha do Noctalia representa uma área por monitor. A primeira janela fica centralizada com `width_percent` da largura útil; a segunda divide a área em duas colunas iguais. A terceira abre outra área, centralizada; a quarta ocupa a segunda coluna dessa área. Fechar uma janela centraliza a restante sem transferir janelas de outras áreas. Alt+Esquerda/Direita troca de área; clicar numa bolinha traz o grupo correspondente.

Janelas de outras áreas ficam minimizadas pelo NiriWin e reaparecem ao voltar. Janelas em tela cheia/maximizadas continuam protegidas. Os grupos são reconstruídos na ordem de detecção ao reiniciar o gerenciador; não são áreas de trabalho virtuais nativas do Windows. Aplicativos que impõem larguras mínimas podem limitar o encaixe em colunas.


## Bordas e movimentos suaves
O Noctalia publica a cor de destaque e outlineVariant da paleta atual em windows.border_palette_json. O NiriWin aplica a borda nativa do Windows 11: mais viva na janela ativa e discreta nas demais, sem alterar espessura. windows.border_enabled desliga o efeito. Em versoes do Windows que nao permitem ler a cor anterior, desligar ou recuperar restaura a cor gerenciada pelo sistema (DWMWA_COLOR_DEFAULT), nao uma cor personalizada anterior de outro gerenciador.
windows.layout_animation_enabled suaviza o encaixe em cerca de 140ms e respeita shell.animation_enabled do Noctalia. Nao intercepta o fechamento de aplicativos nem implementa fade universal nas janelas; esses efeitos continuam a cargo do Windows e de cada aplicativo.
O journal agora inclui a cor de recuperacao em uma 12a coluna; o executavel novo le os formatos anteriores. Use o novo executavel tambem para recuperar journals novos.

## Destaque com Alt + clique esquerdo
Segure apenas Alt e clique com o botao esquerdo dentro de uma janela gerenciada. Ela fica centralizada e a parceira e minimizada temporariamente na mesma area. Repita na janela destacada para restaurar o par. O destaque e mantido ao visitar outras areas; reiniciar, pausar/recriar os grupos ou desativar o recurso limpa o modo. Se a janela escolhida fechar, a entrada de destaque e removida.
Configurar em Noctalia > Aparencia > Alt + clique esquerdo para destacar (windows.alt_click_spotlight_enabled). O gesto e consumido apenas nas janelas elegiveis visiveis do NiriWin; outros cliques e janelas nao gerenciadas continuam normais. Janelas maximizadas ou de tela cheia nao sao alvos. Ctrl/Shift/Win combinados com Alt ficam livres.
O hook de mouse usa uma thread dedicada, sem guardar coordenadas, conteudo de janela ou historico. Pausar/desativar esvazia os alvos; sair remove o hook. A recuperacao de posicoes continua usando o mesmo journal.

## Ocultacao completa da barra do Windows
Noctalia > Configuracoes > Sistema > Esconder a barra de tarefas do Windows ativa windows.hide_taskbar_permanently. O NiriWin desliga a auto-ocultacao e esconde as barras principal e secundaria; o mouse na borda nao as revela. Desative a opcao ou use Mostrar a barra do Windows para devolver a barra.
O espaco liberado e usado pelas janelas gerenciadas, mantendo gap=12 tambem embaixo (ou o gap configurado). Nao altera globalmente a area util de outros aplicativos; janelas maximizadas e em tela cheia continuam protegidas.
Pausar/parar o NiriWin ou sair do Noctalia restaura a barra. O estado original e salvo antes da alteracao em state/taskbar-recovery.json. Se houver interrupcao inesperada, use Mostrar-Barra-Windows.cmd ou Recover-NiriWin.cmd; ambos param o gerenciador e recuperam o estado. O arquivo de recuperacao e mantido se o Explorer nao confirmar a reexibicao. A inicializacao automatica e o painel reconhecem esse novo journal.
