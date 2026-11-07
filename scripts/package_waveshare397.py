"""Package compiler outputs only; never read a device dump or an SD backup."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import zipfile


def validate_image(path):
    data = path.read_bytes()
    if len(data) < 24 or data[0] != 0xE9 or struct.unpack_from('<H', data, 12)[0] != 9:
        raise ValueError(f'{path.name}: not an ESP32-S3 application/bootloader')
    offset = 24
    checksum = 0xEF
    for _ in range(data[1]):
        _, size = struct.unpack_from('<II', data, offset)
        offset += 8
        end = offset + size
        if end > len(data):
            raise ValueError(f'{path.name}: truncated segment')
        for value in data[offset:end]:
            checksum ^= value
        offset = end
    checksum_at = offset + (15 - offset % 16)
    if data[checksum_at] != checksum:
        raise ValueError(f'{path.name}: invalid image checksum')
    if data[23] != 1:
        raise ValueError(f'{path.name}: expected appended SHA-256')
    digest_at = checksum_at + 1
    if hashlib.sha256(data[:digest_at]).digest() != data[digest_at:digest_at + 32]:
        raise ValueError(f'{path.name}: invalid image SHA-256')
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-root', type=Path, required=True)
    parser.add_argument('--boot-app0', type=Path, required=True)
    parser.add_argument('--sdk-root', type=Path, required=True)
    parser.add_argument('--dependency-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--reader-commit', required=True)
    parser.add_argument('--sdk-commit', required=True)
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[1]
    for commit in (args.reader_commit, args.sdk_commit):
        if not re.fullmatch(r'[0-9a-f]{40}', commit):
            raise ValueError('Supply full reader and SDK publication commit IDs')
    if args.output.exists():
        raise ValueError('Choose a new output directory; existing packages are never overwritten')
    payload = {}
    for environment, folder in [('waveshare397', 'reader'), ('waveshare397-diag', 'diagnostic')]:
        build = args.build_root / environment
        app = validate_image(build / 'firmware.bin')
        validate_image(build / 'bootloader.bin')
        if len(app) > 0x640000:
            raise ValueError('Application exceeds the 0x640000-byte partition')
        if environment == 'waveshare397' and b'CROSSPOINT-BOARD-V1:waveshare397;' not in app:
            raise ValueError('Reader firmware is missing its Waveshare board tag')
        payload[f'{folder}/firmware-{environment}.bin'] = build / 'firmware.bin'
        for name in ['bootloader.bin', 'partitions.bin']:
            payload[f'{folder}/{name}'] = build / name
        payload[f'{folder}/boot_app0.bin'] = args.boot_app0
        if (build / 'partitions.bin').read_bytes()[:2] != b'\xaa\x50':
            raise ValueError('Invalid ESP partition table')
    payload['INSTALL.md'] = project / 'docs/waveshare-397-install.md'
    payload['RELEASE-NOTES.md'] = project / 'docs/waveshare-397-release.md'
    payload['waveshare-397-support.md'] = project / 'docs/waveshare-397-support.md'
    payload['LICENSE'] = project / 'LICENSE'
    payload['licenses/FreeInk-LICENSE'] = args.sdk_root / 'LICENSE'
    payload['licenses/FreeInk-NOTICE'] = args.sdk_root / 'NOTICE'
    payload['platformio.ini'] = project / 'platformio.ini'
    payload['partitions.csv'] = project / 'partitions.csv'
    # Only license notices from installed libraries; never copy a whole user directory.
    for path in args.dependency_root.rglob('*'):
        if path.is_file() and path.name.upper().split('.')[0] in ('LICENSE', 'COPYING', 'NOTICE'):
            payload['licenses/dependencies/' + path.relative_to(args.dependency_root).as_posix()] = path
    for source in payload.values():
        if not source.is_file():
            raise FileNotFoundError(source)
    args.output.mkdir(parents=True)
    for name, source in payload.items():
        target = args.output / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
    # Keep the copies of the public guide navigable after ZIP extraction.
    for name in ('INSTALL.md', 'RELEASE-NOTES.md', 'waveshare-397-support.md'):
        target = args.output / name
        text = target.read_text(encoding='utf-8')
        text = text.replace('(waveshare-397-install.md)', '(INSTALL.md)')
        text = text.replace('(waveshare-397-release.md)', '(RELEASE-NOTES.md)')
        target.write_text(text, encoding='utf-8')
    manifest = {
        'status': 'experimental prerelease',
        'board': 'Waveshare ESP32-S3 e-Paper 3.97',
        'flash_bytes': 16777216,
        'flash_offsets': {'bootloader': '0x0', 'partitions': '0x8000',
                          'boot_app0': '0xe000', 'application': '0x10000'},
        'reader_base': 'd28b9390a2da1db88d916a330bb9a84ddfec0818',
        'sdk_base': '310ec61506fc915836db7799a2e7f4fc135a570d',
        'reader_commit': args.reader_commit,
        'sdk_commit': args.sdk_commit,
        'source_note': 'Published source includes the tested port plus documentation and formatting changes.',
        'binary_origin': '2026-08-28 PlatformIO compiler outputs, not a device flash dump; see release notes.',
        'verification': 'Both images pass ESP32-S3 header, checksum and appended SHA-256 validation.',
    }
    (args.output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    hashes = []
    for path in sorted(args.output.rglob('*')):
        if path.is_file():
            hashes.append(f'{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.relative_to(args.output).as_posix()}')
    (args.output / 'SHA256SUMS.txt').write_text('\n'.join(hashes) + '\n', encoding='utf-8')
    archive_path = Path(str(args.output) + '.zip')
    with zipfile.ZipFile(archive_path, 'x', compression=zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(args.output.rglob('*')):
            if path.is_file():
                archive.write(path, path.relative_to(args.output).as_posix())
    with zipfile.ZipFile(archive_path) as archive:
        if archive.testzip() is not None:
            raise ValueError('ZIP integrity verification failed')
        for entry in hashes:
            digest, name = entry.split('  ', 1)
            if hashlib.sha256(archive.read(name)).hexdigest() != digest:
                raise ValueError(f'ZIP SHA-256 mismatch: {name}')
    print(f'Packaged and verified {len(payload)} input files: {archive_path}')
    print(f'ZIP SHA-256: {hashlib.sha256(archive_path.read_bytes()).hexdigest()}')


if __name__ == '__main__':
    main()
