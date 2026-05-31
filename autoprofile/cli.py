import argparse
import json
import sys
import threading
from pathlib import Path

from autoprofile.core import paths
from autoprofile.modules import account_creation
from autoprofile.modules.profile_update import run_profile_update, settings_from_dict
from autoprofile.modules.account_checker import run_account_checker


def load_json_file(path):
    if not path.exists():
        return {}
    return json.loads(path.read_text(encoding='utf-8'))


def load_text_if_exists(path):
    if not path.exists():
        return None
    return path.read_text(encoding='utf-8')


def apply_cli_overrides(settings, args):
    overrides = {}
    if getattr(args, 'parallel', None) is not None:
        overrides['max_parallel_accounts'] = args.parallel
    if getattr(args, 'theme', None) is not None:
        overrides['default_profile_theme'] = args.theme
    if getattr(args, 'nickname', None) is not None:
        overrides['default_nickname'] = args.nickname
    if getattr(args, 'summary', None) is not None:
        overrides['default_profile_summary'] = args.summary
    if getattr(args, 'custom_url', None) is not None:
        overrides['default_custom_url'] = args.custom_url
    if getattr(args, 'avatar', None) is not None:
        overrides['profile_image_path'] = args.avatar
    if getattr(args, 'rollids', None) is not None:
        overrides['use_rollids'] = True
    if getattr(args, 'loop', False):
        overrides['loopupdateprofiles'] = True
    if getattr(args, 'loop_timeout', None) is not None:
        overrides['loop_timeout'] = args.loop_timeout

    disabled_map = {
        'no_name': 'enable_namechange',
        'no_avatar': 'enable_avatarchange',
        'no_description': 'enable_descriptionchange',
        'no_custom_url': 'enable_customurlchange',
        'no_theme': 'enable_themechange',
        'no_verify': 'enable_profile_verification',
        'no_nameclear': 'enable_nameclear',
        'no_steamid32': 'enable_gatherid32'
    }
    for arg_name, setting_name in disabled_map.items():
        if getattr(args, arg_name, False):
            overrides[setting_name] = False

    raw = settings.__dict__.copy()
    raw.update(overrides)
    return settings_from_dict(raw)


def command_profile_update(args):
    settings_path = Path(args.settings)
    settings = settings_from_dict(load_json_file(settings_path))
    settings = apply_cli_overrides(settings, args)

    accounts_text = load_text_if_exists(Path(args.accounts))
    if accounts_text is not None:
        paths.write_text_file('accounts.txt', accounts_text)
        
    if args.rollids is not None:
        rollids_text = load_text_if_exists(Path(args.rollids))
        if rollids_text is not None:
            paths.write_text_file('rollids.txt', rollids_text)

    proxies_text = load_text_if_exists(Path(args.proxies))
    if proxies_text is not None:
        paths.write_text_file('proxies.html', proxies_text)

    paths.write_json_file('settings.json', settings.__dict__)
    stop_event = threading.Event()
    def write_log(line):
        encoding = sys.__stdout__.encoding or 'utf-8'
        safe_line = line.encode(encoding, errors='replace').decode(encoding)
        sys.__stdout__.write(f'{safe_line}\n')
        sys.__stdout__.flush()

    result = run_profile_update(settings, write_log, stop_event)
    if result.error:
        print(f'CLI job failed: {result.error}')
        return 1
    print(f'CLI job finished in {result.duration_seconds:.1f}s')
    return 0


def command_account_check(args):
    settings_path = Path(args.settings)
    settings = settings_from_dict(load_json_file(settings_path))
    settings = apply_cli_overrides(settings, args)

    accounts_text = load_text_if_exists(Path(args.accounts))
    if accounts_text is not None:
        paths.write_text_file('accounts.txt', accounts_text)

    proxies_text = load_text_if_exists(Path(getattr(args, 'proxies', str(paths.data_path('proxies.html')))))
    if proxies_text is not None:
        paths.write_text_file('proxies.html', proxies_text)
        
    paths.write_json_file('settings.json', settings.__dict__)

    stop_event = threading.Event()
    def write_log(line):
        encoding = sys.__stdout__.encoding or 'utf-8'
        safe_line = line.encode(encoding, errors='replace').decode(encoding)
        sys.__stdout__.write(f'{safe_line}\n')
        sys.__stdout__.flush()

    result = run_account_checker(settings, write_log, stop_event)
    if result.error:
        print(f'CLI job failed: {result.error}')
        return 1
    return 0


def command_web(args):
    from autoprofile.web.server import main

    main(host=args.host, port=args.port)
    return 0


def command_modules(_args):
    modules = [
        {'name': 'profile_update', 'title': 'Profile Update', 'status': 'READY'},
        account_creation.module_status()
    ]
    print(json.dumps({'modules': modules}, indent=2))
    return 0


def command_tui(_args):
    try:
        from autoprofile.tui_app import run_tui
        run_tui()
        return 0
    except ImportError as e:
        print("Failed to load TUI. Please ensure 'textual' is installed: pip install textual")
        print(e)
        return 1


def build_parser():
    parser = argparse.ArgumentParser(prog='autoprofile')
    subparsers = parser.add_subparsers(dest='command', required=True)

    profile_parser = subparsers.add_parser('profile-update')
    profile_parser.add_argument('--accounts', default=str(paths.data_path('accounts.txt')))
    profile_parser.add_argument('--proxies', default=str(paths.data_path('proxies.html')))
    profile_parser.add_argument('--rollids')
    profile_parser.add_argument('--settings', default=str(paths.data_path('settings.json')))
    profile_parser.add_argument('--parallel', type=int)
    profile_parser.add_argument('--theme')
    profile_parser.add_argument('--nickname')
    profile_parser.add_argument('--summary')
    profile_parser.add_argument('--custom-url')
    profile_parser.add_argument('--avatar')
    profile_parser.add_argument('--no-name', action='store_true')
    profile_parser.add_argument('--no-avatar', action='store_true')
    profile_parser.add_argument('--no-description', action='store_true')
    profile_parser.add_argument('--no-custom-url', action='store_true')
    profile_parser.add_argument('--no-theme', action='store_true')
    profile_parser.add_argument('--no-verify', action='store_true')
    profile_parser.add_argument('--no-nameclear', action='store_true')
    profile_parser.add_argument('--no-steamid32', action='store_true')
    profile_parser.add_argument('--loop', action='store_true')
    profile_parser.add_argument('--loop-timeout', type=int)
    profile_parser.set_defaults(func=command_profile_update)

    checker_parser = subparsers.add_parser('account-check')
    checker_parser.add_argument('--accounts', default=str(paths.data_path('accounts.txt')))
    checker_parser.add_argument('--proxies', default=str(paths.data_path('proxies.html')))
    checker_parser.add_argument('--settings', default=str(paths.data_path('settings.json')))
    checker_parser.add_argument('--parallel', type=int)
    checker_parser.add_argument('--loop', action='store_true')
    checker_parser.add_argument('--loop-timeout', type=int)
    checker_parser.set_defaults(func=command_account_check)

    web_parser = subparsers.add_parser('web')
    web_parser.add_argument('--host', default='0.0.0.0')
    web_parser.add_argument('--port', type=int, default=8765)
    web_parser.set_defaults(func=command_web)

    modules_parser = subparsers.add_parser('modules')
    modules_parser.set_defaults(func=command_modules)

    tui_parser = subparsers.add_parser('tui')
    tui_parser.set_defaults(func=command_tui)

    return parser


def main(argv=None):
    parser = build_parser()
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == '__main__':
    raise SystemExit(main())
