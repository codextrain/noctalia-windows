# Comandos por voz locais

Em Central de controle > Sistema > Desligamento automático, ative Comandos por voz (PC). Desligado por padrão. O microfone padrão do Windows só é aberto quando essa opção está ativa; ao desligá-la o helper fecha o stream. O modelo é carregado sob demanda. O serviço auxiliar permanece leve enquanto desativado. Não há armazenamento de gravações nem envio de áudio para a rede.

Comandos:
- PC, desligar em uma hora.
- PC, desligar em trinta minutos.
- PC, cancelar.
- PC, desativar comandos de voz.

PC é pronunciado pê cê. O modelo não contém a palavra literal pc, portanto a gramática usa pê cê e p c; ambas são normalizadas para o mesmo comando. Outras palavras de ativação não são aceitas. Só são aceitas frases completas de agendamento, cancelamento e desativação, sem comandos arbitrários. Não existe desligar agora por voz. O prazo mínimo é 1 minuto, máximo 24 horas. Durações faladas aceitam minutos de 1 a 60 ou horas de 1 a 24. Exige confiança média de pelo menos 80%; ruído e o modelo pequeno podem causar falhas de reconhecimento. Não há garantia de imunidade a comandos vindos de som ambiente.

Vosk 0.3.45 + sounddevice 0.5.3, modelo oficial vosk-model-small-pt-0.3 (Apache 2.0). Downloads de dependências/modelo ocorrem na instalação; reconhecimento posterior é local. Consulta de microfone via PortAudio; nenhuma API paga é usada.

Para instalar: tools/voice/Install-Voice.ps1 -InstallRoot <pasta Noctalia-DragDrop> -Python <python.exe 3.12>. O iniciador passa NOCTALIA_VOICE_STATE como URL file:/// de voice-state.json ao QML e executa vitoria-voice/vitoria-voice.exe com --parent <PID do shell> --state <voice-state.json> --model <voice-model/vosk-model-small-pt-0.3> --config <%LOCALAPPDATA%/noctalia-win/config.json>. O iniciador nativo tools/startup/desktop-start.rs já incorpora esse passo no layout local suportado. O helper encerra quando o processo pai termina; um mutex impede duplicação.

O arquivo de estado contém somente situação e comandos válidos estruturados, com ID e horário; nunca a transcrição de conversas. O QML só aceita comandos recentes, uma vez por ID e com a opção habilitada. A opção não cancela um timer já agendado; para isso diga PC, cancelar ou use Cancelar na tela.

Validação: três testes Python passaram para comandos PC, gramática e rejeição de frases/limites. Modelo e gramática carregados no reconhecedor; silêncio processado. Fixture Qt confirmou agendamento, cancelamento e desativação a partir de comandos estruturados, e que comandos posteriores à desativação são ignorados. A fixture usa sinal de desligamento simulado, sem desligar o PC. Helper empacotado e instalado; estado desativado verificado. Não foi testado reconhecimento de fala humana ou captura real do microfone nesta execução.

Confirmação e barra: agendamento manual e por voz exibem automaticamente Desliga em HH:MM:SS no início da seção direita de todas as barras. Clicar abre Sistema. Cancelamento exibe Timer cancelado por seis segundos. Som de duas notas é solicitado somente após uma ação por voz ter sido aceita pelo timer; o helper consome o reconhecimento confirmado uma vez por ID e ignora confirmações antigas. Agendamento/cancelamento pelos botões permanecem silenciosos. Desativar voz por comando também confirma com som e mensagem.

Validação adicional: quatro testes Python passaram, incluindo confirmação de voz válida, repetição, expiração e ação recusada. Fixture Qt confirmou que ações manuais não geram confirmação sonora e que voz aceita gera o reconhecimento de retorno. Fixture visual mostrou Desliga em 01:00:00 e foi inspecionada. A função sonora foi executada uma vez no PC como teste, sem captura de áudio ou timer real. Helper atualizado empacotado e instalado; shell e helper iniciaram sem erros QML. Nenhum desligamento real foi executado.

Ajuste: cancelamento por voz simplificado para PC, cancelar (pronunciado pê cê cancelar). Gramática, parser, exemplos da interface e testes atualizados. Quatro testes Python passaram e o helper foi reempacotado e instalado.
