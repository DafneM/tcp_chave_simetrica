/* ******************************/
/* FGA/Eng. Software/ FRC       */
/* Prof. Fernando W. Cruz       */
/* Codigo: tcpClient2.c         */
/* ******************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/des.h>

#define MAX_SIZE    	80

void generateDESKey(char *key) {
    int i;
    srand(time(NULL)); // Seed do gerador de números aleatórios com o tempo atual

    // Preenche a chave com valores aleatórios entre 0 e 255 (8 bits)
    for (i = 0; i < 8; i++) {
        key[i] = rand() % 256;
    }
    key[8] = '\0'; // Terminador de string
}

void encryptDES(char *bufout, int bufout_length, char *key) {
    DES_key_schedule des_key;
    DES_set_key((DES_cblock *)key, &des_key);

    int i;
    for (i = 0; i < bufout_length; i += 8) {
        DES_ecb_encrypt((DES_cblock *)(bufout + i), (DES_cblock *)(bufout + i), &des_key, DES_ENCRYPT);
    }
}

int main(int argc,char * argv[]) {
	struct  sockaddr_in ladoServ; /* contem dados do servidor 	*/
	int     sd;          	      /* socket descriptor              */
	int     n,k;                  /* num caracteres lidos do servidor */
	char    bufout[MAX_SIZE];     /* bufout de dados enviados  */
	
	/* confere o numero de argumentos passados para o programa */
  	if(argc<3)  {
    	   printf("uso correto: %s <ip_do_servidor> <porta_do_servidor>\n", argv[0]);
    	   exit(1);  }

	memset((char *)&ladoServ,0,sizeof(ladoServ)); /* limpa estrutura */
	memset((char *)&bufout,0,sizeof(bufout));     /* limpa bufout */
	
	ladoServ.sin_family      = AF_INET; /* config. socket p. internet*/
	ladoServ.sin_addr.s_addr = inet_addr(argv[1]);
	ladoServ.sin_port        = htons(atoi(argv[2]));

    

	/* Cria socket */
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) {
		fprintf(stderr, "Criacao do socket falhou!\n");
		exit(1); }

	/* Conecta socket ao servidor definido */
	if (connect(sd, (struct sockaddr *)&ladoServ, sizeof(ladoServ)) < 0) {
		fprintf(stderr,"Tentativa de conexao falhou!\n");
		exit(1); }
	while (1) {
        char key[9]; // A chave do DES deve ter 8 bytes + 1 para o terminador '\0'
        generateDESKey(&key);
		send(sd, key, sizeof(key), 0);

    printf("\n");
    for(int i = 0; i<sizeof(key); i++){
        printf("%02x ", (unsigned char)key[i]);
    }

    FILE *file = fopen("fractaljulia.bmp", "rb");
    if (!file) {
        perror("Erro ao abrir o arquivo BMP original");
        return 1;
    }

    // Obtenha o tamanho do arquivo BMP
    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Verifique o tamanho do cabeçalho BMP (54 bytes) e calcule o tamanho do corpo
    const int header_size = 54;
    if (file_size <= header_size) {
        perror("Arquivo BMP inválido");
        fclose(file);
        return 1;
    }
    int body_size = file_size - header_size;

    // Aloque um bufout para armazenar o conteúdo do corpo do arquivo BMP
    char *bufout = (char *)malloc(body_size);
    if (!bufout) {
        perror("Erro ao alocar memória para o bufout");
        fclose(file);
        return 1;
    }

    // Ignore o cabeçalho BMP lendo diretamente o corpo no bufout
    if (fseek(file, header_size, SEEK_SET) != 0) {
        perror("Erro ao posicionar no início do corpo do arquivo BMP");
        fclose(file);
        free(bufout);
        return 1;
    }

    // Ler o conteúdo do corpo do arquivo BMP no bufout
    if (fread(bufout, 1, body_size, file) != body_size) {
        perror("Erro ao ler o corpo do arquivo BMP");
        fclose(file);
        free(bufout);
        return 1;
    }

    // Feche o arquivo, pois não precisamos mais dele após ler o corpo
    fclose(file);

    // Implemente aqui a lógica para criptografar o arquivo BMP usando a chave 'key'
    encryptDES(bufout, body_size, key);
		send(sd,&bufout,strlen(bufout),0); /* enviando dados ...  */
		if (strncmp(bufout, "FIM",3) == 0) 
			break;
	} /* fim while */
	printf("------- encerrando conexao com o servidor -----\n");
	close (sd);
	return (0);
} /* fim do programa */

