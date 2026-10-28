# Desligamento automático

Central de controle > Sistema: horas e minutos, agendar/substituir, contagem regressiva e cancelar. Limite de 1 minuto a 24 horas. O painel tem rolagem para alcançar o novo cartão.

O singleton é iniciado pelo Main.qml, por isso fechar o painel não para a contagem. O Vitória precisa ficar em execução. A data de término é salva para reinícios do shell; se já passou quando o shell inicia, o agendamento é descartado para não desligar inesperadamente no login. Suspensão do PC não interrompe o relógio: ao retomar, um prazo já vencido será executado.

Ao vencer, o timer limpa o agendamento e emite exatamente um pedido. Main.qml encaminha à função existente Shell.session("shutdown"), que utiliza shutdown.exe /s /t 0, sem /f. Programas com documentos não salvos podem bloquear o desligamento. Não há desligamento forçado.

Validação executada: dois testes de política (limites, prazos, contagem e formatação) passaram. Fixture nativa Qt confirmou recusa de zero, agendamento de uma hora, contagem de 3600 segundos, pedido único ao vencer e cancelamento. A fixture conecta o sinal a um contador, nunca ao desligamento do Windows. O SystemTab foi instanciado sem erros QML. Nenhum desligamento real foi executado para testar o recurso, e nenhum timer foi ativado na configuração real do usuário.

Voz não implementada nesta etapa. A consulta ao mecanismo System.Speech não retornou reconhecedores instalados neste PC; isso não é uma auditoria de todos os mecanismos de voz possíveis. Uma futura implementação precisa de um reconhecedor português e consentimento para o microfone. Não há captura de áudio em segundo plano neste recurso.
