#ifndef TLSE_ADDITIONS_H
#define TLSE_ADDITIONS_H

#include "tlse.h"

#define BIO_NOCLOSE 0
#define SSL_ERROR_WANT_READ 42
#define SSL_ERROR_WANT_WRITE 43
#define CRYPTO_LOCK 0
#define SSL_MODE_AUTO_RETRY 0
#define SSL_OP_ALL 0
#define SSL_OP_NO_SSLv2 0
#define SSL_OP_NO_SSLv3 0
#define SSL_OP_NO_COMPRESSION 0
#define SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION 0
#define SHA256_CTX bool
#define SHA512_CTX bool

#define MD5_CTX bool
#define X509_STORE void
#define X509 void
#define X509_V_OK 0
#define X509_STORE_CTX void
#define EVP_PKEY void

#define X509_NAME void

#define 	GEN_DNS   2
#define 	GEN_IPADD   7


#define SHA256_DIGEST_LENGTH 0

int SHA256_Init(SHA256_CTX *c) {
    assert(0);
}
int SHA256_Update(SHA256_CTX *c, const void *data, size_t len) {
    assert(0);
}
int SHA256_Final(unsigned char *md, SHA256_CTX *c) {
    assert(0);
}

long SSL_CTX_set_options(SSL_CTX *ctx, long options) {
    assert(0);
}

void X509_STORE_free(X509_STORE *v) {
    assert(0);
}

void SSL_set_verify(SSL *s, int mode,
                    int (*verify_callback)(int, X509_STORE_CTX *)) {
    assert(0);
}

int SSL_CTX_load_verify_locations(SSL_CTX *ctx, const char *CAfile,
                                  const char *CApath) {
    assert(0);
}

int SSL_CTX_set_default_verify_paths(SSL_CTX *ctx) {
    assert(0);
}

int SSL_CTX_set_default_verify_dir(SSL_CTX *ctx);

int SSL_CTX_set_default_verify_file(SSL_CTX *ctx);


#define SHA512_DIGEST_LENGTH 0

int SHA512_Init(SHA256_CTX *c) {
    assert(0);
}
int SHA512_Update(SHA256_CTX *c, const void *data, size_t len) {
    assert(0);
}
int SHA512_Final(unsigned char *md, SHA256_CTX *c) {
    assert(0);
}

#define MD5_DIGEST_LENGTH 0

int MD5_Init(SHA256_CTX *c) {
    assert(0);
}
int MD5_Update(SHA256_CTX *c, const void *data, size_t len) {
    assert(0);
}
int MD5_Final(unsigned char *md, SHA256_CTX *c) {
    assert(0);
}


int SSLv23_server_method () {
    return  SSLv3_server_method();
}

int SSLv23_client_method () {
    return  SSLv3_client_method();
}

int CRYPTO_num_locks() {
    return 0;
}

void CRYPTO_set_locking_callback(void (int, int, const char *, int)) {
   // sure
}

void ERR_free_strings() {
    assert(0);
}

int SSL_CTX_use_certificate_chain_file(SSL_CTX *ctx, const char *file) {
    assert(0);
}

X509_STORE *SSL_CTX_get_cert_store(const SSL_CTX *ctx) {
    assert(0);
}

int SSL_set_tlsext_host_name(const SSL *s, const char *name) {
    assert(0);
}

long SSL_clear_mode(SSL *ssl, long mode) {
    assert(0);
}

int SSL_CTX_use_certificate(SSL_CTX *ctx, X509 *x) {
    assert(0);
}

int SSL_CTX_use_PrivateKey(SSL_CTX *ctx, EVP_PKEY *pkey) {
    assert(0);
}

void SSL_CTX_set_cert_store(SSL_CTX *ctx, X509_STORE *store) {
    assert(0);
}

long SSL_get_verify_result(const SSL *ssl) {
    assert(0);
}

X509 *SSL_get_peer_certificate(const SSL *ssl) {
    assert(0);
}

void X509_free(X509 *a) {
    assert(0);
}

X509_NAME *X509_get_subject_name(const X509 *x) {
    assert(0);
}
int NID_subject_alt_name;
int NID_commonName;

void *X509_get_ext_d2i(const X509 *x, int nid, int *crit, int *idx) {
    assert(0);
}

int X509_NAME_get_text_by_NID(X509_NAME *name, int nid, char *buf, int len) {
    assert(0);
}

#endif
