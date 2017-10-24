/*****************************************************
Trabalho pratico 2 - Redes de Computadores I
Faculdade de Engenharia - PUCRS
------------------------------------------------------
Cliente com codificação uucode

Alunos:Gabriel Fasoli Susin - gabriel.susin@acad.pucrs.br
       Augusto Bergamin     - augusto.bergamin@acad.pucrs.br

Professor: Marcos A. Stemmer
Código modelo escrito por: Marcos A. Stemmer
-------

Instruções para compilar:

	gcc -o client client.c 

Para executar:
	
	./client <ip of server> <port> <file name>

-------
*****************************************************/

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

char uuline[256];

int espera(int sock, int tempo)
{
	struct timeval timeout;
	fd_set sock_fd_set;
	int r;
	timeout.tv_sec=tempo;
	timeout.tv_usec=0;
	FD_ZERO(&sock_fd_set);
	FD_SET(sock,&sock_fd_set);
	r=select(FD_SETSIZE, &sock_fd_set, NULL, NULL, tempo>0? &timeout: NULL);
	if(r<0)	exit(1);
	return r;
}

int wait_answer(int clientSocket)
{
	int dataLen = 0;
	char answer[20];
	char *answer_ptr = answer;
	if(espera(clientSocket,5)==0){exit(1);}
	dataLen = recv(clientSocket, answer_ptr, sizeof(answer), 0);
	answer_ptr += dataLen;
	if(answer[0] == '.')
		return 1;
	else
		while(espera(clientSocket,1) != 0)
		{
			dataLen = recv(clientSocket, answer_ptr, sizeof(answer), 0);
			answer_ptr += dataLen;
		}
	printf("ERRO: %s\n", answer);
	exit(1);
}

/*Codifica uma linha uu-encoding */
char* encode_line(unsigned char *data, int n)
{
char *b;
b = uuline;
*b++ = n + 0x20;
while(n > 0) {
	*b++ = ((*data & 0xFC) >> 2)+0x20;
	*b++ = (((*data & 0x03) << 4) | ((data[1] & 0xF0) >> 4))+0x20;
	*b++ = (((data[1] & 0x0F) << 2) | ((data[2] & 0xC0) >> 6))+0x20;
	*b++ = (data[2] & 0x3F)+0x20;
	data += 3; n -= 3;
	}
*b++='\r'; *b++='\n';
*b++='\0';
for(b=uuline; *b; b++) if(*b==0x20) *b=0x60;
return uuline;
}

int main(int argc, char const *argv[])
{
    int clientSocket = 0;
	int dataLen;
    char fileData[46];
    char encData[60];
    struct sockaddr_in serv_addr;
	struct stat st;
    FILE* file;

    if(argc != 4)
    {
        printf("\n Usage: %s <ip of server> <port> <file name>\n",argv[0]);
        return 1;
    }

    /* Passo 1 - Criar socket */
    if((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket: ");
        exit(1);
    }

    /* Passo 2 - Configura struct sockaddr_in */
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    /* converte Ip em formato string para o formato exigido pela struct sockaddr_in*/
    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        exit(1);
    }

    /* Passo 3 - Conectar ao servidor */
    printf("Conectando ao servidor\n");
    if( connect(clientSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       perror("connect: ");
       exit(1);
    }
	printf("Conectado\n");
    /* Passo 4 - Abrir arquivo */
	file = fopen(argv[3],"r");
	if(file == NULL)
	{
		printf("Não foi possivel abrir o arquivo\n");
		perror("");
	}

	/* Passo 5 - Enviar arquivo */
	memset(encData, '\0', sizeof(encData));
	if(stat(argv[3], &st))
	{
		perror("stat");
		return 1;
	}
	printf("Enviando Arquivo\n");

	sprintf(encData, "begin %o %s\n",st.st_mode & 0x1ff, argv[3]);
	send(clientSocket, encData, sizeof(encData)+1, 0);
	wait_answer(clientSocket);
    while(1)
    {
        dataLen = fread(fileData, 1, 45, file);
		usleep(1000);
        if(dataLen > 0)
        {
            sprintf(encData, "%s", encode_line((unsigned char*)fileData, dataLen));
            send(clientSocket, encData, sizeof(encData)+1, 0);
			wait_answer(clientSocket);
        }
        else if(dataLen < sizeof(fileData))
        {
			sprintf(encData, "end\n");
			send(clientSocket, encData, sizeof(encData), 0);
			wait_answer(clientSocket);
            printf("Arquivo Enviado\n");
            fclose(file);
            break;
        }
    }
    return 0;
}
