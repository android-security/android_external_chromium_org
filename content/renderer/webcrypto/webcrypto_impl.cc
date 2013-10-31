// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/webcrypto/webcrypto_impl.h"

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "third_party/WebKit/public/platform/WebArrayBuffer.h"
#include "third_party/WebKit/public/platform/WebCryptoAlgorithm.h"
#include "third_party/WebKit/public/platform/WebCryptoKey.h"

namespace content {

WebCryptoImpl::WebCryptoImpl() {
  Init();
}

// static
// TODO(eroman): This works by re-allocating a new buffer. It would be better if
//               the WebArrayBuffer could just be truncated instead.
void WebCryptoImpl::ShrinkBuffer(
    WebKit::WebArrayBuffer* buffer,
    unsigned new_size) {
  DCHECK_LE(new_size, buffer->byteLength());

  if (new_size == buffer->byteLength())
    return;

  WebKit::WebArrayBuffer new_buffer =
      WebKit::WebArrayBuffer::create(new_size, 1);
  DCHECK(!new_buffer.isNull());
  memcpy(new_buffer.data(), buffer->data(), new_size);
  *buffer = new_buffer;
}

// static
// TODO(eroman): Expose functionality in Blink instead.
WebKit::WebCryptoKey WebCryptoImpl::NullKey() {
  // Needs a non-null algorithm to succeed.
  return WebKit::WebCryptoKey::create(
      NULL, WebKit::WebCryptoKeyTypeSecret, false,
      WebKit::WebCryptoAlgorithm::adoptParamsAndCreate(
          WebKit::WebCryptoAlgorithmIdAesGcm, NULL), 0);
}

void WebCryptoImpl::encrypt(
    const WebKit::WebCryptoAlgorithm& algorithm,
    const WebKit::WebCryptoKey& key,
    const unsigned char* data,
    unsigned data_size,
    WebKit::WebCryptoResult result) {
  DCHECK(!algorithm.isNull());
  WebKit::WebArrayBuffer buffer;
  if (!EncryptInternal(algorithm, key, data, data_size, &buffer)) {
    result.completeWithError();
  } else {
    result.completeWithBuffer(buffer);
  }
}

void WebCryptoImpl::decrypt(
    const WebKit::WebCryptoAlgorithm& algorithm,
    const WebKit::WebCryptoKey& key,
    const unsigned char* data,
    unsigned data_size,
    WebKit::WebCryptoResult result) {
  DCHECK(!algorithm.isNull());
  WebKit::WebArrayBuffer buffer;
  if (!DecryptInternal(algorithm, key, data, data_size, &buffer)) {
    result.completeWithError();
  } else {
    result.completeWithBuffer(buffer);
  }
}

void WebCryptoImpl::digest(
    const WebKit::WebCryptoAlgorithm& algorithm,
    const unsigned char* data,
    unsigned data_size,
    WebKit::WebCryptoResult result) {
  DCHECK(!algorithm.isNull());
  WebKit::WebArrayBuffer buffer;
  if (!DigestInternal(algorithm, data, data_size, &buffer)) {
    result.completeWithError();
  } else {
    result.completeWithBuffer(buffer);
  }
}

void WebCryptoImpl::generateKey(
    const WebKit::WebCryptoAlgorithm& algorithm,
    bool extractable,
    WebKit::WebCryptoKeyUsageMask usage_mask,
    WebKit::WebCryptoResult result) {
  DCHECK(!algorithm.isNull());
  WebKit::WebCryptoKey key = NullKey();
  if (!GenerateKeyInternal(algorithm, extractable, usage_mask, &key)) {
    result.completeWithError();
  } else {
    DCHECK(key.handle());
    DCHECK_EQ(algorithm.id(), key.algorithm().id());
    DCHECK_EQ(extractable, key.extractable());
    DCHECK_EQ(usage_mask, key.usages());
    result.completeWithKey(key);
  }
}

void WebCryptoImpl::importKey(
    WebKit::WebCryptoKeyFormat format,
    const unsigned char* key_data,
    unsigned key_data_size,
    const WebKit::WebCryptoAlgorithm& algorithm_or_null,
    bool extractable,
    WebKit::WebCryptoKeyUsageMask usage_mask,
    WebKit::WebCryptoResult result) {
  WebKit::WebCryptoKey key = NullKey();
  if (!ImportKeyInternal(format,
                         key_data,
                         key_data_size,
                         algorithm_or_null,
                         extractable,
                         usage_mask,
                         &key)) {
    result.completeWithError();
    return;
  }
  DCHECK(key.handle());
  DCHECK(!key.algorithm().isNull());
  DCHECK_EQ(extractable, key.extractable());
  result.completeWithKey(key);
}

void WebCryptoImpl::sign(
    const WebKit::WebCryptoAlgorithm& algorithm,
    const WebKit::WebCryptoKey& key,
    const unsigned char* data,
    unsigned data_size,
    WebKit::WebCryptoResult result) {
  DCHECK(!algorithm.isNull());
  WebKit::WebArrayBuffer buffer;
  if (!SignInternal(algorithm, key, data, data_size, &buffer)) {
    result.completeWithError();
  } else {
    result.completeWithBuffer(buffer);
  }
}

void WebCryptoImpl::verifySignature(
    const WebKit::WebCryptoAlgorithm& algorithm,
    const WebKit::WebCryptoKey& key,
    const unsigned char* signature,
    unsigned signature_size,
    const unsigned char* data,
    unsigned data_size,
    WebKit::WebCryptoResult result) {
  DCHECK(!algorithm.isNull());
  bool signature_match = false;
  if (!VerifySignatureInternal(algorithm,
                               key,
                               signature,
                               signature_size,
                               data,
                               data_size,
                               &signature_match)) {
    result.completeWithError();
  } else {
    result.completeWithBoolean(signature_match);
  }
}

}  // namespace content