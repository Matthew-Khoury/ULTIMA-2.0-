// Uses AES-256-CBC with OpenSSL

#include <iostream>
#include <cstring>
#include <iomanip>
#include <openssl/evp.h>

using namespace std;

// Print bytes as hex
void printHex(const unsigned char* data, int len) {
    for (int i = 0; i < len; i++)
        cout << hex << setw(2) << setfill('0') << (int)data[i];
    cout << dec << endl;
}

// 256-bit key (32 bytes)
unsigned char key[32] = "0123456789012345678901234567890";

// 128-bit IV (16 bytes)
unsigned char iv[16] = "012345678901234";


// ---------------- ENCRYPT FUNCTION ----------------

string encryptMessage(const string& plaintext) {

    unsigned char ciphertext[128];

    int len;
    int ciphertext_len;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

    // Initialize Encryption
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

    // Encrypt plaintext
    EVP_EncryptUpdate(ctx, ciphertext, &len,
                      (unsigned char*)plaintext.c_str(),
                      plaintext.length());
    ciphertext_len = len;

    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    // Convert to hex string
    string result;
    char buffer[3];

    for (int i = 0; i < ciphertext_len; i++) {
        sprintf(buffer, "%02x", ciphertext[i]);
        result += buffer;
    }

    return result;
}


// ---------------- DECRYPT FUNCTION ----------------

string decryptMessage(const string& ciphertext_hex) {

    unsigned char ciphertext[128];
    unsigned char decryptedtext[128];

    int len;
    int decrypted_len;
    int ciphertext_len = 0;

    // Convert hex string back to bytes
    for (int i = 0; i < ciphertext_hex.length(); i += 2) {
        string byteString = ciphertext_hex.substr(i, 2);
        ciphertext[ciphertext_len++] = (unsigned char)strtol(byteString.c_str(), NULL, 16);
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

    EVP_DecryptUpdate(ctx, decryptedtext, &len,
                      ciphertext, ciphertext_len);
    decrypted_len = len;

    EVP_DecryptFinal_ex(ctx, decryptedtext + len, &len);
    decrypted_len += len;

    decryptedtext[decrypted_len] = '\0';

    EVP_CIPHER_CTX_free(ctx);

    return string((char*)decryptedtext);
}