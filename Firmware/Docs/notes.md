![alt text](images/board.png)

regras:

o objetivo do corehub é:

controlar a climatização de uma sala com base na presença de um ser humano dentro da sala

utilizando o smartdoor temos as seguintes variáveis para utilizar no controle:

para smartdoor:
 - sensores (uplink mqtt): porta, luz
    - porta: aberta, fechada
    - luz: acesa, apagada
 - atuadores (downlink mqtt): buzzer
    - buzzer: soando ou não soando   
 - interno (controle corehub): temporizador
    - temporizador: esgotado ou não esgotado
    - Regras: 
        - porta + aberta, luz + acesa, temporizador + esgotado = buzzer + soando
        - qualquer outra combinação: buzzer + não soando

para senseclima:
    - sensores (uplink mqtt): temperature (por zona)
         - temperature: valor em graus celcius inteiros
    - configuração (downlink mqtt) (controle corehub): intervalo entre leituras de temperatura
        - intervalo: de minutos a horas
    - Regras:
        - sensores enviam dados para ajudar o corehub a definir a temperatura da sala e decidir qual temperatura alvo user no condicionador de ar

para aircontrol:
    - sensores: nenhum
    - atuadores: temperature, power
        - temperature: valor inteiro absoluto entre 16 e 32 para setar condicionador de ar
        - power: valor em texo on ou off, representando ligar ou desligar
    Regras:
        - do smartdoor: porta + fechada/acesa, luz + acesa, power + on, temperature + 24 (se primeira vez)
        - do smartdoor: porta + fechada/acesa, luz + acesa, power + on, temperature + 16-32 (recupera temperatura da memória flashse houver)
        - do smartdoor: basicamente se a luze estiver acesa, power + on, temperature + 16-32 (ou 24 se for a primeira vez )

tabela verdade para ajudar (entre aircontrol e smartdoor)

considerando big endian (da esquerda pra direita)

bit 1: porta
bit 2: luz
bit 3: temporizador
bit 4: buzzer
bit 5: power
bit 6: temperature

linha 1: 000000
linha 2: 001000
linha 3: 010011
linha 4: 011011
linha 5: 100000
linha 6: 101011
linha 7: 110011
linha 8: 111111

sobre o senseclima

Se a temperatura lida for superior a um limite definido (ex: 28 °C), publicar "ON" para 
ligar o ar-condicionado. 
Se a temperatura cair abaixo de um limite mínimo (ex: 24 °C), publicar "OFF" para 
desligar ar-condicionado.

opcional: ler as configurações de intervalo de buzzer, limite superior e inferior, de uma tópico mqtt e salvar esses parâmetros na flash

opcional: ler a temperatura a ser setada no condicionador de um tópico específico


