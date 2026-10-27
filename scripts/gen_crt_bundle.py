#!/usr/bin/env python3
"""
Generate certs/x509_crt_bundle.bin for HTTPS certificate verification.

arduino-esp32 2.x (WiFiClientSecure::setCACertBundle) expects the legacy
ESP-IDF 4.x bundle layout, which newer ESP-IDF generators no longer produce:

    u16 BE  certificate count
    per certificate, sorted by subject DER:
        u16 BE  subject name length
        u16 BE  public key length
        subject name (DER)
        public key (DER, SubjectPublicKeyInfo)

Usage:
    python3 scripts/gen_crt_bundle.py <cacrt_all.pem> [output.bin]

The Mozilla CA list ships with ESP-IDF at
components/mbedtls/esp_crt_bundle/cacrt_all.pem. Only the standard library is
used, so no extra packages are needed.
"""

import base64
import re
import struct
import sys


def der_element(buf, pos):
    """Return (tag, start, end) of the DER element at pos, where end is exclusive."""
    tag = buf[pos]
    length = buf[pos + 1]
    hdr = 2
    if length & 0x80:
        n = length & 0x7F
        length = int.from_bytes(buf[pos + 2:pos + 2 + n], 'big')
        hdr += n
    return tag, pos, pos + hdr + length


def der_children(buf, start, end):
    """Yield (tag, start, end) for each element inside the constructed element [start, end)."""
    _, _, _ = der_element(buf, start)
    length = buf[start + 1]
    pos = start + 2 + ((length & 0x7F) if length & 0x80 else 0)
    while pos < end:
        child = der_element(buf, pos)
        yield child
        pos = child[2]


def subject_and_key(der):
    """Extract subject Name and SubjectPublicKeyInfo DER from a certificate."""
    _, cs, ce = der_element(der, 0)
    tbs = next(der_children(der, cs, ce))
    fields = list(der_children(der, tbs[1], tbs[2]))
    if fields[0][0] == 0xA0:  # explicit [0] version
        fields = fields[1:]
    # serial, signature, issuer, validity, subject, subjectPublicKeyInfo
    subject, spki = fields[4], fields[5]
    return der[subject[1]:subject[2]], der[spki[1]:spki[2]]


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    src = sys.argv[1]
    out = sys.argv[2] if len(sys.argv) > 2 else 'certs/x509_crt_bundle.bin'

    with open(src) as f:
        pem = f.read()

    entries = {}
    for body in re.findall(r'-----BEGIN CERTIFICATE-----(.+?)-----END CERTIFICATE-----', pem, re.S):
        name, key = subject_and_key(base64.b64decode(body))
        entries[name] = key

    bundle = struct.pack('>H', len(entries))
    for name in sorted(entries):
        key = entries[name]
        bundle += struct.pack('>HH', len(name), len(key)) + name + key

    with open(out, 'wb') as f:
        f.write(bundle)
    print(f'{out}: {len(entries)} certificates, {len(bundle)} bytes')


if __name__ == '__main__':
    main()
