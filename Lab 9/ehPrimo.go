package main

import (
    "fmt"
)

// Função para verificar se um número é primo
func ehPrimo(n int) bool {
    if n <= 1 {
        return false
    }
    if n == 2 {
        return true
    }
    if n%2 == 0 {
        return false
    }
    for i := 3; i*i <= n; i += 2 {
        if n%i == 0 {
            return false
        }
    }
    return true
}

// Função que cada goroutine vai executar para verificar primos
func verificarPrimos(numChan <-chan int, resultChan chan<- bool) {
    for num := range numChan {
        resultChan <- ehPrimo(num)
    }
}

func main() {
    // Definindo a sequência de 1 a N
    N := 100  // ou o valor que desejar testar
    M := 4    // número de goroutines

    // Canais para comunicação
    numChan := make(chan int)
    resultChan := make(chan bool)

    // Inicia as goroutines
    for i := 0; i < M; i++ {
        go verificarPrimos(numChan, resultChan)
    }

    // Envia os números para as goroutines
    go func() {
        for i := 1; i <= N; i++ {
            numChan <- i
        }
        close(numChan) // Fecha o canal após enviar todos os números
    }()

    // Recebe e conta os primos
    count := 0
    for i := 1; i <= N; i++ {
        if <-resultChan {
            count++
        }
    }

    // Exibe o total de números primos
    fmt.Println("Total de números primos:", count)
}

