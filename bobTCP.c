/* *****************************/
/* FGA / Eng. Software / FRC   */
/* Prof. Fernando W. Cruz      */
/* Codigo: tcpServer2.c	       */
/* *****************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/des.h>

#define QLEN            5               /* tamanho da fila de clientes  */
#define MAX_SIZE	80		/* tamanho do buffer */

void decryptDES(char *buffer, int buffer_length, char *key) {
    DES_key_schedule des_key;
    DES_set_key_unchecked((DES_cblock *)key, &des_key);

    int i;
    for (i = 0; i < buffer_length; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(buffer + i), (DES_cblock *)(buffer + i), &des_key, DES_DECRYPT);
    }
}

int atende_cliente(int descritor, struct sockaddr_in endCli)  {
   char bufin[MAX_SIZE];
   int  n;
   while (1) {
 	memset(&bufin, 0x0, sizeof(bufin));
    char key[9]; 
	n = recv(descritor, &key, sizeof(key),0);

    printf("\n");
    for(int i = 0; i<sizeof(key); i++){
        printf("%02x ", (unsigned char)key[i]);
    }


    n = recv(descritor, &bufin, sizeof(bufin),0);

    decryptDES(bufin, sizeof(bufin), key);

    // Salve o bufin descriptografado em um novo arquivo BMP
    FILE *decrypted_file = fopen("arquivo_descriptografado.bmp", "wb");
    if (!decrypted_file) {
        perror("Erro ao criar o arquivo descriptografado");
        free(bufin);
        return 1;
    }

    fwrite(bufin, 1, sizeof(bufin), decrypted_file);
    fclose(decrypted_file);

    free(bufin);

	if (strncmp(bufin, "FIM", 3) == 0)
            break;
	fprintf(stdout, "[%s:%u] => %s\n", inet_ntoa(endCli.sin_addr), ntohs(endCli.sin_port), bufin);
   } /* fim while */
   fprintf(stdout, "Encerrando conexao com %s:%u ...\n\n", inet_ntoa(endCli.sin_addr), ntohs(endCli.sin_port));
   close (descritor);
 } /* fim atende_cliente */

int main(int argc, char *argv[]) {
   struct sockaddr_in endServ;  /* endereco do servidor   */
   struct sockaddr_in endCli;   /* endereco do cliente    */
   int    sd, novo_sd;          /* socket descriptors */
   int    pid, alen,n; 

   if (argc<3) {
	  printf("Digite IP e Porta para este servidor\n");
	  exit(1); }
   memset((char *)&endServ,0,sizeof(endServ)); /* limpa variavel endServ    */
   endServ.sin_family 		= AF_INET;           	/* familia TCP/IP   */
   endServ.sin_addr.s_addr 	= inet_addr(argv[1]); 	/* endereco IP      */
   endServ.sin_port 		= htons(atoi(argv[2])); /* PORTA	    */

   /* Cria socket */
   sd = socket(AF_INET, SOCK_STREAM, 0);
   if (sd < 0) {
     fprintf(stderr, "Falha ao criar socket!\n");
     exit(1); }

   /* liga socket a porta e ip */
   if (bind(sd, (struct sockaddr *)&endServ, sizeof(endServ)) < 0) {
     fprintf(stderr,"Ligacao Falhou!\n");
     exit(1); }

   /* Ouve porta */
   if (listen(sd, QLEN) < 0) {
     fprintf(stderr,"Falhou ouvindo porta!\n");
     exit(1); }

   printf("Servidor ouvindo no IP %s, na porta %s ...\n\n", argv[1], argv[2]);
   /* Aceita conexoes */
   alen = sizeof(endCli);
   for ( ; ; ) {
	 /* espera nova conexao de um processo cliente ... */	
	if ( (novo_sd=accept(sd, (struct sockaddr *)&endCli, &alen)) < 0) {
		fprintf(stdout, "Falha na conexao\n");
		exit(1); }
	fprintf(stdout, "Cliente %s: %u conectado.\n", inet_ntoa(endCli.sin_addr), ntohs(endCli.sin_port)); 
	atende_cliente(novo_sd, endCli);
   } /* fim for */
} /* fim do programa */

