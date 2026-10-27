# Ícones com aparência 3D

A barra usa miniaturas estáticas desenhadas com gradientes, luz e sombra: rede, volume, brilho, energia, gato do Vitória Shell, busca, bandeja e clima. Os estados de mudo e desconexão continuam indicados; o clima acompanha a condição atual. Cliques, rolagem e descrições preservam suas funções.

O gato da GPU conserva a animação e a velocidade comandada pela telemetria, agora com sombreamento no corpo. Ele continua respeitando a opção global de animações. Os demais ícones não possuem temporizadores de animação: o desenho só é atualizado quando o tamanho ou o estado muda.

Validação: 26 testes JavaScript aprovados. Os oito ícones estáticos foram renderizados em uma janela QML própria e exportados para conferência visual, sem erros do Canvas. A barra atualizada iniciou sem erros nos registros. Nenhum teste de desempenho ou consumo de GPU foi executado.
