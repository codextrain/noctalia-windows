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

O NiriWin é um projeto separado: seu código e executável não fazem parte deste repositório. Para mostrar uma bolinha numerada por janela, configure `NOCTALIA_NIRIWIN_STATUS` com a URL local do `state/status.json` publicado pelo NiriWin antes de abrir o Noctalia. O widget usa o monitor e suas coordenadas para filtrar janelas e destaca a ativa; sem estado recente, não mostra indicadores fictícios.

Configurações → Aparência oferece arredondamento de janelas de 0–100%, salvo em `windows.corner_rounding_percent`. **A aplicação nas janelas depende do NiriWin ativo e compatível**, que lê as preferências do Noctalia. O máximo corresponde a raio de 32 pixels no NiriWin atual.

O blur permanece experimental e desligado por padrão. Aplicativos com fundo opaco podem não exibir efeito. O Noctalia está excluído da aplicação do blur e seu controle de intensidade está desativado porque o teste produziu margens brancas. A alteração nas janelas também depende do NiriWin; estes controles não implementam blur universal no Noctalia.

## Validação e limites

Os 16 testes JavaScript passaram localmente em 27/10/2026:

```powershell
node --test tests/*.test.cjs
```

O auxiliar Rust foi compilado e executado no Windows durante o desenvolvimento, com amostras reais de GPU. Foram executados dois testes unitários do leitor e testes QML próprios de repouso/corrida. Não houve teste de estresse da GPU nem validação visual completa da barra por captura.

As alterações C++/Qt não foram recompiladas nesta máquina, por falta do SDK de desenvolvimento Qt. A execução local usou o Noctalia instalado com os QML modificados por `NOCTALIA_QML_DIR`. O resultado da compilação completa deve ser conferido no GitHub Actions; enviar os fontes não confirma que o build passou.

Preferências pessoais, caminhos do computador, backups, logs, executáveis locais e telemetria não são versionados.
