const express = require('express');
const http = require('http');
const path = require('path');
const socketIo = require('socket.io');

const app = express();
const server = http.createServer(app);
const io = socketIo(server);

const salaSenha = "config123"; // senha para configuração da sala
let maxJogadores = 1; // número máximo de jogadores na rodada
let jogoIniciado = false; // status do jogo
let configurador = null; // armazena o id do jogador configurador
let jogadoresProntos = new Set(); // jogadores que clicaram em "pronto"
let jogo = { jogadores: {}, vencedor: null, palavra: null }; // estado do jogo

// precisa pros arquivos estáticos (as páginas auxiliares)
app.use(express.static(path.join(__dirname)));

// define quem é a home/principal
app.get('/', (req, res) => {
	res.sendFile(path.join(__dirname, 'forca.html'));
});

// mensagens no fim do jogo
function definirMensagemFinal(jogador, jogadoresRestantes) {
	if (jogador?.vidas <= 0 && jogadoresRestantes.length === 0) {
		return "Ninguém venceu! Foi uma honra ter a(s) sua(s) presença(s). Para jogar novamente reinicie o servidor.";
	}
	if (jogo.vencedor) {
		return `Parabéns ${jogo.vencedor}, você venceu! Foi uma honra ter a(s) sua(s) presença(s). Para jogar novamente reinicie o servidor.`;
	}
	return "";
}

// emite mensagem de fim de jogo para todos os jogadores
function emitirGameOver(mensagem) {
	io.emit("game over", {
		vencedor: jogo.vencedor || null,
		palavra: jogo.palavra,
		mensagem: mensagem,
	});
	io.emit("mensagem", mensagem); // Envia uma mensagem genérica para todos os jogadores
}


// reinicia quando cloca no novoJogo
function resetarJogo() {
	jogo = {
		jogadores: {},
		palavra: null,
		vencedor: null,
	};
	jogadoresProntos.clear();
	jogoIniciado = false;
	maxJogadores = 1;
	configurador = null;
}

