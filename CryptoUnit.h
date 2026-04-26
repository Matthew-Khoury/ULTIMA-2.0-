#ifndef CRYPTO_UNIT_H
#define CRYPTO_UNIT_H

#include <vector>
#include <string>
#include <cstdint>
#pragma once

// crypto packet shared by IPC and MMU
struct CryptoPacket
{
	std::vector<unsigned char> rsa_enc_key;  // 256 bytes
	std::vector<unsigned char> iv;           // 16 bytes
	std::vector<unsigned char> cipher;       // AES payload
};

// crypto unit
class CryptoUnit
{
public:
	static bool generateRSAKeysToFiles(const std::string& pub, const std::string& priv);

	static std::vector<unsigned char> rsaEncryptKey(const std::vector<unsigned char>& key);
	static std::vector<unsigned char> rsaDecryptKey(const std::vector<unsigned char>& enc);

	static std::vector<unsigned char> aesEncrypt(
		const std::vector<unsigned char>& plaintext,
		const std::vector<unsigned char>& key,
		const std::vector<unsigned char>& iv);

	static std::vector<unsigned char> aesDecrypt(
		const std::vector<unsigned char>& cipher,
		const std::vector<unsigned char>& key,
		const std::vector<unsigned char>& iv);

	static void initialize();
};

#endif
