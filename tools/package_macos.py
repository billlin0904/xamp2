#!/usr/bin/env python3
"""Build a self-contained Apple Silicon DMG; optionally Developer ID sign/notarize."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import plistlib
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SYSTEM = ('/usr/lib/', '/System/Library/')
MAGIC = {b'\xcf\xfa\xed\xfe', b'\xfe\xed\xfa\xcf', b'\xca\xfe\xba\xbe', b'\xbe\xba\xfe\xca'}


def run(*args, capture=False):
    return subprocess.run([str(a) for a in args], check=True, text=True,
                          stdout=subprocess.PIPE if capture else None).stdout


def macho_files(app):
    for p in app.rglob('*'):
        if p.is_file() and not p.is_symlink():
            with p.open('rb') as f:
                if f.read(4) in MAGIC:
                    yield p


def dependencies(p):
    return [line.strip().split(' (compatibility version')[0]
            for line in run('otool', '-L', p, capture=True).splitlines()[1:]
            if ' (compatibility version' in line]


def rpaths(p):
    return re.findall(r'cmd LC_RPATH\s+cmdsize \d+\s+path (.*?) \(offset',
                      run('otool', '-l', p, capture=True))


def package(args):
    os.environ.setdefault('DEVELOPER_DIR', '/Applications/Xcode.app/Contents/Developer')
    version = re.search(r'VERSION\s+(\d+\.\d+\.\d+)', (ROOT/'CMakeLists.txt').read_text())[1]
    source = ROOT/'out/build/macos-xcode/src/xamp/Release/xamp.app'
    output = ROOT/'out/package/macos'
    output.mkdir(parents=True, exist_ok=True)
    if args.identity:
        identities = run('security', 'find-identity', '-v', '-p', 'codesigning', capture=True)
        matches = [l for l in identities.splitlines() if args.identity in l and 'Developer ID Application:' in l]
        if len(matches) != 1:
            raise RuntimeError('Select exactly one installed Developer ID Application identity (including private key).')
    if args.notary_profile and not args.identity:
        raise RuntimeError('--notary-profile requires --identity')
    if not args.skip_build:
        subprocess.run(['cmake', '--preset', 'macos-xcode'], cwd=ROOT, check=True)
        subprocess.run(['cmake', '--build', '--preset', 'macos-xcode-release', '--parallel', '4'], cwd=ROOT, check=True)
    # A new staging directory preserves previous packages if deployment fails.
    with tempfile.TemporaryDirectory(prefix='stage-', dir=output) as work:
        stage = Path(work)
        app = stage/'XAMP2.app'
        run('ditto', source, app)
        contents = app/'Contents'
        executable = contents/'MacOS/xamp'
        frameworks = contents/'Frameworks'
        components = contents/'MacOS/components'
        resources = contents/'Resources'
        frameworks.mkdir(exist_ok=True)
        info_path = contents/'Info.plist'
        info = plistlib.loads(info_path.read_bytes())
        info.update(CFBundleIdentifier=args.bundle_id, CFBundleName='XAMP2',
                    CFBundleDisplayName='XAMP2', CFBundleShortVersionString=version,
                    CFBundleVersion=version, CFBundleIconFile='xamp2.icns',
                    NSHighResolutionCapable=True)
        shutil.copy2(ROOT/'src/xamp/xamp2.icns', resources/'xamp2.icns')
        # dlopen dependencies are not visible to macdeployqt's link scanner.
        runtime = ['/opt/homebrew/opt/ffmpeg@6/lib/lib'+name+'.dylib'
                   for name in ['avformat.60', 'avcodec.60', 'avutil.58', 'swresample.4']]
        runtime += ['/opt/homebrew/lib/lib'+name+'.dylib'
                    for name in ['FLAC', 'mecab', 'uchardet', 'opencc', 'deflate', 'samplerate', 'soxr', 'cue']]
        runtime += ['/opt/homebrew/opt/libarchive/lib/libarchive.dylib']
        runtime += ['/opt/homebrew/opt/icu4c@78/lib/lib'+name+'.dylib'
                    for name in ['icuuc', 'icui18n']]
        for lib in runtime:
            shutil.copy2(lib, components/Path(lib).name)
        shutil.copy2(contents/'MacOS/libpcm_dsd_converter.dylib', components/'libpcm_dsd_converter.dylib')
        (contents/'MacOS/libpcm_dsd_converter.dylib').unlink()
        mecab = resources/'mecab'
        shutil.copytree('/opt/homebrew/lib/mecab/dic/ipadic', mecab/'dic')
        (mecab/'mecabrc').write_text('# Dictionary path is supplied by XAMP.\n')
        (contents/'MacOS/mecab').symlink_to('../Resources/mecab')
        info_path.write_bytes(plistlib.dumps(info))
        for p in macho_files(app):
            archs = run('lipo', '-archs', p, capture=True).split()
            if archs != ['arm64']:
                thin = p.with_name(p.name+'.arm64')
                run('lipo', p, '-thin', 'arm64', '-output', thin)
                thin.replace(p)
        for p in macho_files(components):
            for dep in dependencies(p):
                sibling = components/Path(dep).name
                if dep.startswith('@rpath/') and sibling.exists() and sibling != p:
                    run('install_name_tool', '-change', dep, str(sibling), p)
        log = output/'macdeployqt.log'
        with log.open('w') as stream:
            subprocess.run([args.macdeployqt, str(app), '-no-codesign', '-verbose=1',
                            '-libpath='+str(components)], stdout=stream, stderr=subprocess.STDOUT, check=True)
        if any('ERROR:' in line and 'already deployed, skipping.' not in line for line in log.read_text().splitlines()):
            raise RuntimeError('macdeployqt reported errors; inspect '+str(log))
        # Resolve the transitive closure, then use paths relative to each loader.
        queue = list(macho_files(app))
        known = {p.name: p for p in queue}
        min_os = (14, 0)
        copied_sources = set(Path(p).resolve() for p in runtime)
        for p in queue:
            run('lipo', p, '-verify_arch', 'arm64')
            load_commands = run('otool', '-arch', 'arm64', '-l', p, capture=True)
            for v in re.findall(r'\bminos (\d+(?:\.\d+)+)', load_commands):
                min_os = max(min_os, tuple(map(int, v.split('.'))))
            own = run('otool', '-D', p, capture=True).splitlines()[1:]
            own = own[0].strip() if own else None
            search = rpaths(p) + rpaths(executable)
            for dep in dependencies(p):
                if dep == own or dep.startswith(SYSTEM):
                    continue
                def expand(value):
                    return Path(value.replace('@loader_path', str(p.parent)).replace('@executable_path', str(executable.parent)))
                candidates = [expand(dep)]
                if dep.startswith('@rpath/'):
                    tail = dep[len('@rpath/'):]
                    candidates = [frameworks/tail, components/tail] + [expand(r)/tail for r in search]
                target = next((c.resolve() for c in candidates if c.is_file()), None)
                if target is None:
                    target = known.get(Path(dep).name)
                if target is None:
                    raise RuntimeError(f'Unresolved dependency: {p}: {dep}')
                if not target.is_relative_to(app):
                    if '.framework/' in str(target):
                        raise RuntimeError('Framework not deployed by macdeployqt: '+str(target))
                    dest = frameworks/target.name
                    if not dest.exists():
                        shutil.copy2(target, dest)
                        copied_sources.add(target)
                        queue.append(dest)
                        known[dest.name] = dest
                    target = dest
                run('install_name_tool', '-change', dep,
                    '@loader_path/'+os.path.relpath(target, p.parent), p)
            if own:
                run('install_name_tool', '-id', '@rpath/'+p.name, p)
            for r in set(rpaths(p)):
                if r.startswith('/') and not r.startswith(SYSTEM):
                    run('install_name_tool', '-delete_rpath', r, p)
        # Include notices supplied by Homebrew for the bundled libraries and Qt.
        notices = resources/'licenses'
        notices.mkdir(exist_ok=True)
        shutil.copy2(ROOT/'LICENSE', notices/'XAMP-LICENSE')
        for bass_notice in (ROOT/'out/deps/bass-macos').glob('*/*.txt'):
            dest = notices/bass_notice.parent.name
            dest.mkdir(exist_ok=True)
            shutil.copy2(bass_notice, dest/bass_notice.name)
        ipadic_notice = Path('/opt/homebrew/opt/mecab-ipadic/COPYING')
        shutil.copy2(ipadic_notice, notices/'mecab-ipadic-COPYING')
        brew_roots = set()
        for p in copied_sources:
            if '/Cellar/' in str(p):
                parts = p.parts
                i = parts.index('Cellar')
                brew_roots.add(Path(*parts[:i+3]))
        # macdeployqt does not record source locations after rewriting its libraries.
        for p in Path('/opt/homebrew/Cellar').glob('*/*'):
            if any((p/'lib'/f.name).exists() for f in frameworks.iterdir()):
                brew_roots.add(p)
        for prefix in brew_roots:
            dest = notices/(prefix.parent.name+'-'+prefix.name)
            for item in prefix.iterdir():
                if re.match(r'(?i)(license|copying|copyright|notice)', item.name):
                    dest.mkdir(exist_ok=True)
                    if item.is_dir():
                        shutil.copytree(item, dest/item.name, dirs_exist_ok=True)
                    else:
                        shutil.copy2(item, dest/item.name)
        info['LSMinimumSystemVersion'] = '.'.join(map(str, min_os))
        info_path.write_bytes(plistlib.dumps(info))
        # Fail on any dependency that escapes the bundle (except macOS system libs).
        for p in macho_files(app):
            own_lines = run('otool', '-D', p, capture=True).splitlines()[1:]
            own = own_lines[0].strip() if own_lines else None
            for dep in dependencies(p):
                if dep == own or dep.startswith(SYSTEM):
                    continue
                if not dep.startswith('@loader_path/'):
                    raise RuntimeError(f'Nonportable dependency: {p}: {dep}')
                target = (p.parent/dep[len('@loader_path/'):]).resolve()
                if not target.is_relative_to(app) or not target.is_file():
                    raise RuntimeError(f'Dependency escapes bundle: {p}: {dep}')
        sign = ['codesign', '--force', '--sign', args.identity or '-']
        if args.identity:
            sign += ['--options', 'runtime', '--timestamp']
        for p in macho_files(app):
            if p != executable:
                run(*sign, p)
        for framework in frameworks.glob('*.framework'):
            run(*sign, framework)
        run(*sign, app)
        run('codesign', '--verify', '--deep', '--strict', '--verbose=2', app)
        if args.smoke_test:
            snapshot = output/'smoke-test.png'
            snapshot.unlink(missing_ok=True)
            env = dict(os.environ, XAMP_SMOKE_TEST='1', XAMP_MESSAGEBOX_SNAPSHOT=str(snapshot))
            subprocess.run([str(executable)], env=env, check=True, timeout=60)
            if not snapshot.is_file():
                raise RuntimeError('Startup smoke test did not reach the application window')
        (stage/'Applications').symlink_to('/Applications')
        (stage/'安裝說明.txt').write_text('將 XAMP2.app 拖曳到 Applications，即可安裝。\n適用 Apple Silicon；最低 macOS '+info['LSMinimumSystemVersion']+'。\n'+('此為本機測試包，尚未經 Developer ID 簽署與 Apple 公證。\n' if not args.identity else ''))
        label = 'notarized' if args.notary_profile else ('signed' if args.identity else 'local-test')
        dmg = output/f'XAMP2-{version}-macOS-arm64-{label}.dmg'
        pending = output/f'.XAMP2-{version}-pending.dmg'
        run('hdiutil', 'create', '-ov', '-format', 'UDZO', '-volname', 'XAMP2 '+version, '-srcfolder', stage, pending)
        if args.identity:
            run(*sign, pending)
        if args.notary_profile:
            result = json.loads(run('xcrun', 'notarytool', 'submit', pending,
                                    '--keychain-profile', args.notary_profile, '--wait', '--output-format', 'json', capture=True))
            (output/'notarization.json').write_text(json.dumps(result, indent=2))
            if result.get('status') != 'Accepted':
                raise RuntimeError('Apple notarization not accepted; see notarization.json')
            run('xcrun', 'stapler', 'staple', pending)
            run('xcrun', 'stapler', 'validate', pending)
            run('spctl', '--assess', '--type', 'open', '--context', 'context:primary-signature', '--verbose=2', pending)
        pending.replace(dmg)
        digest = hashlib.sha256(dmg.read_bytes()).hexdigest()
        dmg.with_suffix('.dmg.sha256').write_text(digest+'  '+dmg.name+'\n')
        print(json.dumps({'dmg': str(dmg), 'sha256': digest, 'minimum_macos': info['LSMinimumSystemVersion'],
                          'signed': bool(args.identity), 'notarized': bool(args.notary_profile)}, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--identity', help='Developer ID Application identity name or SHA-1')
    parser.add_argument('--notary-profile', help='Existing notarytool Keychain profile (never a password)')
    parser.add_argument('--bundle-id', default='com.billlin0904.xamp2')
    parser.add_argument('--macdeployqt', default='/opt/homebrew/opt/qtbase/bin/macdeployqt')
    parser.add_argument('--skip-build', action='store_true')
    parser.add_argument('--smoke-test', action='store_true', help='Launch the packaged app briefly to verify startup (requires a GUI session)')
    package(parser.parse_args())