// Quando um jogador conecta
io.on('connection', (socket) => {
	console.log(`Novo jogador conectado: ${socket.id}`);

	// sincronizar estado para novos jogadores
	socket.on("sincronizar estado", () => {
		if (jogoIniciado) {
			socket.emit("mensagem", "Jogo já em andamento. Aguarde o próximo jogo.");
			socket.emit("atualizar jogo", {
				estado: jogo.palavra.split("").map(() => "_").join(" "), // formato legível
				vidas: jogo.jogadores[socket.id]?.vidas || 6,
				tentativas: jogo.jogadores[socket.id]?.tentativas || []
			});
		} else {
			socket.emit("mensagem", "Aguardando início do jogo.");
		}
	});

	// Jogador tenta entrar
	socket.on('login', (data) => {
		const { username, senha } = data;
		console.log(`Login recebido: ${username} - ID: ${socket.id}`);

		// verifica se o jogo já começou
		if (jogoIniciado) {
			socket.emit('mensagem', 'Jogo já em andamento. Você será um espectador até a próxima rodada.');
			socket.emit('atualizar jogo', {
				estado: jogo.palavra.split("").map(() => "_").join(" "),
				vidas: 0, // Jogadores novos não têm vidas no jogo em andamento
				tentativas: []
			});
			return;
		}

		// verifica se o jogador é o configurador
		if (senha === salaSenha) {
			if (!configurador) {
				configurador = socket.id; // define o jogador como configurador
				socket.emit('mensagem', 'Você é o configurador da sala.');
				socket.emit('configurador liberado'); // libera interface de configuração

				// adiciona funcionalidade de encerrar o servidor
				socket.on('encerrar servidor', () => {
					if (socket.id === configurador) {
						console.log('Servidor encerrado pelo configurador.');
						io.emit('mensagem', 'O servidor foi encerrado pelo configurador.');
						process.exit(0); // encerra o servidor
					} else {
						socket.emit('mensagem', 'Apenas o configurador pode encerrar o servidor.');
					}
				});
			} else {
				socket.emit('mensagem', 'Já existe um configurador da sala.');
				return;
			}
		} else if (senha !== "" && senha !== salaSenha) {
			socket.emit('mensagem', 'Senha incorreta.');
			return;
		} else {
			// jogador regular
			socket.emit('jogador regular');
		}

		// registra o jogador se ainda não estiver registrado
		if (!jogo.jogadores[socket.id]) {
			jogo.jogadores[socket.id] = {
				nome: username,
				tentativas: [],
				vidas: 6, // dificuldade padrão
			};
		}

		// loga o número de jogadores conectados no servidor
		console.log(`Jogadores conectados: ${Object.keys(jogo.jogadores).length}`);

		// envia uma mensagem de status ao jogador
		socket.emit('mensagem', 'Aguardando configuração da sala ou início do jogo.');

		// atualiza todos os clientes com o número de jogadores conectados
		io.emit('update players', Object.keys(jogo.jogadores).length);
	});

	// configurar número máximo de jogadores
	socket.on("set max jogadores", (max) => {
		if (socket.id !== configurador) {
			socket.emit("mensagem", "Apenas o configurador pode definir o número de jogadores.");
			return;
		}

		if (jogoIniciado) {
			socket.emit("mensagem", "Não é possível alterar o número de jogadores durante uma partida.");
			return;
		}

		// garante que o número mínimo de jogadores seja 1
		if (max < 1) {
			socket.emit("mensagem", "O número mínimo de jogadores deve ser 1.");
			return;
		}

		maxJogadores = max;
		io.emit("mensagem", `Número de jogadores definido para ${maxJogadores}.`);
		console.log(`Número de jogadores configurado para: ${maxJogadores}`);
	});

	// jogador clica em "pronto"
	socket.on("player ready", () => {
		if (jogoIniciado) {
			socket.emit("mensagem", "Jogo já iniciado. Aguarde a próxima rodada.");
			return;
		}

		jogadoresProntos.add(socket.id);

		// log no servidor
		console.log(`Jogadores prontos: ${jogadoresProntos.size}/${maxJogadores}`);

		// mensagem ao jogador
		socket.emit("mensagem", "Você está pronto! Aguardando os outros jogadores.");

		// verifica se todos os jogadores estão prontos e o número corresponde ao configurado
		if (
			jogadoresProntos.size === Object.keys(jogo.jogadores).length &&
			Object.keys(jogo.jogadores).length === maxJogadores
		) {
			iniciarJogo();
		}
	});

	// Processar tentativa de letra
	socket.on('player attempt', (letra) => {
		if (!jogoIniciado) {
			socket.emit('error', 'O jogo ainda não começou.');
			return;
		}

		// validar a entrada
		letra = letra.toUpperCase().trim();
		if (letra.length !== 1 || !/^[A-Z]$/.test(letra)) {
			socket.emit('error', 'Entrada inválida! Insira apenas uma letra.');
			return;
		}

		//removi a parte do "jogador não encontrado", sempre tem jogador

		const jogador = jogo.jogadores[socket.id];

		if (!jogador) {
			socket.emit('error', 'Jogador não encontrado ou desconectado.');
			return;
		}
		if (jogador.tentativas.includes(letra)) {
			socket.emit('error', 'Letra já tentada!');
			return;
		}


		//array armazenando as letras que o jogador ja tentou advinhar
		jogador.tentativas.push(letra);

		// verifica se a palavra foi configurada antes de aceitar tentativas
		// evita erros caso alguem tente jogar antes do jogo comecar
		if (!jogo.palavra) {
			socket.emit('error', 'A palavra ainda não foi configurada.');
			return;
		}
		//letra correta e letra incorreta
		if (jogo.palavra.includes(letra)) {
			socket.emit('correct', `Letra ${letra} está correta!`);
		} else {
			jogador.vidas--;
			socket.emit('incorrect', `Letra ${letra} está incorreta. Você tem ${jogador.vidas} vidas restantes.`);
		}

		//atualiza o estado da palavra para o jogador
		const palavraAtual = jogo.palavra.split('')
			.map((l) => (jogador.tentativas.includes(l) ? l : '_'))
			.join(' ');

		socket.emit('atualizar jogo', {
			estado: palavraAtual,
			vidas: jogador.vidas,
			tentativas: jogador.tentativas,
		});

		// teste de vitória ou derrota e qtd de vidas
		const jogadoresRestantes = Object.values(jogo.jogadores).filter(j => j.vidas > 0);
		const mensagem = definirMensagemFinal(jogador, jogadoresRestantes);

		if (palavraAtual.replace(/\s/g, '') === jogo.palavra) {
			// vitória: jogador adivinhou a palavra
			jogo.vencedor = jogador.nome;
			emitirGameOver(`Parabéns, ${jogador.nome}! Você venceu ao adivinhar a palavra.`);
			finalizarJogo(mensagem);
		} else if (jogador.vidas <= 0) {
			// derrota: jogador perdeu todas as vidas
			socket.emit('game over', { vencedor: null, palavra: jogo.palavra });
			delete jogo.jogadores[socket.id];

			// verificar estado geral dos jogadores
			if (jogadoresRestantes.length === 1) {
				// último jogador restante vence
				const ultimoJogador = jogadoresRestantes[0];
				jogo.vencedor = ultimoJogador.nome;
				emitirGameOver(`Último sobrevivente: ${ultimoJogador.nome}. Parabéns!`);
				finalizarJogo(mensagem);
			} else if (jogadoresRestantes.length === 0) {
				// todos os jogadores perderam
				emitirGameOver('Todos os jogadores perderam. Que azar!');
				finalizarJogo(mensagem);
			}
		}

	});

	socket.on("restart game", () => {
		console.log("reiniciando o jogo...");

		// reinicia o estado global
		resetarJogo();

		// libera o configurador novamente
		io.emit("configurador liberado");
		io.emit("mensagem", "configuração liberada para nova rodada.");
	});

	// ajeitar o jogo quando tiver desconexões
	function handleDisconnect(socket, io, jogo, jogadoresProntos, configurador, resetarJogo) {
		// verifica se o jogador existe antes de remover
		if (jogo.jogadores[socket.id]) {
			delete jogo.jogadores[socket.id];
			jogadoresProntos.delete(socket.id);
			console.log(`Jogador ${socket.id} removido por desconexão.`);
		}

		// verifica se o jogador era o configurador e redefine se necessário
		if (socket.id === configurador) {
			configurador = null;
		}

		// verifica se não há mais jogadores na sala
		if (Object.keys(jogo.jogadores).length === 0) {
			// chama a função para resetar o jogo
			resetarJogo();
			console.log('todos os jogadores saíram. jogo reiniciado.');
		} else {
			// emite atualização para os jogadores restantes
			io.emit('update players', Object.keys(jogo.jogadores).length);
		}

		// log de desconexão
		console.log(`jogador desconectado: ${socket.id}`);
	}

	// vincula o evento de desconexão ao handler
	socket.on('disconnect', () => {
		handleDisconnect(socket, io, jogo, jogadoresProntos, configurador, resetarJogo);
	});

});

