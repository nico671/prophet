#ifndef PROPHET_SHA256_H
#define PROPHET_SHA256_H

#include <stdbool.h>
#include <stddef.h>

// REF: https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf

/** @brief Compute the SHA-256 digest of a file as lowercase hexadecimal. */
bool sha256_file(const char* path, char out[65]);

/** @brief Compute the SHA-256 digest of bytes as lowercase hexadecimal. */
bool sha256_bytes(const void* data, size_t size, char out[65]);

#endif // PROPHET_SHA256_H
