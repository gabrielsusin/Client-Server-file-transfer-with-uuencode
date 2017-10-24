/*****************************************************
Trabalho pratico 2 - Redes de Computadores I
Faculdade de Engenharia - PUCRS
------------------------------------------------------
Servidor com decodificação uucode

Alunos:Gabriel Fasoli Susin - gabriel.susin@acad.pucrs.br
       Augusto Bergamin     - augusto.bergamin@acad.pucrs.br

Professor: Marcos A. Stemmer
Código modelo escrito por: Marcos A. Stemmer
-------
Instruções para compilar:

	gcc -o servuu serv-uu.c -lm

Para executar:
	
	./client
-------
*****************************************************/
#include <stdio.h>
#ifdef unix
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <math.h>
#else
#include <winsock2.h>
#endif

#define PORT 7400
#define MAXMSG 512
#define MAXFILENAME 30

/* Rotina geral de tratamento de erro fatal
   escreve a mensagem e termina o programa */
void erro_fatal(char *mensagem)
{
perror(mensagem);
exit(EXIT_FAILURE);
}


// Função para transformar de octal para decimal
long long convert8To10(int octal){
    int decimal = 0;
    int i = 0;
    while(octal != 0){
        decimal += (octal%10) * pow(8,i);
        ++i;
        octal/=10;
	}
    i = 1;
    return decimal;
}

/*Funcao cria socket para o servidor*/
int cria_soquete_serv(int porta)
{
	int sock, b;
	struct sockaddr_in nome;
	sock=socket(PF_INET, SOCK_STREAM, 0);/* Cria um soquete */
	if(sock < 0) erro_fatal("socket");
	nome.sin_family=AF_INET;	
	nome.sin_port=htons(porta);	
	nome.sin_addr.s_addr=htonl(INADDR_ANY);	
	b=bind(sock, (struct sockaddr *)&nome, sizeof(nome)); /* bind: Liga um socket a uma porta de servico */
	if(b<0) erro_fatal("bind");
	/* Libera para atender conexoes na porta */
	if(listen(sock,1)<0)  erro_fatal("socket");
	return sock;
}

//Função para decode de uuline
int decode_line(char * dados, char *uuline)
{
int i,j,n;
n=(*uuline++)-0x20;
if(n > 45 || n < 1) return -1;
for(i=0; i < n; uuline+=4) {
	j=4;
	while(j--) {
		uuline[j]-= 0x20;
		if(uuline[j]==0x40) uuline[j]=0;
		if(uuline[j] & (-0x40)) return -2;
		}
	dados[i++]=(uuline[0] << 2) | (uuline[1] >> 4);
	dados[i++]=(uuline[1] << 4) | (uuline[2] >> 2);
	dados[i++]=(uuline[2] << 6) | uuline[3];
	}
return n;
}


/* Envia string de texto */
int envia_pro_cliente(int filedes, char *msg)
{
int nbytes;
nbytes=send(filedes, msg, strlen(msg)+1,0);
if(nbytes<0) erro_fatal("send");
return nbytes;
}


int main()
{
	int sock, fd_sock;
	FILE *arq;
	char buffer[MAXMSG], buffer2[64], permission2[3], filename2[MAXFILENAME];
	int nbytes,linelen,modo;
	char *permission, *filename, *p;
	struct sockaddr_in clientname;
	int size;

	/* Cria um socket para receber o contato inicial com um cliente */
	sock=cria_soquete_serv(PORT);
	printf("Socket inicial: handle=%d\n",sock);
	size=sizeof(clientname);

	/* Aceita a conexao e cria um socket para comunicar-se com este cliente */
	fd_sock=accept(sock, (struct sockaddr *)&clientname, &size);
	if(fd_sock < 0) erro_fatal("accept");
	fprintf(stderr, "Servidor: aceitei conexao do host \"%s\", handle=%d\n",inet_ntoa(clientname.sin_addr), fd_sock);
	/* Comunica-se com o cliente usando o socket gerado pelo accept */
	memset(filename2,0,sizeof(filename2));
	printf("Recebendo\n");
	while(1){
		memset(buffer,0,sizeof(buffer));
		memset(buffer2,0,sizeof(buffer2));
		nbytes = recv(fd_sock, buffer, MAXMSG-1,0); //Recebe uma linha de dados do cliente
		if(nbytes < 0) erro_fatal("recv");
		//Se linha começa com Begin
		if(strstr(buffer,"begin")!=NULL){
			permission = strtok(buffer," ");
			permission = strtok(NULL," ");//Pega permissões e coloca em char pointer
			filename = strtok(NULL," ");//Pega nome do arquivo e coloca em char pointer
			memcpy(permission2,permission,4);//Coloca a permissão do char pointer para um array de char
			memcpy(filename2,filename,strlen(filename)-1);//Coloca o nome do arquivo de um char pointer para um array de char
			modo = convert8To10(atoi(permission2)); // Transforma a permissão de octal para decimal
			arq = fopen(filename2,"w");//Abre arquivo para escrita
			if(arq!=NULL){//Se abrir corretamente manda '.'
				buffer[0] = '.'; buffer[1] = '\0';
				send(fd_sock, buffer, 2, 0);
				}
			else{// Caso tenha algum problema na abertura manda '#' com erro
				sprintf(buffer,"#Erro ao abrir arquivo");
				send(fd_sock, buffer,22, 0);
				sleep(10);
				break;
				}
			}
		//Se a linha possui end
		else if(strstr(buffer,"end")!=NULL){
			fclose(arq);//Fecha o arquivo
			if(chmod(filename2,modo)<0){printf("Não foi possivel atualizar a permissão");}//Muda a permissão do arquivo
			buffer[0] = '.'; buffer[1] = '\0';
			send(fd_sock, buffer, 2, 0); //Envia '.'
			break;//Sai do loop para fechar conexão e terminar programa
			}
		//Para o resto dos dados
		else{
			linelen = decode_line(buffer2,buffer);//Decodifica a linha
			fwrite(buffer2, 1, linelen, arq);// Escreve no arquivo
			buffer[0] = '.'; buffer[1] = '\0';
			send(fd_sock, buffer, 2, 0);// Envia resposta
        		}
		}
	
	close(fd_sock); // Fecha
	printf("END");	
	return 0;
}
