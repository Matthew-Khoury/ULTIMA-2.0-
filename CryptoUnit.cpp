#include "CryptoUnit.h"

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <cstring>
#include <iostream>

static EVP_PKEY* global_pkey = nullptr;

// RSA key generation
bool CryptoUnit::generateRSAKeysToFiles(const std::string& pub, const std::string& priv)
{
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (!ctx) return false;

    EVP_PKEY* pkey = nullptr;

    EVP_PKEY_keygen_init(ctx);
    EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048);
    EVP_PKEY_keygen(ctx, &pkey);

    FILE* fpub = fopen(pub.c_str(), "wb");
    FILE* fpriv = fopen(priv.c_str(), "wb");

    if (!fpub || !fpriv) return false;

    PEM_write_PUBKEY(fpub, pkey);
    PEM_write_PrivateKey(fpriv, pkey, NULL, NULL, 0, NULL, NULL);

    fclose(fpub);
    fclose(fpriv);

    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(ctx);

    return true;
}

void CryptoUnit::initialize()
{
    if (global_pkey != nullptr)
        return;

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);

    EVP_PKEY_keygen_init(ctx);
    EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048);

    EVP_PKEY_keygen(ctx, &global_pkey);

    EVP_PKEY_CTX_free(ctx);
}

// RSA
static EVP_PKEY* loadPublic()
{
    FILE* f = fopen("ULTIMA_public.pem", "rb");
    if (!f) return nullptr;
    EVP_PKEY* key = PEM_read_PUBKEY(f, NULL, NULL, NULL);
    fclose(f);
    return key;
}

static EVP_PKEY* loadPrivate()
{
    FILE* f = fopen("ULTIMA_private.pem", "rb");
    if (!f) return nullptr;
    EVP_PKEY* key = PEM_read_PrivateKey(f, NULL, NULL, NULL);
    fclose(f);
    return key;
}

std::vector<unsigned char> CryptoUnit::rsaEncryptKey(
    const std::vector<unsigned char>& key)
{
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(global_pkey, NULL);

    EVP_PKEY_encrypt_init(ctx);
    EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);

    size_t outlen = 0;

    // get required size
    EVP_PKEY_encrypt(ctx, NULL, &outlen,
                     key.data(), key.size());

    std::vector<unsigned char> out(outlen);

    EVP_PKEY_encrypt(ctx,
                     out.data(), &outlen,
                     key.data(), key.size());

    EVP_PKEY_CTX_free(ctx);

    out.resize(outlen);
    return out;
}

std::vector<unsigned char> CryptoUnit::rsaDecryptKey(
    const std::vector<unsigned char>& enc)
{
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(global_pkey, NULL);

    EVP_PKEY_decrypt_init(ctx);
    EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);

    size_t outlen = 0;

    EVP_PKEY_decrypt(ctx, NULL, &outlen,
                     enc.data(), enc.size());

    std::vector<unsigned char> out(outlen);

    EVP_PKEY_decrypt(ctx,
                     out.data(), &outlen,
                     enc.data(), enc.size());

    EVP_PKEY_CTX_free(ctx);

    out.resize(outlen);
    return out;
}

// AES CTR
std::vector<unsigned char> CryptoUnit::aesEncrypt(
    const std::vector<unsigned char>& plain,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    std::vector<unsigned char> out(plain.size() + 32);
    int len, total = 0;

    EVP_EncryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, key.data(), iv.data());

    EVP_EncryptUpdate(ctx, out.data(), &len, plain.data(), plain.size());
    total = len;

    EVP_EncryptFinal_ex(ctx, out.data() + len, &len);
    total += len;

    EVP_CIPHER_CTX_free(ctx);
    out.resize(total);

    return out;
}

std::vector<unsigned char> CryptoUnit::aesDecrypt(
    const std::vector<unsigned char>& cipher,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    std::vector<unsigned char> out(cipher.size() + 32);
    int len, total = 0;

    EVP_DecryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, key.data(), iv.data());

    EVP_DecryptUpdate(ctx, out.data(), &len, cipher.data(), cipher.size());
    total = len;

    EVP_DecryptFinal_ex(ctx, out.data() + len, &len);
    total += len;

    EVP_CIPHER_CTX_free(ctx);
    out.resize(total);

    return out;
}
