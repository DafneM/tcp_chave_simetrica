#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <openssl/des.h>

#pragma pack(push, 1)
typedef struct
{
    char signature[2];
    int fileSize;
    short reserved1;
    short reserved2;
    int dataOffset;
} BMPFileHeader;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    int headerSize;
    int width;
    int height;
    short planes;
    short bitsPerPixel;
    int compression;
    int imageSize;
    int xResolution;
    int yResolution;
    int colorsUsed;
    int importantColors;
} BMPInfoHeader;
#pragma pack(pop)

BMPFileHeader fileHeader;
BMPInfoHeader infoHeader;

void decryptContent(const unsigned char *encryptedBuffer, int encryptedSize, unsigned char *key, unsigned char *decryptedBuffer, int *decryptedSize)
{

    DES_cblock des_key;
    DES_key_schedule key_schedule;
    memcpy(des_key, key, 8);
    DES_set_key_unchecked(&des_key, &key_schedule);

    *decryptedSize = 0;

    for (int i = 0; i < encryptedSize; i += 8)
    {

        DES_ecb_encrypt(encryptedBuffer + i, decryptedBuffer + *decryptedSize, &key_schedule, DES_DECRYPT);
        *decryptedSize += 8;
    }
}

void initHeaderBmp()
{
    fileHeader.signature[0] = 'B';
    fileHeader.signature[1] = 'M';
    fileHeader.fileSize = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + (40 * 20 * 3);
    fileHeader.reserved1 = 0;
    fileHeader.reserved2 = 0;
    fileHeader.dataOffset = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

    infoHeader.headerSize = sizeof(BMPInfoHeader);
    infoHeader.width = 40;
    infoHeader.height = 20;
    infoHeader.planes = 1;
    infoHeader.bitsPerPixel = 24;
    infoHeader.compression = 0;
    infoHeader.imageSize = 40 * 20 * 3;
    infoHeader.xResolution = 0;
    infoHeader.yResolution = 0;
    infoHeader.colorsUsed = 0;
    infoHeader.importantColors = 0;
}

int main(int argc, char *argv[])
{
    // system("clear");
    initHeaderBmp();

    int sockfd, newsockfd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size;
    unsigned char key[8];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(argv[1]);
    serverAddr.sin_port = htons(atoi(argv[2]));

    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("Error binding socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 10) == -1)
    {
        perror("Error listening for connections");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Bob: Waiting for connections...\n");

    addr_size = sizeof(clientAddr);
    newsockfd = accept(sockfd, (struct sockaddr *)&clientAddr, &addr_size);
    if (newsockfd == -1)
    {
        perror("Error accepting connection");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int bytesRead = recv(newsockfd, key, 8, 0);
    if (bytesRead != 8)
    {
        perror("Error receiving key");
        close(newsockfd);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Bob: Simetric key received.\n");

    unsigned char encryptedBuffer[65536];
    int encryptedSize;

    bytesRead = recv(newsockfd, encryptedBuffer, sizeof(encryptedBuffer), 0);
    if (bytesRead == -1)
    {
        perror("Error receiving encrypted content");
        close(newsockfd);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    encryptedSize = bytesRead;

    printf("Bob: Encrypted message received from Alice.\n");

    unsigned char decryptedBuffer[65536];
    int decryptedSize;

    decryptContent(encryptedBuffer, encryptedSize, key, decryptedBuffer, &decryptedSize);

    FILE *outputFile = fopen("bob.bmp", "wb");
    if (!outputFile)
    {
        perror("Error opening output file");
        close(newsockfd);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Write the headers to the file
    fwrite(&fileHeader, sizeof(BMPFileHeader), 1, outputFile);
    fwrite(&infoHeader, sizeof(BMPInfoHeader), 1, outputFile);

    fwrite(decryptedBuffer, 1, decryptedSize, outputFile);
    fclose(outputFile);

    printf("Bob: Desencrypted message saved as 'bob.bmp'.\n");

    close(newsockfd);
    close(sockfd);
    return 0;
}
