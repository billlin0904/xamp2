#!/usr/bin/env python3
"""Wrap a deployed XAMP2.app in a native macOS Installer wizard."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import plistlib
import shutil
import subprocess
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]


def run(*args, capture=False):
    return subprocess.run([str(x) for x in args], check=True, text=True,
                          stdout=subprocess.PIPE if capture else None).stdout


def build(args):
    os.environ.setdefault('DEVELOPER_DIR', '/Applications/Xcode.app/Contents/Developer')
    app = args.app.resolve()
    info = plistlib.loads((app/'Contents/Info.plist').read_bytes())
    version, minimum = info['CFBundleShortVersionString'], info['LSMinimumSystemVersion']
    identifier = info['CFBundleIdentifier'] + '.installer'
    run('codesign', '--verify', '--deep', '--strict', app)
    if args.notary_profile and not args.identity:
        raise RuntimeError('Notarization requires --identity (Developer ID Installer).')
    if args.identity:
        identities = run('security', 'find-identity', '-v', capture=True)
        matches = [line for line in identities.splitlines()
                   if args.identity in line and 'Developer ID Installer:' in line]
        if len(matches) != 1:
            raise RuntimeError('Select one valid Developer ID Installer identity including its private key.')
    output = ROOT/'out/package/macos'
    output.mkdir(parents=True, exist_ok=True)
    state = 'notarized' if args.notary_profile else ('signed' if args.identity else 'unsigned')
    destination = output/f'XAMP2-{version}-macOS-arm64-installer-{state}.pkg'
    with tempfile.TemporaryDirectory(prefix='installer-', dir=output) as work:
        work = Path(work)
        payload = work/'payload'
        payload.mkdir()
        run('ditto', app, payload/'XAMP2.app')
        components = work/'components.plist'
        run('pkgbuild', '--analyze', '--root', payload, components)
        entries = plistlib.loads(components.read_bytes())
        for entry in entries:
            entry['BundleIsRelocatable'] = False
            entry['BundleIsVersionChecked'] = True
            entry['BundleOverwriteAction'] = 'upgrade'
        components.write_bytes(plistlib.dumps(entries))
        run('pkgbuild', '--root', payload, '--install-location', '/Applications',
            '--component-plist', components, '--identifier', identifier,
            '--version', version, '--ownership', 'recommended', work/'XAMP2-component.pkg')
        resources = work/'Resources'
        shutil.copytree(ROOT/'setup/macos/installer', resources)
        for page in resources.glob('*.html'):
            page.write_text(page.read_text().replace('@VERSION@', version).replace('@MIN_OS@', minimum))
        root = ET.Element('installer-gui-script', minSpecVersion='2')
        ET.SubElement(root, 'title').text = 'XAMP2 '+version
        for element in ['welcome', 'readme', 'conclusion']:
            ET.SubElement(root, element, {'file': element+'.html', 'mime-type': 'text/html'})
        ET.SubElement(root, 'options', {'customize': 'never', 'require-scripts': 'false', 'hostArchitectures': 'arm64'})
        ET.SubElement(root, 'domains', enable_localSystem='true', enable_currentUserHome='false', enable_anywhere='false')
        allowed = ET.SubElement(ET.SubElement(root, 'volume-check'), 'allowed-os-versions')
        ET.SubElement(allowed, 'os-version', min=minimum)
        outline = ET.SubElement(root, 'choices-outline')
        ET.SubElement(outline, 'line', choice='xamp')
        choice = ET.SubElement(root, 'choice', id='xamp', visible='false', title='XAMP2')
        ET.SubElement(choice, 'pkg-ref', id=identifier)
        ref = ET.SubElement(root, 'pkg-ref', id=identifier, version=version, onConclusion='none')
        ref.text = 'XAMP2-component.pkg'
        must_close = ET.SubElement(ET.SubElement(root, 'pkg-ref', id=identifier), 'must-close')
        ET.SubElement(must_close, 'app', id=info['CFBundleIdentifier'])
        distribution = work/'Distribution.xml'
        ET.ElementTree(root).write(distribution, encoding='utf-8', xml_declaration=True)
        pending = work/'XAMP2.pkg'
        command = ['productbuild', '--distribution', distribution, '--resources', resources, '--package-path', work]
        if args.identity:
            command += ['--sign', args.identity, '--timestamp']
        run(*command, pending)
        if args.identity:
            run('pkgutil', '--check-signature', pending)
        if args.notary_profile:
            result = json.loads(run('xcrun', 'notarytool', 'submit', pending, '--keychain-profile', args.notary_profile,
                                    '--wait', '--output-format', 'json', capture=True))
            (output/'installer-notarization.json').write_text(json.dumps(result, indent=2))
            if result.get('status') != 'Accepted':
                # Preserve a rejected package for diagnosis without calling it notarized.
                shutil.copy2(pending, output/'XAMP2-installer-notarization-failed.pkg')
                raise RuntimeError('Apple notarization was not accepted; see installer-notarization.json')
            run('xcrun', 'stapler', 'staple', pending)
            run('xcrun', 'stapler', 'validate', pending)
            run('spctl', '--assess', '--type', 'install', '--verbose=2', pending)
        pending.replace(destination)
    digest = hashlib.sha256(destination.read_bytes()).hexdigest()
    destination.with_suffix('.pkg.sha256').write_text(digest+'  '+destination.name+'\n')
    print(destination)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('app', type=Path, help='Self-contained deployed .app (for example inside the generated DMG)')
    parser.add_argument('--identity', help='Developer ID Installer name or SHA-1')
    parser.add_argument('--notary-profile', help='Existing notarytool Keychain profile')
    build(parser.parse_args())
