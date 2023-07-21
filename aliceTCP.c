#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/des.h>

void generateSymmetricKey(unsigned char *key)
{
    srand(time(NULL));

    for (int i = 0; i < 8; i++)
    {
        key[i] = (unsigned char)rand();
    }
}

int connectToServer(const char *ipAddress, int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress, &serverAddr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Error connecting");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

unsigned char *readFile(const char *filename, long *fileSize)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        perror("Error opening the file");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    *fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    const int header_size = 54;
    if (*fileSize <= header_size)
    {
        perror("Arquivo BMP inválido");
        fclose(file);
        return 1;
    }
    int body_size = *fileSize - header_size;

    unsigned char *fileBuffer = (unsigned char *)malloc(*fileSize);
    if (!fileBuffer)
    {
        perror("Error allocating memory");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    if (fseek(file, header_size, SEEK_SET) != 0)
    {
        perror("Erro ao posicionar no início do corpo do arquivo BMP");
        fclose(file);
        free(fileBuffer);
        return 1;
    }

    if (fread(fileBuffer, 1, body_size, file) != body_size)
    {
        perror("Error reading the file");
        free(fileBuffer);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fclose(file);
    return fileBuffer;
}

void encryptContent(unsigned char *content, long contentSize, const unsigned char *key)
{
    DES_cblock desKey;
    DES_key_schedule keySchedule;
    memcpy(desKey, key, 8);
    DES_set_key_unchecked(&desKey, &keySchedule);

    int numBlocks = contentSize / 8;
    int remainingBytes = contentSize % 8;

    for (int i = 0; i < numBlocks; i++)
    {
        DES_ecb_encrypt(content + (i * 8), content + (i * 8), &keySchedule, DES_ENCRYPT);
    }

    if (remainingBytes > 0)
    {
        DES_ecb_encrypt(content + (numBlocks * 8), content + (numBlocks * 8), &keySchedule, DES_ENCRYPT);
    }
}

int main(int argc, char *argv[])
{
    // system("clear");
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <server_ip_address> <server_port>\n", argv[0]);
        return 1;
    }

    unsigned char key[8];
    generateSymmetricKey(key);

    int socket_fd = connectToServer(argv[1], atoi(argv[2]));

    if (send(socket_fd, key, sizeof(key), 0) != sizeof(key))
    {
        perror("Error sending the key");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    printf("Alice: Key succesfully sent!\n");

    long fileSize;
    unsigned char *fileBuffer = readFile("fractaljulia.bmp", &fileSize);

    encryptContent(fileBuffer, fileSize, key);

    if (send(socket_fd, fileBuffer, fileSize, 0) != fileSize)
    {
        perror("Error sending the encrypted content");
        free(fileBuffer);
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    printf("Alice: Encrypted message succesfully sent!\n");

    free(fileBuffer);
    close(socket_fd);

    return 0;
}