// nao colocamos muitas pois eh apenas teste
const palavrasDisponiveis = ["VASCO", "FLU", "BOTA", "OLARIA"];

// seleciona uma palavra aleatoria
function selecionarPalavraAleatoria() {
	return palavrasDisponiveis[Math.floor(Math.random() * palavrasDisponiveis.length)];
}

// iniciar o jogo
function iniciarJogo() {
	console.log("Iniciando o jogo...");
	jogoIniciado = true;
	jogo.palavra = selecionarPalavraAleatoria(); // seleciona uma palavra aleatória

	// reseta tentativas e vidas para todos os jogadores
	for (const id in jogo.jogadores) {
		jogo.jogadores[id].tentativas = [];
		jogo.jogadores[id].vidas = 6;
	}

	// atualiza o estado inicial para todos os clientes
	io.emit("atualizar jogo", {
		estado: "_ ".repeat(jogo.palavra.length).trim(), // formato mais legível
		vidas: 6,
		tentativas: []
	});
	io.emit("mensagem", "O jogo começou! Boa sorte!");
}

// Finalizar o jogo
function finalizarJogo(mensagem) {
	console.log(mensagem);
	io.emit('mensagem', mensagem); // Consistente com outros eventos em português

	console.log('Jogo finalizado. Aguardando reinício.');

	// Opção de encerramento manual (apenas para testes ou necessidades específicas)
	const finalizarServidor = false; // Altere para true se quiser encerrar o servidor
	if (finalizarServidor) {
		setTimeout(() => {
			console.log('Encerrando servidor...');
			process.exit(0); // Finaliza o servidor
		}, 5000);
	}
}

// Quando reinicia o jogo, reinicia a palavra e o estado completo
function reiniciarEstado() {
	jogoIniciado = false;
	jogadoresProntos.clear();
	// Mantém os jogadores existentes e limpa seus atributos
	for (const id in jogo.jogadores) {
		jogo.jogadores[id] = {
			...jogo.jogadores[id],
			tentativas: [],
			vidas: 6,
		};
	}
	jogo.palavra = null; // A palavra será configurada na função iniciarJogo
	jogo.vencedor = null;
	maxJogadores = 1;
	configurador = null;
}

// Iniciar servidor
server.listen(3000, () => {
	console.log('Servidor rodando na porta 3000');
});
