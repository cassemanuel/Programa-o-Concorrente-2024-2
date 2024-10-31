package main

import (
    "fmt"
)

func main() {
    // Cria um canal de strings para comunicação
    str := make(chan string)

    // Inicia uma goroutine que irá responder às mensagens
    go func() {
        // Recebe a primeira mensagem de 'main' e imprime
        msg := <-str
        fmt.Println("Goroutine recebeu:", msg)

        // Envia a primeira resposta para 'main'
        str <- "Oi Main, bom dia, tudo bem?"

        // Recebe a segunda mensagem de 'main' e imprime
        msg = <-str
        fmt.Println("Goroutine recebeu:", msg)

        // Envia a segunda resposta para 'main'
        str <- "Certo, entendido."

        // Imprime finalizando
        fmt.Println("Goroutine: finalizando")
    }()

    // Envia a primeira mensagem para a goroutine
    str <- "Olá, Goroutine, bom dia!"

    // Recebe a resposta da goroutine e imprime
    msg := <-str
    fmt.Println("Main recebeu:", msg)

    // Envia a segunda mensagem para a goroutine
    str <- "Tudo bem! Vou terminar tá?"

    // Recebe a resposta final da goroutine e imprime
    msg = <-str
    fmt.Println("Main recebeu:", msg)

    // Imprime finalizando
    fmt.Println("Main: finalizando")
    
    fmt.Println("Goroutine: finalizando")
}

