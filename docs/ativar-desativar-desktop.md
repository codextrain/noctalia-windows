# Ativar e desativar o desktop

Em **Central de controle > Sistema > Vitória Shell e NiriWin**, use **Desativar Vitória Shell e NiriWin**. O controle pede ao NiriWin que pare e restaure as janelas, efeitos e barra de tarefas; depois solicita a saída normal do Vitória. Nenhum processo é encerrado à força. Aplicativos abertos são preservados.

Para voltar, abra **Vitória Shell** no menu Iniciar. A mesma entrada alterna entre os dois estados, sem atalhos na área de trabalho. Tecnicamente o Windows indexa uma entrada `.lnk` na pasta Programas do usuário; ela aponta para o lançador com `--toggle`. O marcador `desktop-disabled.flag`, ao lado do lançador, preserva a escolha no próximo logon. Os atalhos e configurações pessoais permanecem instalados.

O lançador deve ser a entrada da inicialização automática; entradas independentes que abram diretamente outros executáveis não consultam esse marcador. O microfone e o monitor de GPU encerram junto com seu processo pai. Um desligamento automático agendado pelo Vitória fica sem execução enquanto o shell estiver fechado; revise o agendamento ao reativar.

## Instalação no layout de execução atual

A pasta de execução contém `Vitoria-Start.exe`, `NiriWin/niriwin.exe` e `Noctalia-DragDrop/runtime-path.txt`, com o caminho para o executável instalado do shell. Compile e instale os atalhos:

```powershell
rustc --edition 2021 -O tools/startup/desktop-start.rs -o C:/DesktopVitoria/Vitoria-Start.exe
powershell -NoProfile -File tools/startup/Install-DesktopControl.ps1 -RuntimeRoot C:/DesktopVitoria
```

Atualize os QML em uso, incluindo `Commons/DesktopControl.qml` e `Commons/qmldir`. Inicie pelo lançador para receber a variável `NOCTALIA_DESKTOP_CONTROL` que habilita o botão. Instalações sem o controle mostram a instrução de instalação e mantêm a ação desabilitada.

Opções do lançador: sem argumentos respeita a escolha salva; `--enable` ativa; `--disable` desativa; `--toggle` alterna. Chamadas simultâneas são serializadas por um mutex. Falhas geram mensagem e `inicio-nativo.log`; a escolha só muda após a operação ser concluída.

## Validação

Três testes Rust cobrem a decisão de logon, ações explícitas, alternância e argumentos inválidos. Os testes de integração devem confirmar parada normal, recuperação da barra de tarefas, marcador persistente e reativação dos processos. Não simulam um novo logon real.

Validação local executada: desativação com ambos os processos encerrados, barra principal do Windows visível e marcador gravado; execução sem argumentos com marcador não iniciou os processos; reativação abriu uma instância de cada um e removeu o marcador. Não foi realizado um novo logon real.
