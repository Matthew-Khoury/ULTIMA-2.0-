#ifndef AES_MESSAGE_ENCRYPTION_H
#define AES_MESSAGE_ENCRYPTION_H

#include <string>

std::string encryptMessage(const std::string& plaintext);
std::string decryptMessage(const std::string& ciphertext_hex);

#endif